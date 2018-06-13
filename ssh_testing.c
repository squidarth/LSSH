#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include<string.h>
#include <sys/types.h>
#include<openssl/dh.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}
int main() {
//  FILE *f;
//  f = popen("bash -c ls", "r");
//  char fbuff[256];
//  //fgets(fbuff, 100, f);
//  while(fgets(fbuff, sizeof(fbuff), f) != NULL) {
//    printf("%s", fbuff);
//  }
//  printf("\n");
  pid_t procId = fork();
  if (procId == 0 ) {
    printf("I am child with proc id: %d", procId);
  } else if (procId > 0) {
    printf("I am parent with proc id: %d", procId);
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
