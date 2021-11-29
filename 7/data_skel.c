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
#include <openssl/conf.h>

#define NON_VAR_LENGTH 0     //TODO change me
#define LEN 16
#define IV_LEN 7
#define TAG_LEN 14




//taken from documentation here: https://wiki.openssl.org/index.php/EVP_Authenticated_Encryption_and_Decryption
int ccm_encrypt(unsigned char *plaintext, int plaintext_len,
                unsigned char *aad, int aad_len,
                unsigned char *key,
                unsigned char *iv,
                unsigned char *ciphertext,
                unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    //////printf("in encrypt\n");
    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        return -1;

    /* Initialise the encryption operation. */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ccm(), NULL, NULL, NULL))
        return -1;

    //////printf("initialized\n");
    /*
     * Setting IV len to 8.
     */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, IV_LEN, NULL))
        return -1;
    //////printf("set iv len\n");
    /* Set tag length */
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, TAG_LEN, NULL);
    //////printf("set tag len\n");
    /* Initialise key and IV */
    if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
        return -1;
    //////printf("initialized key and iv. ");
    /* Provide the total plaintext length */
    if(1 != EVP_EncryptUpdate(ctx, NULL, &len, NULL, plaintext_len))
        return -1;
    //////printf("provided plaintext len\n");
    /* Provide any AAD data. This can be called zero or one times as required */
    if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
        return -1;
    //////printf("provided aad\n");
    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can only be called once for this.
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        return -1;
    ciphertext_len = len;
    //////printf("provided plaintext\n");
    /*
     * Finalise the encryption. Normally ciphertext bytes may be written at
     * this stage, but this does not occur in CCM mode.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        return -1;
    ciphertext_len += len;
    //////printf("finalized encryption\n");
    /* Get the tag */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_GET_TAG, TAG_LEN, tag))
        return -1;

    //////printf("got tag\n");
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}
// also taken from documentation here: https://wiki.openssl.org/index.php/EVP_Authenticated_Encryption_and_Decryption
int ccm_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *aad, int aad_len,
                unsigned char *tag,
                unsigned char *key,
                unsigned char *iv,
                unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        return -1;
    //////printf("initialized\n");
    /* Initialise the decryption operation. */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ccm(), NULL, NULL, NULL))
        return -1;

    /* Setting IV len to 8 */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, IV_LEN, NULL))
        return -1;

    //////printf("set iv len\n");
    /* Set expected tag value. */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, TAG_LEN, tag))
        return -1;

    /* Initialise key and IV */
    if(1 != EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
        return -1;

    //////printf("initialized key and iv");
    /* Provide the total ciphertext length */
    if(1 != EVP_DecryptUpdate(ctx, NULL, &len, NULL, ciphertext_len))
        return -1;


    //////printf("total cipherlen\n");
    /* Provide any AAD data. This can be called zero or more times as required */
    if(1 != EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
        return -1;

    //////printf("provided aad\n");
    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    ret = EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);

    //////printf("decrypted\n");
    plaintext_len = len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    if(ret > 0) {
        /* Success */
        return plaintext_len;
    } else {
        /* Verify failed */
        //////printf("verify failed\n");
        return -1;
    }
}
int compute_Gradebook_size(Gradebook *R) {
  unsigned int i,j,total_assignment_size = 0, total_student_size = 0, gradebook_size = 0;

  ////printf("in compute Gradebook\n");
  ////printf("num assignments: %d\n", R->num_assignments);
  for(i=0; i<R->num_assignments; i++){
    ////printf("in the loop\n");
    //////printf("name len: %d\n", R->assignments[i]->name_len);
    total_assignment_size += 3*sizeof(unsigned int);
    total_assignment_size += sizeof(double); //every assignment has 3 ints and a double, (id, points, namelen +  weights).
    ////printf("added to total assignment size\n");
    total_assignment_size += sizeof(char)*R->assignments[i]->name_len;

  }

  for(j=0; j< R->num_students; j++){
    //every student has 2 unsigned ints for name_len + grade.
    ////printf("in students loop\n");
    total_student_size += 2*sizeof(unsigned int); //name_len, num_grades,
    total_student_size += sizeof(char)*R->students[j]->name_len; //add sizeof name.

    total_student_size += 2*sizeof(unsigned int)*R->students[j]->num_grades; //2 arrays of the same length.
  }

  gradebook_size = 3*sizeof(unsigned int) + total_assignment_size + total_student_size;
  //gradebook has 3 unsigned ints and 2 arrays of assignments + students.
  return gradebook_size;
}

