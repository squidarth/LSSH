#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include<string.h>
#include <sys/types.h>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "crypto.h"
#define NUM_PIPES          2

#define PARENT_WRITE_PIPE  0
#define PARENT_READ_PIPE   1

int pipes[NUM_PIPES][2];

/* always in a pipe[], pipe[0] is for read and
   pipe[1] is for write */
#define READ_FD  0
#define WRITE_FD 1

#define CHILD_WRITE_FD  ( pipes[PARENT_READ_PIPE][WRITE_FD]  )
#define PARENT_READ_FD  ( pipes[PARENT_READ_PIPE][READ_FD]   )
#define PARENT_WRITE_FD ( pipes[PARENT_WRITE_PIPE][WRITE_FD] )
#define CHILD_READ_FD   ( pipes[PARENT_WRITE_PIPE][READ_FD]  )

#define ERROR_PIPE ( errfd[1] )


void error(char *msg)
{
    perror(msg);
    exit(1);
}
void hex (unsigned char *p, size_t n){
  for (int i = 0;i<n;i++) {
    printf("%02x", *p++);
  }
}

void run_bash_process() {
  char *argv[]={ "/bin/bash", "-i", 0};

  dup2(CHILD_READ_FD, STDIN_FILENO);
  dup2(CHILD_WRITE_FD, STDOUT_FILENO);
  dup2(CHILD_WRITE_FD, STDERR_FILENO);

  execv(argv[0], argv);
}

int listen_on_socket(int portno) {
  struct sockaddr_in serv_addr;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0)
               error("ERROR on binding");

  listen(sockfd,5);
  return sockfd;
}


int main(int argc, char *argv[]) {
  // Network stuff
  int sockfd, newsockfd, portno, n;

  portno =  atoi(argv[1]);
  socklen_t clilen;
  char sock_buffer[1024];

  struct sockaddr_in cli_addr;
  sockfd = listen_on_socket(portno);

  printf("Waiting for connections on port: %d \n", portno);
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  clilen = sizeof(cli_addr);

  bzero(sock_buffer,1024);

  // Setting up pipes for bash <-> socket
  // communication.
  pipe(pipes[PARENT_READ_PIPE]);
  pipe(pipes[PARENT_WRITE_PIPE]);

  int errfd[2];
  pipe(errfd);

  // Begin doing crypto handshake with client
  unsigned char iv[16];
  EVP_PKEY * full_key = generate_key();
  unsigned char server_public_key [100];
  unsigned char client_public_key[100];
  unsigned char *public_key_ptr = &server_public_key[0];
  size_t server_pub_len = i2d_PUBKEY (full_key, &public_key_ptr);

  n = recv(newsockfd,sock_buffer,75, 0);
  memcpy(iv, sock_buffer, 16);
  memcpy(client_public_key, &sock_buffer[16], 59);

  char writebuffer[1000];
  bzero(writebuffer, 1000);
  memcpy(writebuffer, server_public_key, 59);

  n =  write(newsockfd, writebuffer, 59);
  if (n < 0) {
      printf("failed to write :(");
  }

  size_t keylen;
  unsigned char * secret_key = derive (full_key, client_public_key, 59, &keylen);

  pid_t procId = fork();

  if (procId == 0 ) {
    // Child proess
    run_bash_process();
  } else if (procId > 0) {
    char plain_buffer[1024];
    char cipher_buffer[1024];
    int bytes_read;
    /* close fds not required by parent */
    close(CHILD_READ_FD);
    close(CHILD_WRITE_FD);

    fd_set rfds;
    int retval, highestFileDescriptor;

    highestFileDescriptor = PARENT_READ_FD > newsockfd ? PARENT_READ_FD : newsockfd;

    int cipher_length;
    while (1) {
      FD_ZERO(&rfds);
      FD_SET(PARENT_READ_FD, &rfds);
      FD_SET(newsockfd, &rfds);
      bzero(cipher_buffer, 1024);
      bzero(plain_buffer, 1024);

      retval = select(highestFileDescriptor + 1, &rfds, NULL, NULL, NULL);
      if (retval == -1) {
        perror("error with select");
      } else if (retval) {
        if(FD_ISSET(PARENT_READ_FD, &rfds)) {
            // data is available from bash
            cipher_length = 0;
            bytes_read = read(PARENT_READ_FD, plain_buffer, 1024);
            if (bytes_read >= 0) {
              cipher_length = encrypt_data((unsigned char *)plain_buffer, bytes_read, secret_key, iv, (unsigned char *)cipher_buffer);

              int plaintext_length = strlen(plain_buffer);

              send(newsockfd, &cipher_length,4, 0);
              send(newsockfd, &plaintext_length,4, 0);
              send(newsockfd, cipher_buffer, cipher_length, 0);
            }
        } else {
          // data is available on socket
          int cipher_length;
          int plaintext_length;
          read(newsockfd, &cipher_length, 4);
          read(newsockfd, &plaintext_length, 4);
          n = read(newsockfd,cipher_buffer,cipher_length);

          decrypt((unsigned char *)cipher_buffer, cipher_length, secret_key, iv, (unsigned char *) plain_buffer);
          memset(&plain_buffer[plaintext_length], 0, 1024-plaintext_length);
          if (n <=  0) {
            printf("failed to read");
            exit(1);
          }
          write(PARENT_WRITE_FD, plain_buffer, strlen(plain_buffer));
          if (n < 0) {
            printf("failed to write to socket");
          }
        }
      }
    }
  } else {
    printf("failed to fork a process :(\n");
  }
}
