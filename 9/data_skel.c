#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "data.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/comp.h>

#define MAX_FILE_SIZE 1000000   //equivalent to 1 MB
#define MAX_DUB_SIZE 24         //max number of digits in a double
#define MAX_INT_SIZE 10         //max number of digits in an int

void handleErrors() {
  printf("invalid\n");
  exit(255);
}

void freeBuffer(Buffer* b){
  free(b->buf);
  free(b);
}

int compute_Gradebook_size(Gradebook R) {
  int res = 0;
  //Account for numAssignments and NumStudents and 3 dashes
  res += MAX_INT_SIZE*2+3;
  //add in points and weights for each assignment
  res += MAX_INT_SIZE*R.numAssignments + MAX_DUB_SIZE*R.numAssignments;
  //add in the lengths of each assignment name
  Assignment* currA = R.assignments;
  while(currA != NULL){
    res += strlen(currA->assignmentName)+1;
    currA = currA->next;
  }
  //add lengths of first and last name with size of the grade array
  Student* currS = R.students;
  while(currS != NULL){
    res += strlen(currS->firstName)+1;
    res += strlen(currS->lastName)+1;
    res += MAX_INT_SIZE*R.numAssignments+1;
    currS = currS->next;
  }
  return res;
}

int checkHex(char* key){
  int res = 0;
  char* tmp = key;
  for(int i = 0; i < 64; i++){
    if(!isxdigit(*tmp)){
      printf("nonhexdigit included in key\n");
      handleErrors();
    }
    tmp++;
  }
  return 1;
}

//produce A | B
void concat_buffs(Buffer *A, char* buffer, int len) {
  strcat(A->buf, buffer);
  A->length = A->length + len;
}

void write_to_path(FILE* fp, Gradebook G, unsigned char *key) {
  Buffer B = {};
  char numBuf[MAX_DUB_SIZE];
  int len, gradebooksize;
  //create buffer to encrypt
  gradebooksize = compute_Gradebook_size(G);
  B.buf = calloc(1, gradebooksize+1);
  B.length = 0;
  /***************************************************
   * Format:
   * (NumStudents)-(NumAssignments)-(Assignments)-(Students)
   * Assignment format: AssignName Points Weight;AssignName Points Weight;
   * Student format: FirstName LastName [allGrades seperated by space];FirstName LastName [allGrades seperated by space]
   * ***************************************************/
  //place numStudents in buffer
  len = sprintf(numBuf, "%d", G.numStudents);
  concat_buffs(&B, numBuf, len);
  //place seperator
  concat_buffs(&B, "-", 1);

  //place numAssignments in buffer
  len = sprintf(numBuf, "%d", G.numAssignments);
  concat_buffs(&B, numBuf, len);
  //place seperator
  concat_buffs(&B, "-", 1);
  //place all of the assignments on the buffer along with their points and weights
  Assignment* currA = G.assignments;
  while(currA != NULL){
    //add the assignemnt name
    concat_buffs(&B, currA->assignmentName, strlen(currA->assignmentName));
    //add a space
    concat_buffs(&B, " ", 1);
    //add the points
    len = sprintf(numBuf, "%d", currA->points);
    concat_buffs(&B, numBuf, len);
    //add space
    concat_buffs(&B, " ", 1);
    //add the weight
    len = sprintf(numBuf, "%f", currA->weight);
    concat_buffs(&B, numBuf, len);
    //add the semicolin
    concat_buffs(&B, ";", 1);
    //move to the next assignment
    currA = currA->next;
  }
  //place seperator
  concat_buffs(&B, "-", 1);
  //write the students with their grades to the buffer
  Student* currS = G.students;
  while(currS != NULL){
    //write the first name
    concat_buffs(&B, currS->firstName, strlen(currS->firstName));
    //write a space
    concat_buffs(&B, " ", 1);
    //write the last name
    concat_buffs(&B, currS->lastName, strlen(currS->lastName));
    //write a space
    concat_buffs(&B, " ", 1);
    //write all of the grades
    Grade* currG = currS->grades;
    while(currG != NULL){
      //write number
      len = sprintf(numBuf, "%d", currG->score);
      // printf("%s\n", numBuf);
      concat_buffs(&B, numBuf, len);
      //write space
      if(currG->next != NULL){
        concat_buffs(&B, " ", 1);
      }
      //move to next grade
      currG = currG->next;
    }
    //write the semicolon
    concat_buffs(&B, ";", 1);
    //move to the next student
    currS = currS->next;
  }
  if(B.length >= MAX_FILE_SIZE){
    printf("file too large to write\n");
    handleErrors();
  }
  // printf("|%s|\n", B.buf);

  /*********************************************************
   * Buffer obtained, encrypt and write to fp
   * *******************************************************/
  unsigned char* ciphertext;
  
  //generate iv
  unsigned char *iv = (unsigned char *) malloc(sizeof(unsigned char)*(12));
  FILE* random = fopen("/dev/urandom", "r");
  fread(iv, sizeof(unsigned char)*12, 1, random);
  fclose(random);
  //call encrypt
  //printf("strlen: %ld; B.len: %ld\n", strlen(B.buf), B.length);
  int cipherLen = encrypt(B.buf, B.length, key, iv, &ciphertext);
  //printf("strlen: %ld; cipherLen: %d\n", strlen(ciphertext), cipherLen);
  //write ciphertext to file
  fwrite(ciphertext, sizeof(unsigned char), cipherLen, fp);
}

