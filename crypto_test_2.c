#include "crypto.h"
void hex (unsigned char *p, size_t n){ while(n--) printf("%02x", *p++); }


int main (void){
  EVP_PKEY * pkey1 = gen(), * pkey2 = gen();
  unsigned char pub1 [100], pub2 [100], *ptr1 = &pub1[0], *ptr2 = &pub2[0];
  // Important detail: i2d_PUBKEY is used for getting the
  // public key out of the generated keypair
  size_t publen1 = i2d_PUBKEY (pkey1, &ptr1), publen2 = i2d_PUBKEY (pkey2, &ptr2);
  printf ("pub1="); hex(pub1, publen1); putchar('\n');
  printf ("pub2="); hex(pub2, publen2); putchar('\n');

  size_t len1, len2;
  unsigned char * secret_key_1 = derive (pkey1, pub2, publen2, &len1);
  unsigned char * secret_key_2 = derive (pkey2, pub1, publen1, &len2);

  printf ("prv1/pub2="); hex(secret_key_1, len1); putchar('\n');

  printf ("prv2/pub1="); hex(secret_key_2, len2); putchar('\n');
  /* don't bother freeing for Q&D test code */

  //unsigned char *key = (unsigned char *)"01234567890123456789012345678901";

  /* A 128 bit IV */
  unsigned char iv[16];
  int randomData = open("/dev/urandom", O_RDONLY);
  if (randomData < 0) {
    handleErrors();
  }

  int result = read(randomData, iv, 16); 
  if (result < 0) {
    handleErrors();
  }

  /* Message to be encrypted */
  unsigned char *plaintext =
                (unsigned char *)"The quick brown fox jumps over the lazy dog";

  /* Buffer for ciphertext. Ensure the buffer is long enough for the
   * ciphertext which may be longer than the plaintext, dependant on the
   * algorithm and mode
   */
  unsigned char ciphertext[128];

  /* Buffer for the decrypted text */
  unsigned char decryptedtext[128];

  int decryptedtext_len, ciphertext_len;

  /* Encrypt the plaintext */
  ciphertext_len = encrypt_data (plaintext, strlen ((char *)plaintext), secret_key_1, iv,
                            ciphertext);

  /* Do something useful with the ciphertext here */
  printf("Ciphertext is:\n");
  BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);

  /* Decrypt the ciphertext */
  decryptedtext_len = decrypt(ciphertext, ciphertext_len, secret_key_1, iv,
    decryptedtext);

  /* Add a NULL terminator. We are expecting printable text */
  decryptedtext[decryptedtext_len] = '\0';

  /* Show the decrypted text */
  printf("Decrypted text is:\n");
  printf("%s\n", decryptedtext);


  return 0;
}
