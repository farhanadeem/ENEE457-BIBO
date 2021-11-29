#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "data.h"

#define DEBUG
#define LEN 32 

/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

int validate_input(char **argv_){
   int value1;
   int value2 = 1;
   //This is for the command type
   if(strcmp(argv_[1],"-N")==0){
      value1 = 1;
   }else{
      value1 = 0;
   }
   //This is to ensure the characters of the filename are good
   for(int p = 0; p<strlen(argv_[2]); p++){
      char curr = argv_[2][p];
      if(!((curr >= 'A' && curr <= 'Z') || (curr >= 'a' && curr <= 'z') || (curr >= '1' && curr <='9')|| (curr == '.')||(curr == '_'))){
	   value2 = 0;   
      }
   }
   
   return value1 && value2;
}


unsigned char* generate_key(){

   unsigned char *key1 = (unsigned char*) malloc(sizeof(unsigned char)*LEN);
   FILE *random = fopen("/dev/urandom", "r");
   fread(key1, sizeof(unsigned char)*LEN, 1, random);
   fclose(random);
   return key1;
}




int main(int argc, char** argv) {
  FILE *fp;
  char *key = NULL;
  //printf("%s\n", argv[0]);
  //This will check if you gave only three inputs in command line, which should be executable, tag, filename.
  if (argc <= 1) {
    printf("Need to include a tag and a filename\n");
    return(255);
  }else if(argc <= 2){
    printf("Usage: setup <logfile pathname>\n");
    return(255);
  }else if(argc > 3){
  printf("too many inputs");
  return(255);
  }




//--------------------------------------------------------------------------------------------
/* add your code here */
//3.) generate key and print to STDOUT
//2.) make sure that filename is not already in use, if it is then print invalid and error
//1.) maybe use regex to validate the tag(argv[1]) and filename(argv[2]).
//4.) I should encrypt the Gradebook file generated and use the cipher as input to a MAC. This is so 
//    in gradebookadd_skel.c and gradebookdisplay_skel.c we can just run MAC again to verify, remember key is 
//    passed in from user in the command line, so I dont need to store it, just encrypt.

//First we need to validate the input from the command line input. 
if (validate_input(argv) == 0){
   printf("validate_input caught\n");
   printf("Invalid\n");
   return(255);
}
   unsigned char *key1;
   unsigned char *IV;


//NOW MAKE SURE THAT FILE NAME IS NOT ALREADY IN USE
if(!(file_test(argv[2]) == 0)){  //this was inside condition originally access(argv[2], R_OK|W_OK) == 0
//if we are in here then the file exists
   printf("File already exists\n");
   printf("Invalid\n");
   return(255);

}else{//IN HERE OPEN THE FILE AND GENERATE THE KEY FOR THE FILE.
//if we are in here then the file does not exist and we can continue

// OPEN the third input at the command line and make that the name of the gradebook.
  fp = fopen(argv[2], "w"); // could also do *(argv+2)

  if (fp == NULL){
#ifdef DEBUG
    printf("setup: fopen() error could not create file\n");
#endif
    printf("invalid\n");
    return(255);
  }
  
#ifdef DEBUG
  if (file_test(argv[2]))
    printf("created file named %s\n", argv[2]);
#endif
  
   //THIRD WE GENERATE THE KEY and PRINT TO STDOUT.
   printf("Key for this Gradebook: ");
   key1 = generate_key();

  for(int i = 0; i < LEN; i++){
      unsigned int curr = *(key1 + i);
      printf("%.2x", curr);
  }
  printf("\n");
      
   //WRITE STATEMENTS IN THE FILE FOR FORMATTING REASONS WHEN I LOAD THE DATA IN LATER.
   fprintf(fp,"%s","num_of_assignments: 0 \n");
   fprintf(fp,"%s","num_of_students: 0 \n");   
   fprintf(fp,"%s","sum_of_weights: 0 \n");   
   fclose(fp);
   
   
}//end of else block


  IV = generate_key();
  //NOW WE SHOULD ENCRPYT THE GRADEBOOK WITH THE KEY
  fp = fopen(argv[2],"rb");
  fseek(fp, 0, SEEK_END);
  long file_len = ftell(fp);
  fclose(fp);
  fp = fopen(argv[2],"rb");
  unsigned char *buffer = (unsigned char *)malloc(file_len+1);
  fread(buffer, sizeof(unsigned char), file_len, fp);
  
  unsigned char *cipher_text = (unsigned char*)malloc(file_len+1);
  unsigned char *parity = (unsigned char*)malloc(16*sizeof(unsigned char));
  unsigned char *plaintext = (unsigned char*)malloc(file_len+1);  
  gcm_encrypt(buffer, file_len, key1, IV, 1, cipher_text, parity);
  
  fp=freopen(NULL,"w",fp);
  fwrite(cipher_text, file_len, 1, fp);
  
  //NOW SAVE THE PARITY IN A DIFFERENT FILE
  FILE *fp1 = fopen("temp.txt", "a");
  fprintf(fp1,"%s ", argv[2]);
  //now print the parity into the file
    for(int i = 0; i < LEN; i++){
      unsigned int curr = *(parity + i);
      fprintf(fp1, "%.2x", curr);
  }
    fprintf(fp1, " ");

    //now print the IV into the file
    for(int i = 0; i < LEN; i++){
      unsigned int curr = *(IV + i);
      fprintf(fp1, "%.2x", curr);
  }
  
  fprintf(fp1, "\n");
  //TO DECRPYT
  //gcm_decrypt(cipher_text, file_len, parity, key1, IV, 1, plaintext);
  //fp=freopen(NULL,"w",fp);
  //fwrite(plaintext, file_len, 1, fp); 



  fclose(fp);
  fclose(fp1);

  return(0);

}