Buffer print_Gradebook(Gradebook *R) {
  Buffer  B = {0};

  //TODO Code this

  return B;
}

//produce A | B
Buffer concat_buffs(Buffer *A, Buffer *B) {
  Buffer  C = {0};
  //TODO Code this
  return C;
}

void write_to_path(char *path, Buffer *B, unsigned char *key_data) {
  unsigned char iv[IV_LEN],tag[TAG_LEN];
  FILE * random, * fp;
  int cipher_len;
  unsigned char * ciphertext;
  //create place for ciphertext.

  ciphertext = (unsigned char *) malloc(B->Length*sizeof(unsigned char));


  //randomly generate IV.
  random = fopen("/dev/urandom", "r");
  fread(iv, sizeof(unsigned char)*IV_LEN, 1, random);
  //iv[IV_LEN] = '\0'; //null terminate iv.
  //encrypt the buffer with IV.
  cipher_len = ccm_encrypt(B->Buf, B->Length, iv, IV_LEN, key_data, iv, ciphertext, tag);
  //////printf("Printing Ciphertext and Tag.\n");
  int i =0;
  //////printf("Ciphertext: ");

  if(cipher_len == -1){
    ////printf("invalid\n");
    return;
  }

  //print stuff for debugging.
  /*while(i < cipher_len){
    ////printf("%.2x", ciphertext[i]);
    i++;
  }
  i =0;
  ////printf("\nTag:");
  while(i < TAG_LEN){
      ////printf("%.2x", tag[i]);
      i++;
  }
  i = 0;
  ////printf("\nIV:");
  while(i < IV_LEN){
    ////printf("%.2x", iv[i]);
    i++;
  }
  ////printf("\n");
  ////printf("Length Ciphertext: %d\n", cipher_len);

  */
  //write IV to path.
  fp = fopen(path, "wb");
  fwrite(iv, sizeof(unsigned char)*IV_LEN, 1, fp);
  //write the tag to path.
  fwrite(tag, sizeof(unsigned char)*TAG_LEN, 1, fp);
  //write encrypted gradebook (ciphertext) to path as bytes.
  fwrite(ciphertext, sizeof(unsigned char)*cipher_len, 1, fp);

  //close file pointers.
  fclose(random);
  fclose(fp);

  //free memory
  free(ciphertext);
  return;
}

