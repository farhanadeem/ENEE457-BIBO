#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
//#include "data.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#define DEBUG
#define LEN 16

typedef struct _Assignment Assignment;
typedef struct _Student Student;
typedef struct _Gradebook {
    char name[100];
    Assignment *assignments;
    Student *stu;
    int num_as;
    int num_stu;
} Gradebook;
typedef struct _Assignment {
    char name[100];
    int point;
    float weight;
    Assignment *next_as;
    Student *stu;
} Assignment;

typedef struct _Student {
    char fn[100];
    char ln[100];
    int grade;
    Student *next_stu;
} Student;

void handleErrors(void)
{
    //ERR_print_errors_fp(stderr);
    printf("Error. The key is incorrect\n");
    exit(255);
    //abort();
}

int checkString(char str1[]) {
    int i, x=0, p;
    p=strlen(str1);

    for (i = 0; i < p ; i++)
        if (isalpha(str1[i]) || str1[i]=='_' || str1[i]=='.')
            continue;
        else return 0;

    return 1;
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext, unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
        handleErrors();
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
        handleErrors();
    if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors();     
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
        handleErrors();
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}



/* test whether the file exists */
int file_test(char* filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

/*IO functions*/
void store_gradebook(Gradebook *g) {
    g->assignments = NULL;
    g->stu = NULL;
    FILE *fp;
    fp=fopen(g->name,"wb");
    fwrite(g,sizeof(Gradebook),1,fp);
    fclose(fp);
}

#define LEN 16
int main(int argc, char** argv) {
    FILE *fp;
    //random key generation
    unsigned char *key = (unsigned char*) malloc(sizeof(unsigned char)*LEN);
    unsigned char *iv = (unsigned char*) malloc(sizeof(unsigned char)*LEN);
    unsigned char *tag = (unsigned char*) malloc(sizeof(unsigned char)*LEN);
    FILE *random = fopen("/dev/urandom", "r");
    fread(key, sizeof(unsigned char)*LEN, 1, random);
    fread(iv, sizeof(unsigned char)*LEN, 1, random);
    int i = 0;
    while((strlen(key)!=16 || strlen(iv)!=16)&& i <5){
    	fread(key, sizeof(unsigned char)*LEN, 1, random);
    	fread(iv, sizeof(unsigned char)*LEN, 1, random);
    	i++;
    }
    fclose(random);

    if (argc != 3 || strcmp(argv[1],"-N")!=0) {
        printf("Usage: setup -N <your gradebook name>(may not contain space)\n");
        return(255);
    }
//check whether the filename exists
    if (file_test(argv[2])) {
        printf("invalid\n");
        exit(255);
    }
//create gradebook and store plain gradebook
    Gradebook g = {"whatever",NULL,NULL,0,0};
    if(strlen(argv[2])>100){
            printf("Buffer overflow warning! Exiting.....\n");
            exit(255);
    }
    if(!checkString(argv[2])){
      printf("Name contains illegal characters.\n");
      exit(255);
    }
    strcpy(g.name,argv[2]);
    store_gradebook(&g);
// find file sie
    fp = fopen(argv[2], "rb");
    if (fp == NULL) {
        printf("setup: fopen() error could not create file\n");
        printf("invalid\n");
        return(255);
    }
    fseek(fp,0,SEEK_END);
    long fileSize =ftell(fp);
    rewind(fp);
//encrpytion
    unsigned char plain[fileSize+1];
    unsigned char ciphertext[fileSize+1];
    //read plaintext
    fread(plain, fileSize,1,fp);
    fclose(fp);
    long int ciphertext_len = encrypt (plain, fileSize, key, iv,ciphertext,tag);
// write cipherfile
    printf("Key is:");
    for(int i=0; i<strlen(key); i++) {
        printf("%02x",key[i]);
    }
    printf("\n");
    fp = fopen(argv[2], "wb");
    fwrite(iv, 16,1,fp);
    fwrite(tag, 16,1,fp);
    fwrite(ciphertext, ciphertext_len,1,fp);
    fclose(fp);
    free(key);

    return(0);

}
//gcc -o setup setup.c
