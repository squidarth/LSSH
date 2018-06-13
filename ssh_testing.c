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

#define PARENT_READ_FD  ( pipes[PARENT_READ_PIPE][READ_FD]   )
#define PARENT_WRITE_FD ( pipes[PARENT_WRITE_PIPE][WRITE_FD] )

#define CHILD_READ_FD   ( pipes[PARENT_WRITE_PIPE][READ_FD]  )
#define CHILD_WRITE_FD  ( pipes[PARENT_READ_PIPE][WRITE_FD]  )

void error(char *msg)
{
    perror(msg);
    exit(1);
}
int main() {
  pipe(pipes[PARENT_READ_PIPE]);
  pipe(pipes[PARENT_WRITE_PIPE]);

  pid_t procId = fork();

  if (procId == 0 ) {
    char *argv[]={ "/bin/bash","-i", 0};
    printf("I am child with proc id: %d\n", procId);

    dup2(CHILD_READ_FD, STDIN_FILENO);
    dup2(CHILD_WRITE_FD, STDOUT_FILENO);
    execv(argv[0], argv);
  } else if (procId > 0) {
    printf("I am parent with proc id: %d\n", procId);
    char buffer[10000];
    int file_listing;
    /* close fds not required by parent */
    close(CHILD_READ_FD);
    close(CHILD_WRITE_FD);
    // Write to child’s stdin
    char *commands[] = {"ls", "cd ../..", "ls"};
    printf("commands size: %lu\n", sizeof(commands)/sizeof(char*));
    for (int i = 0;i< sizeof(commands)/sizeof(char*);++i) {
      char *write_command = commands[i];
      char * gray_box = "\n";
      printf("length of gray_box %d\n", strlen(gray_box));
      printf("write command is %s\n", write_command);

      char *new_command = (char* )malloc(strlen(write_command) + strlen(gray_box) + 1);
      sprintf(new_command, "%s%s", write_command, gray_box);
      printf(new_command);

      write(PARENT_WRITE_FD, new_command, strlen(new_command));
      // Read from child’s stdout
      file_listing = read(PARENT_READ_FD, buffer, sizeof(buffer)-1);
      if (file_listing >= 0) {
          buffer[file_listing] = 0;
          //printf("%s", buffer);
      }
      free(new_command);
    }
  } else {
    printf("failed to fork a proccess :(\n");
  }



//  int sockfd, newsockfd, portno, clilen, n;
//
//  char buffer[256];
//
//  struct sockaddr_in serv_addr, cli_addr;
//  sockfd = socket(AF_INET, SOCK_STREAM, 0);
//  bzero((char *) &serv_addr, sizeof(serv_addr));
//  portno = 3334;
//
//  serv_addr.sin_family = AF_INET;
//  serv_addr.sin_port = htons(portno);
//  serv_addr.sin_addr.s_addr = INADDR_ANY;
//
//  if (bind(sockfd, (struct sockaddr *) &serv_addr,
//           sizeof(serv_addr)) < 0)
//               error("ERROR on binding");
//
//  listen(sockfd,5);
//
//  printf("Waiting for connections on port: %d \n", portno);
//  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
//  while (1) {
//
//    clilen = sizeof(cli_addr);
//
//    bzero(buffer,256);
//    printf("starting to read\n");
//    n = recv(newsockfd,buffer,255, 0);
//    printf("finished reading\n");
//    if (n > 0) {
//      printf("received %s\n", buffer);
//    } else {
//      printf("failed to read");
//    }
//    printf("starting to write\n");
//
//    n = send(newsockfd,"I got your message\n",20, 0);
//
//    printf("finished write\n");
//    if (n < 0) {
//      printf("failed to write :(");
//    }
//  }
//
//  close(sockfd);
}