Buffer read_from_path(char *path, unsigned char *key_data) {
  Buffer  B = {0};
  FILE * fp;
  unsigned char iv[IV_LEN], tag[TAG_LEN], *ciphertext, *plaintext;
  int i, plaintext_len;
  //first 128 bits (16 bytes) is IV (randomly generated);
  fp = fopen(path, "rb");
  fseek(fp, 0, SEEK_END);
  B.Length = ftell(fp) - (IV_LEN -1) - (LEN-1); //length of the file - IV - Tag. (size of actual struct data)

  rewind(fp); //rewind the file.

  if(B.Length >= 0){ //check that the file length is at least as much as IV+taglen (subtracted LEN and IV_LEN earlier)
    fread(iv, sizeof(unsigned char), IV_LEN, fp); //read IV from the file.

    //print iv for debug.
    /*////printf("\nRetrieved IV: ");
    for(i=0; i < IV_LEN; i++){
      ////printf("%.2x", iv[i]);
    }
    ////printf("\n");
    */
    fread(tag, sizeof(unsigned char), TAG_LEN, fp);

    //print tag for debug.
    /*////printf("\nRetrieved Tag: ");
    for(i=0; i < TAG_LEN; i++){
      ////printf("%.2x", tag[i]);
    }

    ////printf("\n");
    */
    //make a spot for the ciphertext and plaintext.
    ciphertext = (unsigned char *) malloc(sizeof(unsigned char)*B.Length);
    plaintext = (unsigned char *) malloc(sizeof(unsigned char)*B.Length);

    //read in ciphertext.
    fread(ciphertext, sizeof(unsigned char), B.Length, fp);
    i = 0;
    /*////printf("Length Ciphertext: %d\n", (int) B.Length);
    ////printf("CipherText: \n");
    while(i < B.Length){
      ////printf("%.2x", ciphertext[i]);
      i++;
    }
    ////printf("\n\n");
    */
    //Use IV, Key and Tag from file to decrypt + verify.
    plaintext_len = ccm_decrypt(ciphertext, B.Length, iv, IV_LEN, tag, key_data, iv, plaintext);

    if(plaintext_len == -1){
      //verify failed.
      B.Length = -1;
    } else {
      //print plaintext for debug.
      //printf("%d\n", plaintext_len);
      //printf("Plaintext: \n");
      for(i=0; i < plaintext_len; i++){
        //printf("%.2x", plaintext[i]);
      }
      //printf("\n");

      B.Buf = plaintext; //assign the buffer to point to the plaintext.
    }

  } else {
    B.Length = -1;
  }

  //close file pointers.
  fclose(fp);

  //free memory.
  free(ciphertext);

  //return the buffer.

  return B;
}

void dump_assignment(Assignment *A) {

  //TODO Code this

  return;
}

