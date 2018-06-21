#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "crypto.c"

void hex (unsigned char *p, size_t n){ while(n--) printf("%02x", *p++); }

int main() {
  // Generate crypto parameters
  // Create a random initialization vector
  // of the correct length
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
  const char* hostname = "127.0.0.1"; /* localhost */
  const char* portname = "3345";
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
  printf("Connecting to socket\n");
  int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd == -1) {
      printf("%s", strerror(errno));
  }
  if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
      printf("%s", strerror(errno));
  }
  printf("Connected to socket\n");

  unsigned char write_buffer[1024];

  bzero(write_buffer, 1024);
  memcpy(write_buffer, iv, 16);

  memcpy(&write_buffer[16], client_public_key, 59);

  write(fd, write_buffer,75);

  char read_buffer[1024];
  bzero(read_buffer, 1024);

  // receive server public key
  printf("Before read\n");
  int n = read(fd, read_buffer,59);
  printf("Read %s bytes\n", n);

  printf("Before memcpy\n");
  memcpy(server_public_key, read_buffer, 59);


  size_t keylen;

  printf("length of server key: %s\n", strlen(server_public_key));
  unsigned char * secret_key = derive (full_client_key, server_public_key, 59, &keylen);
  printf("PRINTING SECRET KEY\n");
  hex(secret_key, keylen);

  // Send over initialization vector and
  // the client public key.
}
