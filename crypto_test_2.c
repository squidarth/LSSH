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

void hex (unsigned char *p, size_t n){ while(n--) printf("%02x", *p++); }

void err (const char * msg){ fprintf(stderr, "%s:\n", msg); ERR_print_errors_fp(stderr); exit(1); }
void handleErrors(void)
{
  ERR_print_errors_fp(stderr);
  abort();
}

int encrypt_data(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int ciphertext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    handleErrors();
  ciphertext_len = len;

  /* Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
  ciphertext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
  unsigned char *iv, unsigned char *plaintext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int plaintext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    handleErrors();
  plaintext_len = len;

  /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
  plaintext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}
EVP_PKEY * gen (void) {
  EVP_PKEY_CTX *pctx, *kctx;
  EVP_PKEY *params = NULL, *pkey = NULL;

  if( NULL == (pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL)) ) err("CTX1_new");
  if( 1 != EVP_PKEY_paramgen_init(pctx) ) err("pg_init");
  if( 1 != EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1) ) err("pg_curve");
  if( 1 != EVP_PKEY_paramgen(pctx, &params) ) err("pg");
  if( NULL == (kctx = EVP_PKEY_CTX_new(params, NULL)) ) err("CTX2_new");
  if( 1 != EVP_PKEY_keygen_init(kctx) ) err("kg_init");



  // WHERE THE KEY IS ACTUALLY GENERATED
  if( 1 != EVP_PKEY_keygen(kctx, &pkey) ) err("kg");

  // This does not need to be set in later versions of openssl
  EC_KEY_set_asn1_flag (pkey->pkey.ec, OPENSSL_EC_NAMED_CURVE);
  /* point format needed before 'sending' and this is convenient */
  EC_KEY_set_conv_form (pkey->pkey.ec, POINT_CONVERSION_COMPRESSED);

  EVP_PKEY_CTX_free(pctx);
  EVP_PKEY_CTX_free(kctx);
  EVP_PKEY_free(params);
  return pkey;
}
unsigned char * derive (EVP_PKEY * self,
    const unsigned char * peer_ptr, size_t peer_len, size_t *len_ptr){
  EVP_PKEY * peer = d2i_PUBKEY (NULL, &peer_ptr, peer_len);
  /* DON'T change EC_GROUP; point_format not needed on 'receive' */

  EVP_PKEY_CTX *ctx; unsigned char * buf_ptr;
  if( !(ctx = EVP_PKEY_CTX_new (self, NULL)) ) err("CTX_new");
  if( 1 != EVP_PKEY_derive_init(ctx) ) err("derive_init");
  if( 1 != EVP_PKEY_derive_set_peer(ctx, peer) ) err("derive_peer");
  if( 1 != EVP_PKEY_derive (ctx, NULL, len_ptr) ) err("derive1");
  if( !(buf_ptr = OPENSSL_malloc (*len_ptr)) ) err("malloc");
  if( 1 != EVP_PKEY_derive (ctx, buf_ptr, len_ptr) ) err("derive2");
  EVP_PKEY_CTX_free(ctx);
  EVP_PKEY_free(peer);
  return buf_ptr;
}

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
