#include <ctype.h>
#include <string.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "gradebook.h"
#include "gradebook_crypto.h"

/* Initializing the encryption function */
int gradebook_encrypt(gradebook_t *gradebook, uint8_t *key, uint8_t *iv,
                      int iv_len, uint8_t *ciphertext, uint8_t *tag) {

	EVP_CIPHER_CTX *ctx;
	int len;
	int ciphertext_len;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new()))
		handleErrors();

	/* 
	* Initialise the encryption operation. 
	* Note - you need to choose the encryption method based on the attacks you demonstrate. 
	*/
	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, NULL, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
		handleErrors();

	/*
	 * Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, (uint8_t *)gradebook, sizeof(gradebook_t)))
		handleErrors();
	ciphertext_len = len;

	/* Get the tag */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
		handleErrors();

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}