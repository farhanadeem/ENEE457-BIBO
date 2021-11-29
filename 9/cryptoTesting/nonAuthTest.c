#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <string.h>

void handleErrors(){
    printf("Error");
    exit(255);
}

int main(int argc, char* argv[]){
    unsigned char inbuf[1024], outbuf[1024 + EVP_MAX_BLOCK_LENGTH];
    unsigned char cGen[1024 + EVP_MAX_BLOCK_LENGTH];
    char line[30];
    int inlen, outlen, clen, cGenlen = 0, q = 1;
    EVP_CIPHER_CTX *ctx;
    unsigned char key[] = "0123456789abcdef";
    unsigned char iv[] = "fedcba9876543210";
    //blank out outbuf and cGen 
    for(int i = 0; i < 1024 + EVP_MAX_BLOCK_LENGTH; i++){
        cGen[i] = '\0';
        outbuf[i] = '\0';
    }
    /***************************************
     * ENCRYPTING TESTING
     * ***************************************/
    if (argc > 1){
        FILE *in;
        in = fopen("in.txt", "r");
        FILE *cipherOut;
        cipherOut = fopen("cipherOut.txt", "w");
        // Don't set key or IV right away; we want to check lengths 
        if(!(ctx = EVP_CIPHER_CTX_new()))
            handleErrors();
        EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, NULL, NULL, 1);
        OPENSSL_assert(EVP_CIPHER_CTX_key_length(ctx) == 16);
        OPENSSL_assert(EVP_CIPHER_CTX_iv_length(ctx) == 16);
        EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv, 1);

        inlen = fread(inbuf, 1, 1024, in);
        printf("*%s*\n", inbuf);
        if (!EVP_CipherUpdate(ctx, outbuf, &outlen, inbuf, inlen)) {
            // Error
            printf("Error enc1");
            EVP_CIPHER_CTX_free(ctx);
            return 0;
        }
        fwrite(outbuf, 1, outlen, cipherOut);

        if (!EVP_CipherFinal_ex(ctx, outbuf, &outlen)) {
            // Error 
            printf("Error enc2");
            EVP_CIPHER_CTX_free(ctx);
            return 0;
        }
        fwrite(outbuf, 1, outlen, cipherOut);
        EVP_CIPHER_CTX_free(ctx);
        fclose(in);
        fclose(cipherOut);
    } else{
        /**************************************
         * DECRYPTING
         ***************************************/
        FILE *cipherIn;
        cipherIn = fopen("cipherOut.txt", "r");
        FILE *out;
        out = fopen("out.txt", "w");
        // Don't set key or IV right away; we want to check lengths 
        ctx = EVP_CIPHER_CTX_new();
        EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv, 0);

        inlen = fread(inbuf, 1, 1024, cipherIn);
        printf("*%s*\n", inbuf);
        if (!EVP_CipherUpdate(ctx, outbuf, &outlen, inbuf, inlen)) {
            // Error
            printf("Error dec1");
            EVP_CIPHER_CTX_free(ctx);
            return 0;
        }
        fwrite(outbuf, 1, outlen, out);

        if (!EVP_CipherFinal_ex(ctx, outbuf, &outlen)) {
            // Error 
            printf("Error dec2");
            EVP_CIPHER_CTX_free(ctx);
            return 0;
        }
        fwrite(outbuf, 1, outlen, out);
        EVP_CIPHER_CTX_free(ctx);
        fclose(cipherIn);
        fclose(out);
    }
}