#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <sqlite3.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <ctype.h>
#include "functions.h"

//Function to handle crypto errors
void handleErrors(void)
{
    printf("invalid");
    exit(255);
}

//Function to convert string to byte array. Checks string has valid hex characters
unsigned char *datahex(char *string)
{

    if (string == NULL)
        return NULL;

    size_t slength = strlen(string);
    if ((slength % 2) != 0) // must be even
        return NULL;

    size_t dlength = slength / 2;

    unsigned char *data = malloc(dlength);
    memset(data, 0, dlength);

    size_t index = 0;
    while (index < slength)
    {
        char c = string[index];
        int value = 0;
        if (c >= '0' && c <= '9')
            value = (c - '0');
        else if (c >= 'A' && c <= 'F')
            value = (10 + (c - 'A'));
        else if (c >= 'a' && c <= 'f')
            value = (10 + (c - 'a'));
        else
        {
            free(data);
            return NULL;
        }

        data[(index / 2)] += value << (((index + 1) % 2) * 4);

        index++;
    }

    return data;
}

//The main encryption function, takes in a key and uses it to decrypt. Selects
//a new iv for the encryption, then writes the iv and tag appended to the file.
void encryptt(char *fp, char *ckey)
{
    FILE *ifp = fopen(fp, "r");

    //Get file size
    fseek(ifp, 0L, SEEK_END);
    int fsize = ftell(ifp);
    //set back to normal
    fseek(ifp, 0L, SEEK_SET);
    int outLen1 = 0;
    int outLen2 = 0;

    unsigned char *indata = malloc(fsize);
    unsigned char ivec[16];
    unsigned char tag[16];

    FILE *random = fopen("/dev/urandom", "r");
    fread(ivec, sizeof(unsigned char) * 16, 1, random);
    fclose(random);

    unsigned char *outdata = malloc(fsize * 2);

    fread(indata, sizeof(char), fsize, ifp); //Read Entire File to buffer
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

    fwrite(outdata, sizeof(char), outLen1 + outLen2, ofp); //Write encrypted data back to file
    fclose(ofp);

    //Append the iv and tag
    ofp = fopen(fp, "a");
    fwrite(ivec, sizeof(char), 16, ofp);
    fwrite(tag, sizeof(char), 16, ofp);

    EVP_CIPHER_CTX_free(ctx);
    fclose(ofp);

    free(indata);
    free(outdata);
}

//The decryption method takes in a provided key, and reads the iv and tag from the end of
//the encrypted file. The tag and iv are checked and the file is decrypted if they are valid.
int decryptt(char *fp, unsigned char *ckey)
{
    FILE *ifp = fopen(fp, "r");
    //Get file size
    fseek(ifp, 0L, SEEK_END);
    int fsize = ftell(ifp);
    //set back to normal
    fseek(ifp, 0L, SEEK_SET);

    int outLen1 = 0;
    int outLen2 = 0;
    unsigned char *indata = malloc(fsize);
    unsigned char *outdata = malloc((fsize)*2);
    unsigned char ivec[16];

    unsigned char tag[16];
    int ret;

    //Read File
    fread(indata, sizeof(char), fsize, ifp); //Read Entire File
    memcpy(ivec, &indata[fsize - 32], 16);
    memcpy(tag, &indata[fsize - 16], 16);

    fclose(ifp);

    //Set up decryption
    EVP_CIPHER_CTX *ctx;
    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        handleErrors();
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
    {
        handleErrors();
    }

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
    {
        handleErrors();
    }

    if (1 != EVP_DecryptInit_ex(ctx, NULL, NULL, ckey, ivec))
    {
        handleErrors();
    }

    if (1 != EVP_DecryptUpdate(ctx, NULL, &outLen1, ivec, 16))
    {
        handleErrors();
    }

    if (1 != EVP_DecryptUpdate(ctx, outdata, &outLen1, indata, fsize - 32))
    {
        handleErrors();
    }

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
    {
        handleErrors();
    }

    ret = EVP_DecryptFinal(ctx, outdata + outLen1, &outLen2);

    if (ret)
    {
        FILE *ofp = fopen(fp, "w");
        fwrite(outdata, sizeof(char), outLen1 + outLen2, ofp);
        fclose(ofp);
        EVP_CIPHER_CTX_free(ctx);

        free(indata);
        free(outdata);
        return 0;
    }
    else
    {
        printf("invalid\n");

        EVP_CIPHER_CTX_free(ctx);

        free(indata);
        free(outdata);
        return 1;
    }
}
//Checks if file with filename exists
int file_test(char *filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}
//Checks string to see if every character is alphanumeric
int check(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        //4
        if (!isalnum(str[i]))
        {
            return 0;
        }
    }
    return 1;
}
//Checks string to see if every character is alphabetic
int checkAlpha(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        //4
        if (!isalpha(str[i]))
        {
            return 0;
        }
    }
    return 1;
}
//Checks string to see if every character is numeric
int checkDigit(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        //4
        if (!isdigit(str[i]))
        {
            return 0;
        }
    }
    return 1;
}
//Checks string to see if it is a valid float
int checkFloat(char *str)
{
    int hasPeriod = 0;

    for (int i = 1; str[i] != '\0'; i++)
    {
        if (str[i] == '.' && !hasPeriod)
        {
            hasPeriod = 1;
        }
        else if (str[i] == '.' && hasPeriod)
        {
            return 0;
        }
        else if (!isdigit(str[i]))
        {
            return 0;
        }
    }
    return 1;
}