int get_Gradebook(Gradebook **R, Buffer *B) {
  unsigned int  bytesRead = 0;
  unsigned int i = 0, j=0,k=0;
  unsigned char * ptr = B->Buf; //start at beginning of buffer.

  *R = (Gradebook *) malloc(sizeof(Gradebook)); //allocate a gradebook.
  //get number of assignments.

  (*R)->num_assignments = *(unsigned int *)(ptr+bytesRead);
  bytesRead += sizeof(unsigned int); //keep track of the number of bytes we have read.
  //allocate space for assignments.
  //printf("Num assignments:  %d\n", (*R)->num_assignments);
  //get next id.
  (*R)->next_id = *(unsigned int *)(ptr+bytesRead);
  bytesRead += sizeof(unsigned int);
  //printf("next id :  %d\n", (*R)->next_id);
  //get number of students.
  (*R)->num_students = *(unsigned int *)(ptr+bytesRead);
  bytesRead += sizeof(unsigned int);
  //printf("Num students:  %d\n", (*R)->num_students);

  //allocate space for assignments if they are any.
  if((*R)->num_assignments == 0){
    (*R)->assignments = NULL;
  } else {
    (*R)->assignments =(Assignment **) malloc(sizeof(Assignment *)*((*R)->num_assignments));
    //iterate through the assignments and read from file.
    for(i = 0; i < (*R)->num_assignments; i++){
      //malloc space for this assignment.
      //////printf("in loop");
      (*R)->assignments[i]= (Assignment *) malloc(sizeof(Assignment));
      //read in arguments.
      //////printf("malloced space for assignment %d\n", i);
      //id
      //printf("Printing Buf\n");
      (*R)->assignments[i]->id = *(unsigned int *)(ptr+bytesRead);
      //printf("%d\n", (*R)->assignments[i]->id);
      bytesRead += sizeof(unsigned int);

      //points
      (*R)->assignments[i]->points = *(unsigned int *)(ptr+bytesRead);
      ////printf("%d\n", (*R)->assignments[i]->points);
      bytesRead += sizeof(unsigned int);

      //weight
      (*R)->assignments[i]->weight = *(double *)(ptr+bytesRead);
      ////printf("%f\n", (*R)->assignments[i]->weight);
      bytesRead += sizeof(double);

      //name length
      (*R)->assignments[i]->name_len = *(unsigned int *)(ptr+bytesRead);
      ////printf("%d\n", (*R)->assignments[i]->name_len);
      bytesRead += sizeof(unsigned int);

      //name. allocate space for it.
      (*R)->assignments[i]->name = (char *) malloc(1+sizeof(char)*(*R)->assignments[i]->name_len);
      strncpy((*R)->assignments[i]->name, (char *)(ptr+bytesRead), (*R)->assignments[i]->name_len);
      (*R)->assignments[i]->name[(*R)->assignments[i]->name_len] = '\0'; //null terminate string.
      //printf("%s\n", (*R)->assignments[i]->name);

      //////printf("\n");
      bytesRead += sizeof(char)*(*R)->assignments[i]->name_len;


    }
  }


  //allocate space for students if we need to.
  if((*R)->num_students == 0){
    (*R)->students = NULL;
  } else {
    (*R)->students = (Student **) malloc(sizeof(Student *)*((*R)->num_students));
    for(j=0; j < (*R)->num_students; j++){
      //iterate through students.

      //allocate space for student
      (*R)->students[j] = (Student *) malloc(sizeof(Student));
      //printf("allocating student space\n");
      //read from buffer name length
      (*R)->students[j]->name_len = *(unsigned int *)(ptr+bytesRead);
      bytesRead += sizeof(unsigned int);

      //num_grades
      (*R)->students[j]->num_grades = *(unsigned int *)(ptr+bytesRead);
      bytesRead += sizeof(unsigned int);
      //printf("num grades: %d\n", (*R)->students[j]->num_grades);
      //allocate space for student name.
      (*R)->students[j]->name = (char *) malloc(1+ sizeof(char)*(*R)->students[j]->name_len);
      //printf("allocating space for name\n");
      //copy the name.
      strncpy((*R)->students[j]->name, (ptr+bytesRead), (*R)->students[j]->name_len);
      (*R)->students[j]->name[(*R)->students[j]->name_len] = '\0'; //end string name correctly.

      //printf("copying name\n");
      bytesRead += sizeof(char)*((*R)->students[j]->name_len);

      if((*R)->students[j]->num_grades == 0){
        (*R)->students[j]->assignment_ids = NULL;
        (*R)->students[j]->grades = NULL;
      } else {
        //allocate space for the assignment ids + grades.
        (*R)->students[j]->assignment_ids = (unsigned int *) malloc(sizeof(unsigned int)*((*R)->students[j]->num_grades));
        (*R)->students[j]->grades = (unsigned int *) malloc(sizeof(unsigned int)*((*R)->students[j]->num_grades));
        //printf("allocated bytes for arrays\n");

        //read the assignment ids from buffer and then the grades.
        //printf("reading bytes for students\n");
        for(i = 0; i < (*R)->students[j]->num_grades; i++){
          (*R)->students[j]->assignment_ids[i] = *(unsigned int *)(ptr+bytesRead);
          bytesRead += sizeof(unsigned int);
        }

        for(i = 0; i < (*R)->students[j]->num_grades; i++){
          (*R)->students[j]->grades[i] = *(unsigned int *)(ptr+bytesRead);
          bytesRead += sizeof(unsigned int);
        }
      }

    }
  }



  return bytesRead;
}


//read a gradebook from the path.
int read_Gradebook_from_path(char *path, unsigned char *key, Gradebook **outbuf, unsigned int *outnum) {
  *outnum = 0;
  *outbuf = NULL;

  //read in a file
  Buffer  B = read_from_path(path, key);
  if(B.Length == -1){
    return -1;
  }

  *outnum = get_Gradebook(outbuf, &B);

  return 0;
}