Buffer* read_from_path(FILE* fp, unsigned char *key) {
  Buffer*  B = malloc(sizeof(Buffer));
  int inlen;
  unsigned char *iv = calloc(1, sizeof(unsigned char)*13);
  unsigned char *tag = calloc(1, sizeof(unsigned char)*17);
  unsigned char inbuf[MAX_FILE_SIZE] = {0};

  //read in the iv and tag
  inlen = fread(tag, 1, 16, fp);
  if(inlen != 16){
    printf("Error reading tag in read_from_path\n");
    handleErrors();
  }
  inlen = fread(iv, 1, 12, fp);
  if(inlen != 12){
    printf("Error reading iv in read_from_path\n");
    handleErrors();
  }
  //read in all the data from the file then decrypt
  inlen = fread(inbuf, 1, MAX_FILE_SIZE, fp);
  if(!feof(fp)){
    if(inlen == MAX_FILE_SIZE){
      // the filesize  exceeded the max size for my design
      printf("File too large for my design; Increase MAX_FILE_SIZE to fix\n");
      handleErrors();
    }else{
      //error occured reading the file
      printf("Error reading in file in read_from_path\n");
      handleErrors();
    }
  }
  unsigned char* plaintext = calloc(1, sizeof(unsigned char)*(inlen+1));
  //inbuf contains the whole file so you can call decrypt
  int len = decrypt(inbuf, inlen, key, iv, tag, &plaintext);
  //initialize the buffer
  B->length = len;
  B->buf = plaintext;
  return B;
}

