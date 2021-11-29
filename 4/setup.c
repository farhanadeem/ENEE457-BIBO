#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <sqlite3.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "functions.h"

//Function to provide initial file encryption and output the key
void encrypt2(char *fp)
{
    FILE *ifp = fopen(fp, "r");

    //Get file size
    fseek(ifp, 0L, SEEK_END);
    int fsize = ftell(ifp);
    //set back to normal
    fseek(ifp, 0L, SEEK_SET);
    unsigned char tag[16];
    int outLen1 = 0;
    int outLen2 = 0;
    unsigned char *indata = malloc(fsize);
    unsigned char *ran = (unsigned char *)malloc(sizeof(unsigned char) * 48);
    unsigned char ckey[32];
    unsigned char ivec[16];

    //Get the random bytes for the key and iv
    FILE *random = fopen("/dev/urandom", "r");
    fread(ran, sizeof(unsigned char) * 48, 1, random);
    fclose(random);
    memcpy(ckey, &ran[0], 32);
    memcpy(ivec, &ran[32], 16);
    for (int i = 0; i < 32; ++i)
    {
        printf("%02x", ckey[i]);
    }
    printf("\n");
    unsigned char *outdata = malloc(fsize * 2);

    //Read File
    fread(indata, sizeof(char), fsize, ifp); //Read Entire File
    fclose(ifp);
    FILE *ofp = fopen(fp, "w");

    //Set up encryption
    EVP_CIPHER_CTX *ctx;
    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        handleErrors();
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
    {
        handleErrors();
    }

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
    {
        handleErrors();
    }

    if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, ckey, ivec))
    {
        handleErrors();
    }

    if (1 != EVP_EncryptUpdate(ctx, NULL, &outLen1, ivec, 16))
    {
        handleErrors();
    }

    if (1 != EVP_EncryptUpdate(ctx, outdata, &outLen1, indata, fsize))
    {
        handleErrors();
    }

    if (1 != EVP_EncryptFinal(ctx, outdata + outLen1, &outLen2))
    {
        handleErrors();
    }

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
    {
        handleErrors();
    }

    //Write the encrypted data to the file
    fwrite(outdata, sizeof(char), outLen1 + outLen2, ofp);
    fclose(ofp);

    //Write the iv and tag to the file
    ofp = fopen(fp, "a");
    fwrite(ivec, sizeof(char), 16, ofp);
    fwrite(tag, sizeof(char), 16, ofp);
    EVP_CIPHER_CTX_free(ctx);
    fclose(ofp);
    free(ran);

    free(indata);
    free(outdata);
}

int main(int argc, char **argv)
{
    char *key = NULL;
    int rc;
    char *err_msg = 0;
    FILE *fp;
    sqlite3 *db;

    //Do argument checking
    if (argc != 3)
    {
        printf("invalid\n");
        return (255);
    }
    if (strcmp(argv[1], "-N") != 0)
    {
        printf("invalid\n");
        return (255);
    }
    if (file_test(argv[2]))
    {
        printf("invalid\n");
        return (255);
    }
    else
    {
        rc = sqlite3_open(argv[2], &db);
        if (rc)
        {
            printf("invalid\n");
            sqlite3_close(db);
            return (1);
        }
    }

    //SQL queries for initial setup
    char *sql =
        "CREATE TABLE Students(StudentID INTEGER PRIMARY KEY ASC, FirstName TEXT, LastName TEXT, UNIQUE(FirstName,LastName));" //Table for student records, gurantees unique first and last name pairing
        "CREATE TABLE Assignments(AssignmentID INTEGER PRIMARY KEY ASC, Name TEXT UNIQUE, Points INT, Weight REAL);" //Table for assignement records, gurantees assignment names are unique
        "CREATE TABLE StudentScores(StudentID INT, AssignmentID INT, Points INT, PRIMARY KEY (StudentID, AssignmentID), CONSTRAINT aDelete FOREIGN KEY (AssignmentID) REFERENCES Assignments(AssignmentID) ON DELETE CASCADE, CONSTRAINT sDelete FOREIGN KEY (StudentID) REFERENCES Students(StudentID) ON DELETE CASCADE);";
        //Table for grades for assignments, gurantees deletion of any student or assignment deletes all according records in the table
    
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {

        printf("invalid\n");
        sqlite3_close(db);

        sqlite3_free(err_msg);
        return 255;
    }
    
    sqlite3_close(db);

    encrypt2(argv[2]); //Encrypt the now created table
    return (0);
}

