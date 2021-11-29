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

#include <ctype.h>

#define NON_VAR_LENGTH 0     //TODO change me



/*MY FUNCTIONS*************************/
void key_gen(char* key, int length){
  FILE *random = fopen("/dev/urandom", "r");
  fread(key, sizeof(unsigned char) * length, 1, random);
}

int handleErrors(){
  printf("invalid\n");
  exit(255);
}

int gcm_encrypt(unsigned char *plaintext, int plaintext_len,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *ciphertext,
                unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;


    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the encryption operation. */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
        handleErrors();

    /*
     * Set IV length if default 12 bytes (96 bits) is not appropriate
     */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        handleErrors();

    /* Initialise key and IV */
    if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Normally ciphertext bytes may be written at
     * this stage, but this does not occur in GCM mode
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Get the tag */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
        handleErrors();

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *tag,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the decryption operation. */
    if(!EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
        handleErrors();

    /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        handleErrors();

    /* Initialise key and IV */
    if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
        handleErrors();

    /*
     * Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    if(ret > 0) {
        /* Success */
        plaintext_len += len;
        return plaintext_len;
    } else {
        /* Verify failed */
        return -1;
    }
}

/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

int is_number(char *arg){
  if(*arg == '\0')
    return 0;
  while(*arg != '\0'){
    if(isdigit(*arg) == 0){
      return 0;
    }
    arg++;
  }
  return 1;
}

int is_float(char *arg){
  int num_points = 0;
  if(*arg == '\0')
    return 0;
  while(*arg != '\0'){
    if(*arg == '.'){
      num_points++;
    }
    else if(isdigit(*arg) == 0){
      return 0;
    }
    arg++;
  }
  if(num_points > 1){
      return 0;
  }
  return 1;
}

/*For validating names*/
int is_book_name_valid(char *name, int len){
  int i = 0;
  int is_valid = 1;
  if(len == 0)
    return 0;
  for(i = 0; i < len; i++){
    char curr = *(name + i);
    if(isalnum(curr) == 0){
      is_valid = 0;
      break;
    }
  }
  return is_valid;
}

/*Validate the key*/
int is_key_valid(char *key, int len){
  int i = 0;
  int is_valid = 1;
  if(len == 0)
    return 0;
  for(i = 0; i < len; i++){
    char curr = *(key + i);
    if(isxdigit(curr) == 0){
      is_valid = 0;
      break;
    }
  }
  return is_valid;
}

/*Validate student name and last name*/
int is_stud_name_valid(char *name, int len){
  int i = 0;
  int is_valid = 1;
  if(len == 0)
    return 0;
  for(i = 0; i < len; i++){
    char curr = *(name + i);
    if(isalpha(curr) == 0){
      is_valid = 0;
      break;
    }
  }
  return is_valid;
}

/*Validate assignment name*/
int is_assign_name_valid(char *name, int len){
  int i = 0;
  int is_valid = 1;
  if(len == 0)
    return 0;
  for(i = 0; i < len; i++){
    char curr = *(name + i);
    if(isalpha(curr) == 0 && isdigit(curr) == 0){
      is_valid = 0;
      break;
    }
  }
  return is_valid;
}

int is_points_valid(int input){
  if(input < 0)
    return 0;
  return 1;
}

int is_grade_valid(int input){
  if(input < 0)
    return 0;
  return 1;
}

int is_weight_valid(float input){
  if(input < 0 || input > 1)
    return 0;
  return 1;
}


