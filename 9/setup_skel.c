#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

#define DEBUG

int main(int argc, char** argv) {
  FILE *fp;
  EVP_CIPHER_CTX *ctx;
  unsigned char* ciphertext;

  if (argc != 3) {
    printf("Invalid Usage\n");
    printf("Proper Usage: setup -N <logfile pathname>\n");
    handleErrors();
  }

  //check second arg is -N
  //keep in mind arg[1] is user input
  if(strcmp("-N", argv[1]) != 0){
    printf("Invalid Usage; Second Arg is not -N\n");
    printf("Proper Usage: setup -N <logfile pathname>\n");
    handleErrors();
  }
  
  //need to check if pathname is composed of alphanumeric characters(underscore and period included)
  if(strlen(argv[2]) > 100){
    printf("filename too long\n");
    handleErrors();
  }
  if(checkFileName(argv[2]) == 0){
    printf("Invalid Usage; Invalid Name Provided\n");
    printf("Proper Usage: setup -N <logfile pathname>\n");
    handleErrors();
  }
  if(access(argv[2], F_OK ) == 0 ) {
    printf("Invalid: File %s already exists\n", argv[2]);
    handleErrors();
  } 

  //create the file using the valid name
  fp = fopen(argv[2], "w");
  if (fp == NULL){
    printf("Invalid: no file created\n");
    handleErrors();
  }

  //generate random key and iv
  unsigned char *key = (unsigned char *) malloc(sizeof(unsigned char)*(32));
  unsigned char *iv = (unsigned char *) malloc(sizeof(unsigned char)*(12));
  FILE* random = fopen("/dev/urandom", "r");
  fread(key, sizeof(unsigned char)*32, 1, random);
  fread(iv, sizeof(unsigned char)*12, 1, random);
  fclose(random);
  // printf("Key:\n");
  // BIO_dump_fp (stdout, (const char *)key, 32);
  // printf("IV:\n");
  // BIO_dump_fp (stdout, (const char *)iv, 12);

  //encrypt the resulting file
  int cipher_len = encrypt("0-0--", 5, key, iv, &ciphertext);

  //write to the file
  fwrite(ciphertext, sizeof(unsigned char), cipher_len, fp);

  //close file and print key
  fclose(fp);
  printf("Key is: ");
  for(int i = 0; i < 32; i++){
    printf("%02x", *(key+i));
  }
  printf("\n");  
  return(0);

}
