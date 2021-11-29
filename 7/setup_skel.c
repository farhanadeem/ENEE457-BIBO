#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <regex.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"
#define DEBUG

#define LEN 16
// 128 bits


/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

int main(int argc, char** argv) {
  FILE *fp;
  unsigned char *key = NULL, *gradebook_name;
  int  i = 0, value = 0;
  FILE * random;
  regex_t regex;
  Gradebook G;
  Buffer B;


  if (argc != 3 || strcmp(argv[1], "-N") != 0) {
    printf("Usage: setup -N <logfile pathname>\n");
    return(255);
  }

  //use a regular expression to make sure the title only contains alphanumeric + _.
  value = regcomp(&regex, "^[[:alnum:]_\\.]+$", REG_EXTENDED);
  if(value){
    printf("Could not compile regex.\n");
    return(255);
  }

  //match input.
  value = regexec(&regex, argv[2], 0,NULL, 0);
  if(value){
    printf("invalid\n");
    return(255);
  }

  //if the gradebook already exists, don't let them create another one.
  if(access(argv[2], F_OK) == 0){
    //file already exists, throw error.
    printf("invalid\n");
    return(255);
  }

  fp = fopen(argv[2], "w"); //check input against alpha-numeric + _ . file names
  if (fp == NULL){
#ifdef DEBUG
    printf("setup: fopen() error could not create file\n");
#endif
    printf("invalid\n");
    return(255);
  }

#ifdef DEBUG
  if (file_test(argv[1]))
    printf("created file named %s\n", argv[1]);
#endif

  //create secret key and IV and print key to stdout
  key = (unsigned char *) malloc(sizeof(unsigned char)*LEN);
  random = fopen("/dev/urandom", "r");
  fread(key, sizeof(unsigned char)*LEN, 1, random);
  //key[LEN] = '\0'; //NULL terminate.

  printf("Key is: ");
  for(i = 0; i < LEN; i++){
    printf("%.2x", key[i]);
  }
  printf("\n");

  //use key to make a gradebook struct and write it to the filepath.
  B.Buf = (char *) malloc(sizeof(Gradebook));
  B.Length = sizeof(Gradebook);
  G.num_assignments = 0; //set number of assignments = 0;
  G.next_id = 0;
  G.num_students = 0;
  memcpy(B.Buf, &G, sizeof(Gradebook)); //copy the newly created gradebook into the buffer.

  write_to_path(argv[2], &B, key); //write the gradebook to the specified path with the key.

  //close file pointers and free memory
  fclose(random);
  fclose(fp);

  regfree(&regex);
  free(key);
  free(B.Buf);

  return(0);

}
