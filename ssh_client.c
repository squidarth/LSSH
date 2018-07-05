#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "crypto.h"

void hex (unsigned char *p, size_t n){
  for (int i = 0;i<n;i++) {
    printf("%02x", p[i]);
  }
}
int main(int argc, char *argv[]) {
  // Generate crypto parameters
  // Create a random initialization vector
  // of the correct length
  if (argc < 3) {
    printf("Please provide a host and port");
    exit(1);
  }

  unsigned char iv[16];
  int randomData = open("/dev/urandom", O_RDONLY);
  if (randomData < 0) {
    handleErrors();
  }

  int result = read(randomData, iv, 16);
  if (result < 0) {
    handleErrors();
  }

  EVP_PKEY * full_client_key = generate_key();
  unsigned char server_public_key [100];
  unsigned char client_public_key[100];
  unsigned char *client_public_key_ptr = &client_public_key[0];
  size_t client_pub_len = i2d_PUBKEY (full_client_key, &client_public_key_ptr);


  // Initiate connection to server
  const char* hostname = argv[1]; /* localhost */
  const char* portname = argv[2];
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_ADDRCONFIG;
  struct addrinfo* res=0;
  int err = getaddrinfo(hostname, portname, &hints, &res);
  if (err!=0) {
      printf("failed to resolve remote socket address (err=%d)",err);
  }
  int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd == -1) {
      printf("%s", strerror(errno));
  }
  if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
      printf("%s", strerror(errno));
  }

  unsigned char write_buffer[1024];

  bzero(write_buffer, 1024);
  memcpy(write_buffer, iv, 16);

  memcpy(&write_buffer[16], client_public_key, 59);
  // Send over initialization vector and
  // the client public key.

  write(fd, write_buffer,75);

  char read_buffer[1024];
  bzero(read_buffer, 1024);

  // receive server public key
  int n = recv(fd, read_buffer,59,0);

  memcpy(server_public_key, read_buffer, 59);

  size_t keylen;

  unsigned char * secret_key = derive (full_client_key, server_public_key, 59, &keylen);

  printf("Welcome to the Less Secure Shell!\n");
  // Now that crypto handshake is over, start
  // sending bash commands to server.
  fd_set rfds;
  int retval;
  char cipherbuffer[1024];
  char plainbuffer[1024];
  while (1) {
      FD_ZERO(&rfds);
      FD_SET(STDIN_FILENO, &rfds);
      FD_SET(fd, &rfds);
      bzero(cipherbuffer, 1024);
      bzero(plainbuffer, 1024);

      retval = select(fd + 1, &rfds, NULL, NULL, NULL);
      if (retval == -1) {
        perror("error with select");
      } else if (retval) {
        if(FD_ISSET(fd, &rfds)) {
          // data available on socket
          int cipher_length;
          int plaintext_length;
          read(fd, &cipher_length, 4);
          read(fd, &plaintext_length, 4);
          n  = read(fd, cipherbuffer, cipher_length);


          decrypt((unsigned char *)cipherbuffer, cipher_length, secret_key, iv, (unsigned char *) plainbuffer);

          memset(&plainbuffer[plaintext_length], 0, 1024-plaintext_length);

          printf("%s", plainbuffer);
          fflush(stdout);
        } else {
          // data available on stdin
          int cipher_length;
          int plaintext_length;
          n = read(STDIN_FILENO, plainbuffer, 255);
          cipher_length = encrypt_data( (unsigned char *)plainbuffer, strlen(plainbuffer), secret_key, iv, (unsigned char *)cipherbuffer);

          plaintext_length = strlen(plainbuffer);
          write(fd, &cipher_length,4);
          write(fd, &plaintext_length,4);
          write(fd, cipherbuffer, cipher_length);
        }
      }
  }
}
