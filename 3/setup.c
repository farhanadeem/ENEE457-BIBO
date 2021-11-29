#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <string.h>
#include "data.h"

#define DEBUG



int main(int argc, char** argv) {
  FILE *fp;
  int opt;
  int num_opt = 1;
  int flagN = 0;  
  Gradebook new_gradebook;

  int i = 0;
  unsigned char *struct_buffer;
  unsigned char *encrypt_buffer;
  /*For encrypt/decrypt*/

  unsigned char *key;
  unsigned char *iv;
  unsigned char *tag;

  /*My code here*/
  /*Check format of command*/
  if (argc != 3) {
      printf("Usage: setup <-N> <logfile pathname>\n");
      return(255);
  }

  while((opt = getopt(argc, argv, "N")) != -1){
    if(num_opt != 1){
      printf("Too many command line arguements.\n");
      printf("invalid\n");
      return(255);
    }
    switch(opt){
      case 'N': flagN = 1;
      break;
    }
    num_opt++;
  }

  if(flagN != 1){
    printf("Wrong command line arguements.\n");
    printf("invalid\n");
    return(255);
  }

  /*Is the gradebook name alphanumeric*/
  if(is_book_name_valid(argv[2], strlen(argv[2])) == 0){
    printf("invalid\n");
    return(255);
  }

  /*Does the file already exist?*/
  if(file_test(argv[2]) != 0){
    printf("File already exists!\n");
    printf("invalid\n");
    return(255);
  }

  /*Generate key and IV to be used for encryption.*/
  key = (unsigned char *)malloc(sizeof(unsigned char) * LEN);
  iv = (unsigned char *)malloc(sizeof(unsigned char) * LEN);
  tag = (unsigned char *)malloc(sizeof(unsigned char) * LEN);
  key_gen(key, LEN);
  key_gen(iv, LEN);

  /*Attempt to create the file*/
  fp = fopen(argv[2], "w");

  if (fp == NULL){
    printf("setup: fopen() error could not create file\n");
    printf("invalid\n");
    return(255);
  }

  /*Initialize gradebook*/
  new_gradebook.num_stud = 0;
  new_gradebook.num_assign = 0;
  new_gradebook.all_weights = 0;

  /*Copy the new gradebook struct into a buffer*/
  struct_buffer = (char*)malloc(sizeof(Gradebook));
  /*DISABLED ENCRYPTION*/
  encrypt_buffer = (char*)malloc(sizeof(Gradebook));
  
  memcpy(struct_buffer, &new_gradebook, sizeof(Gradebook));

  /*Encrypt Here NOTE: DISABLED FOR NOW***************************************************/
  gcm_encrypt(struct_buffer, sizeof(Gradebook), key, iv, LEN, encrypt_buffer, tag);
  
  /*Write IV then tag to the output file*/
  
  fwrite(iv, LEN, 1, fp);
  
  fwrite(tag, LEN, 1, fp);
  /*JUST THE UNENCRYPTED STRUCT VERSION
  fwrite(struct_buffer, sizeof(Gradebook), 1, fp);*/
  /*NOTE: DISABLED ENCRYPTION*/
  fwrite(encrypt_buffer, sizeof(Gradebook), 1, fp);
  
  fclose(fp);

  /*Print out the key*/
  printf("Key: ");
  for(i = 0; i < LEN; i++){
      unsigned int curr = *(key + i);
      printf("%.2x", curr);
  }
  printf("\n");

  free(key);
  free(iv);
  free(tag);
  free(struct_buffer);
  free(encrypt_buffer);
 
  return(0);

}