//write a gradebook to the particular path.
int write_Gradebook_to_path(char *path, unsigned char* key, Gradebook * G){
  unsigned int gradebook_size, bytesRead = 0,i = 0, k=0, j=0;
  Buffer B = {0};
  unsigned char * ptr;

  /*typedef struct _Student {
    unsigned int name_len;
    unsigned int grade;
    char * name;
  } Student;

  typedef struct _Assignment {
    //put some things here
    unsigned int num_students; //each assignment tracks the number of students and two arrays of length num
    //student if the student doesn't have a grade for the assignment it's marked as '-' in the grades array.
    unsigned int points; //number of points its out of.
    double weight; //weight of this assignment.
    Student ** students;
  } Assignment;

  typedef struct _Gradebook {
    //put some things here
    unsigned int num_assignments;
    Assignment ** assignments;
  } Gradebook;
  */
  //////printf("here\n");
  B.Length = compute_Gradebook_size(G);
//  ////printf("computed gradebook size\n");
  B.Buf = (unsigned char * ) malloc(sizeof(unsigned char)*B.Length);
  ptr = B.Buf;

  //////printf("did variables\n");

  //memcpy things over from Gradebook to buffer.
  //num assignments.
  memcpy((ptr+bytesRead), (unsigned char *)&(G->num_assignments), sizeof(unsigned int));
  bytesRead += sizeof(unsigned int);

  //next_id
  memcpy((ptr+bytesRead), (unsigned char *)&(G->next_id), sizeof(unsigned int));
  bytesRead += sizeof(unsigned int);

  //num_students
  memcpy((ptr+bytesRead), (unsigned char *)&(G->num_students), sizeof(unsigned int));
  bytesRead += sizeof(unsigned int);

  //////printf("copied num assignment.\n");
  for(i=0; i < G->num_assignments; i++){
    //copy the assignment num students, points, and weights, name length and name.

    //////printf("Printing Buf:\n");
    //id
    memcpy((ptr+bytesRead), (unsigned char *)&(G->assignments[i]->id), sizeof(unsigned int));
    //////printf("%d", *(unsigned int *)(ptr+bytesRead));
    bytesRead += sizeof(unsigned int);

    //points
    memcpy((ptr+bytesRead), (unsigned char *)&(G->assignments[i]->points), sizeof(unsigned int));
    //////printf("%d", *(unsigned int *)(ptr+bytesRead));
    bytesRead += sizeof(unsigned int);

    //weight
    memcpy((ptr+bytesRead), (unsigned char *)&(G->assignments[i]->weight), sizeof(double));
    //////printf("%f", *(double *)(ptr+bytesRead));
    bytesRead += sizeof(double);

    //name length
    memcpy((ptr+bytesRead), (unsigned char *)&(G->assignments[i]->name_len), sizeof(unsigned int));
    //////printf("%d", *(unsigned int *)(ptr+bytesRead));
    bytesRead += sizeof(unsigned int);

    //assignment name.
    memcpy((ptr+bytesRead), (unsigned char *)(G->assignments[i]->name), sizeof(char)*G->assignments[i]->name_len);

    bytesRead += sizeof(char)*G->assignments[i]->name_len;



  }


  //copy over the students
  //////printf("copied data for assignment %d\n", i);
  for(j=0; j < G->num_students; j++){

    //////printf("copied data for student %d\n", j);
    //copy the student name length.
    memcpy((ptr+bytesRead), (unsigned char *)&(G->students[j]->name_len), sizeof(unsigned int));
    bytesRead += sizeof(unsigned int);

    //student num_grades.
    memcpy((ptr+bytesRead), (unsigned char *)&(G->students[j]->num_grades), sizeof(unsigned int));
    bytesRead += sizeof(unsigned int);

    //copy the name.
    memcpy((ptr+bytesRead), (unsigned char *)(G->students[j]->name), sizeof(char)*G->students[j]->name_len);
    bytesRead += sizeof(char)*G->students[j]->name_len;

    //copy the assignment_ids array.
    memcpy((ptr+bytesRead), (unsigned char *)(G->students[j]->assignment_ids), sizeof(unsigned int)*G->students[j]->num_grades);
    bytesRead += sizeof(unsigned int)*G->students[j]->num_grades;

    //copy the grades array.
    memcpy((ptr+bytesRead), (unsigned char *)(G->students[j]->grades), sizeof(unsigned int)*G->students[j]->num_grades);
    bytesRead += sizeof(unsigned int)*G->students[j]->num_grades;
  }

  //after transferring contents to buffer, write the buffer to file.
  write_to_path(path, &B, key);



  //free the buffer after writing.
  free(B.Buf);
  return 0;
}