int get_Gradebook(Gradebook *R, Buffer B) {
  unsigned int  buflen = B.length;
  unsigned char* buf = B.buf;
  char* token; 

  //get number of students
  token = strtok(buf, "-");
  if(token == NULL){
    printf("get gradebook parse error1\n");
    handleErrors();
  }
  R->numStudents = atoi(token);

  //get number of assignments
  token = strtok(NULL, "-");
  if(token == NULL){
    printf("get gradebook parse error2\n");
    handleErrors();
  }
  R->numAssignments = atoi(token);

  //get list of assignments with weights, points, and assignment number
  char* assignments;
  if(R->numAssignments != 0){
    token = strtok(NULL, "-");
    if(token == NULL && R->numAssignments != 0){
      printf("get gradebook parse error3\n");
      handleErrors();
    }else{
      assignments = token;
    }
  }

  //get list of students with grades
  char* students;
  if(R->numStudents != 0){
    token = strtok(NULL, "-");
    if(token == NULL && R->numStudents != 0){
      printf("get gradebook parse error4\n");
      handleErrors();
    }else{
      students = token;
    }
  }

  //parse the assignments stirng
  token = NULL;
  int assignNum = 1;
  char* start, *end;
  Assignment* prevA;
  if(R->numAssignments != 0){
    token = strtok(assignments, ";");
  }
  while( token != NULL ) {
    Assignment* A = calloc(1, sizeof(Assignment));
    A->assignmentNumber = assignNum;
    //find the assignment name and copy it into A
    start = token;
    end = strchr(start, ' ');
    A->assignmentName = calloc(1, end - start +1);
    memcpy(A->assignmentName, start, end-start);

    //find the points associated with the assignment and add it to A
    start = end+1;
    end = strchr(start, ' ');
    A->points = atoi(start);
    
    //find weight associated with the assignment and add it to A
    A->weight = atof(end+1);
    
    if(assignNum == 1){
      R->assignments = A;
      prevA = A;
    }else{
      prevA->next = A;
      prevA = A;
    }    
    token = strtok(NULL, ";");
    assignNum++;
  }

  //parse the students string
  token = NULL;
  int studNum = 1;
  Student* prevS;
  if(R->numStudents != 0){
    token = strtok(students, ";");
  }
  while( token != NULL ) {
    Student* S = calloc(1, sizeof(Student));

    //find first name and place in S
    start = token;
    end = strchr(start, ' ');
    S->firstName = calloc(1, end - start +1);
    memcpy(S->firstName, start, end-start);

    //find first name and place in S
    start = end+1;
    end = strchr(start, ' ');
    S->lastName = calloc(1, end - start +1);
    memcpy(S->lastName, start, end-start);

    //create linked list of grades
    if(R->numAssignments != 0){
      Grade* prevG;
      for(int i = 0; i < R->numAssignments; i++){
        start = end+1;
        end = strchr(start, ' ');
        Grade* tmp = calloc(1, sizeof(Grade));
        tmp->score = atoi(start);
        tmp->next = NULL;
        if(i == 0){
          S->grades = tmp;
          prevG = tmp;
        }else{
          prevG->next = tmp;
          prevG = tmp; 
        }           
      }
    }

    if(studNum == 1){
      R->students = S;
      prevS = S;
    }else{
      prevS->next = S;
      prevS = S;
    }
    
    token = strtok(NULL, ";");
    studNum++;
  }
  // printf("DONE parsing to a gradebook\n");
  return 1;
}

Gradebook* read_Gradebook_from_path(FILE* fp, unsigned char *key) {
  int valid = -1;
  Gradebook* gradebook = malloc(sizeof(Gradebook));

  //read in a file 
  Buffer*  B = read_from_path(fp, key);

  //create a Gradebook using the buffer
  valid = get_Gradebook(gradebook, *B);
  //can free all of B memory after this
  freeBuffer(B);
  if(valid != 1){
    printf("Failed creating a Gradebook\n");
    handleErrors();
  }
  return gradebook;
}

int checkFileName(char* name){
  if(name == NULL || *name == '\0'){
    printf("No Name Provided\n");
    return 0;
  } 
  char* tmp = name;
  while(*tmp != '\0'){
    char c = *tmp;
    //c is a number
    if(c >= 48 && c <= 57){
      tmp++;
    }
    //c is period or underscore
    else if(c == 46 || c == 95){
      tmp++;
    }
    //c is an uppercase letter
    else if(c >= 65 && c <= 90){
      tmp++;
    }
    //c is a lowercase letter
    else if(c >= 97 && c <= 122){
      tmp++;
    }
    //invalid character
    else{
      printf("Invalid Character(%c) Provided in Filename\n", c);
      return 0;
    }
  }
  return 1;
}

int checkAssignmentName(char* name){
  if(name == NULL || *name == '\0'){
    printf("No Name Provided\n");
    return 0;
  } 
  char* tmp = name;
  while(*tmp != '\0'){
    char c = *tmp;
    //c is a number
    if(c >= 48 && c <= 57){
      tmp++;
    }
    //c is an uppercase letter
    else if(c >= 65 && c <= 90){
      tmp++;
    }
    //c is a lowercase letter
    else if(c >= 97 && c <= 122){
      tmp++;
    }
    //invalid character
    else{
      printf("Invalid Character(%c) Provided in Filename\n", c);
      return 0;
    }
  }
  return 1;
}

