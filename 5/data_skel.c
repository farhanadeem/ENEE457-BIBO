#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/comp.h>


void handleErrors(void) {
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext) {

   EVP_CIPHER_CTX *ctx;

   int len;
   int ciphertext_len;
   int ret;

   /* Create and initialise the context */
   if(!(ctx = EVP_CIPHER_CTX_new()))
     handleErrors();

   /*
   * Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
   if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
     handleErrors();

   /* Padding Disabled */
   if(1 != EVP_CIPHER_CTX_set_padding(ctx, 0))
   	handleErrors();

   /*
   * Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
   if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
     handleErrors();
   ciphertext_len = len;

   /*
   * Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
   ret = EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
   if(1 != ret)
     handleErrors();
   ciphertext_len += len;

   /* Clean up */
   EVP_CIPHER_CTX_free(ctx);

   if (ret <= 0) {
   	return -1;
   } else {
   	return 1;
   }
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext) {

   EVP_CIPHER_CTX *ctx;

   int len;
   int plaintext_len;
   int ret;

   /* Create and initialise the context */
   if(!(ctx = EVP_CIPHER_CTX_new()))
      handleErrors();

   /*
   * Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
   if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
      handleErrors();

   /* Disabling padding */
   if(1 != EVP_CIPHER_CTX_set_padding(ctx, 0))
   		handleErrors();

   /*
   * Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary.
   */
   if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
      handleErrors();
   plaintext_len = len;

   /*
   * Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
   ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
   if(1 != ret)
      handleErrors();
   plaintext_len += len;

   /* Clean up */
   EVP_CIPHER_CTX_free(ctx);

   if (ret > 0) {
   	return 1;
   } else {
   	return -1;
   }
}
