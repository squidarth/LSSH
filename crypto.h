#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/conf.h>
#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/ec.h>
#include <openssl/err.h>

void handleErrors(void);
int encrypt_data(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext);

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
  unsigned char *iv, unsigned char *plaintext);

EVP_PKEY * gen (void);
unsigned char * derive (EVP_PKEY * self,
    const unsigned char * peer_ptr, size_t peer_len, size_t *len_ptr);

unsigned char * derive (EVP_PKEY * self,
    const unsigned char * peer_ptr, size_t peer_len, size_t *len_ptr);