int checkName(char* name){
  if(name == NULL || *name == '\0'){
    printf("No Name Provided\n");
    return 0;
  } 
  char* tmp = name;
  while(*tmp != '\0'){
    char c = *tmp;
    //c is an uppercase letter
    if(c >= 65 && c <= 90){
      tmp++;
    }
    //c is a lowercase letter
    else if(c >= 97 && c <= 122){
      tmp++;
    }
    //invalid character
    else{
      printf("Invalid Character(%c) Provided in Filename\n", c);
      return 0;
    }
  }
  return 1;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *tag, unsigned char **plaintext){
  EVP_CIPHER_CTX *ctx;
  *plaintext = calloc(1, sizeof(unsigned char)*(ciphertext_len+1));
  unsigned char* outbuf = *plaintext;
  int outlen, plaintext_len, ret;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())){
    printf("1\n");
    handleErrors();
  }

  /* Initialise the decryption operation. */
  if(!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)){
    printf("2\n");
    handleErrors();
  }

  /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
  if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL)){
    printf("3\n");
    handleErrors();
  }

  /* Initialise key and IV */
  if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)){
    printf("4\n");
    handleErrors();
  }

  /*
  * Provide the message to be decrypted, and obtain the plaintext output.
  * EVP_DecryptUpdate can be called multiple times if necessary
  */
  if(!EVP_DecryptUpdate(ctx, outbuf, &outlen, ciphertext, ciphertext_len)){
    printf("5\n");
    handleErrors();
  }
  plaintext_len = outlen;

  /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
  if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)){
    printf("6\n");
    handleErrors();
  }

  /*
  * Finalise the decryption. A positive return value indicates success,
  * anything else is a failure - the plaintext is not trustworthy.
  */
  ret = EVP_DecryptFinal_ex(ctx, outbuf + outlen, &outlen);

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);
  if(ret > 0) {
    /* Success */
    plaintext_len += outlen;
    return plaintext_len;
  } else {
    /* Verify failed */
    printf("Verify failed\n");
    handleErrors();
  }
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char **ciphertext) {
  //the +16+12 are to account for the tag and iv
  //the +1 is for a null byte and calloc initializes everything to null
  //added +50 for test purposes
  *ciphertext = calloc(1, sizeof(unsigned char)*(plaintext_len+16+12+1+50));
  unsigned char* tag = malloc(sizeof(unsigned char)*16);
  int outlen, ciphertext_len = 16+12;
  unsigned char* outbuf = *ciphertext+16+12;
  EVP_CIPHER_CTX *ctx;

  // Create and initialise the context 
  if(!(ctx = EVP_CIPHER_CTX_new())){
    printf("1\n");
    handleErrors();
  }

  // Initialise the encryption operation. 
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)){
    printf("2\n");
    handleErrors();
  }

  //Set IV length if default 12 bytes (96 bits) is not appropriate
  if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL)){
    printf("3\n");
    handleErrors();
  }

  // Initialise key and IV 
  if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)){
    printf("4\n");
    handleErrors();
  }
  /*
   * Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
  */
  if(1 != EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, plaintext_len)){
    printf("5\n");
    handleErrors();
  }
  ciphertext_len += outlen;

  /*
  * Finalise the encryption. Normally ciphertext bytes may be written at
  * this stage, but this does not occur in GCM mode
  */
  if(1 != EVP_EncryptFinal_ex(ctx, outbuf + outlen, &outlen)){
    printf("6\n");
    handleErrors();
  }
  ciphertext_len += outlen;

  /* Get the tag */
  if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)){
    printf("7\n");
    handleErrors();
  }

  //write the tag and the iv since they don't need to be encrypted then the encrypted text
  memcpy(*ciphertext, tag, 16);
  memcpy(*ciphertext+16, iv, 12);

  EVP_CIPHER_CTX_free(ctx);
  return ciphertext_len;
}

int findAssignmentName(char* assignmentName, Gradebook G){
  Assignment* curr = G.assignments;

  while(curr != NULL){
    if(strcmp(curr->assignmentName, assignmentName) == 0){
      return curr->assignmentNumber;
    }
    curr = curr->next;
  }

  return -1;
}

int findStudentName(char *firstName, char *lastName, Gradebook G){
  Student* curr = G.students;
  int i = 1;
  while(curr != NULL){
    if(strcmp(firstName, curr->firstName) == 0 && strcmp(lastName, curr->lastName) == 0){
      return i;
    }
    i++;
    curr = curr->next;
  }
  return -1;
}
