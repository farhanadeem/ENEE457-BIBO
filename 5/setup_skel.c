#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <ctype.h>
#include <stdbool.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

#define DEBUG
/*
// test whether the file exists 
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}
*/

int main(int argc, char** argv) {
	EncryptedGradebook enc_gradebook = {0};
	DecryptedGradebook dec_gradebook = {0};
	FILE *fp;
	int i, ret;

       	if (strncmp(argv[1], "-N", 3)) {
		#ifdef DEBUG
	        printf("Second Argument should be '-N' \n");
		#endif
		printf("Invalid\n");
		exit(INVALID);
	}
		
	if (argc != 3) {
		#ifdef DEBUG
		printf("Usage: Setup Option Gradebook_name\n");
		#endif
		printf("Invalid\n");
		exit(INVALID);
	}


	for (i=0; i < strnlen(argv[2], MAX_USER_INPUT_LEN); i++) {
	  if (!(isalnum(argv[2][i]) || argv[2][i] == '.' || argv[2][i] == '_')) {
  		#ifdef DEBUG
			printf("Invalid filename\n");
			#endif
			printf("Invalid\n");
			exit(INVALID);
  	}
  
  	if (!access(argv[2], F_OK)) {
			#ifdef DEBUG
			printf("Gradebook Exists Already\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
	}

	unsigned char key[KEY_SIZE];

	ret = RAND_bytes(key, sizeof(key));
	if (ret == -1) {
		#ifdef DEBUG
		printf("Random key generation failed\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	printf("Random Key for setup is: ");
	for (i=0; i < KEY_SIZE; i++) {
		printf("%02X", key[i]);
	}
	printf("\n");


	unsigned char iv[IV_SIZE];

	ret = RAND_bytes(iv, sizeof(iv));
	if (ret == -1) {
		#ifdef DEBUG
		printf("Random IV generation failed\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}


	memcpy(enc_gradebook.iv, iv, sizeof(iv));
	ret = encrypt((unsigned char *)&dec_gradebook.num_assignments, sizeof(DecryptedGradebook), key, iv, (unsigned char *)&enc_gradebook.encrypted_data);
	if (ret == -1) {
		#ifdef DEBUG
		printf("Failed Encryption\n");
		#endif
		printf("Invalid\n");
		exit(INVALID);
	}
	
	unsigned char temp[MAC_SIZE];
	unsigned char* mac_res; 
	mac_res = HMAC(EVP_md5(), key, sizeof(key), (unsigned char *)&enc_gradebook.iv, sizeof(EncryptedGradebookSize), temp, NULL);


	memcpy(enc_gradebook.tag, temp, sizeof(temp));
	

	fp = fopen(argv[2], "w");
	if (fp == NULL) {
		#ifdef DEBUG
		printf("Couldn't create file\n");
		#endif
		printf("Invalid\n");
		exit(INVALID);
	}
	

	fwrite(enc_gradebook.iv, 1, sizeof(EncryptedGradebook), fp);
	fflush(fp);
	fclose(fp);
	return 0;
}

