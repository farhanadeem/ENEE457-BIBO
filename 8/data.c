#include "data.h"

/************************************************
* 				Cryptogaphic Functions
*************************************************/

int handle_errors(EVP_CIPHER_CTX *ctx) 
{
    ERR_print_errors_fp(stderr);
	EVP_CIPHER_CTX_free(ctx);
	return -1;
}

int gcm_encrypt(unsigned char *plaintext, int plaintext_len,
                const unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *ciphertext,
                unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
		#ifdef DEBUG
			printf("CTX New Error\n");
		#endif
        return handle_errors(ctx);
	}

    /* Initialise the encryption operation. */
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
		#ifdef DEBUG
			printf("EncryptInit Error\n");
		#endif
        return handle_errors(ctx);
	}

    /*
     * Set IV length if default 12 bytes (96 bits) is not appropriate
     */
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL)) {
		#ifdef DEBUG
			printf("IV Length Error\n");
		#endif
        return handle_errors(ctx);
	}

    /* Initialise key and IV */
    if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) {
		#ifdef DEBUG
			printf("Key/IV Set Error\n");
		#endif
        return handle_errors(ctx);
	}

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
		#ifdef DEBUG
			printf("EncryptUpdate Error\n");
		#endif
        return handle_errors(ctx);
	}
    ciphertext_len = len;

    /*
     * Finalise the encryption. Normally ciphertext bytes may be written at
     * this stage, but this does not occur in GCM mode
     */
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
		#ifdef DEBUG
			printf("EncryptFinal Error\n");
		#endif
        return handle_errors(ctx);
	}
    ciphertext_len += len;

    /* Get the tag */
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LENGTH, tag)) {
		#ifdef DEBUG
			printf("Tag Get Error\n");
		#endif
        return handle_errors(ctx);
	}

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *tag,
                const unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx = NULL;
    int len = 0;
    int plaintext_len = 0;
    int ret = 0;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
		#ifdef DEBUG
			printf("CTX New Error\n");
		#endif
        return handle_errors(ctx);
	}

    /* Initialise the decryption operation. */
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
		#ifdef DEBUG
			printf("DecryptInit Error\n");
		#endif
        return handle_errors(ctx);
	}

    /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL)) {
		#ifdef DEBUG
			printf("IV Length Error\n");
		#endif
        return handle_errors(ctx);
	}

    /* Initialise key and IV */
    if (!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) {
		#ifdef DEBUG
			printf("Key/IV Set Error\n");
		#endif
        return handle_errors(ctx);
	}

	/* Set expected tag value. Works in OpenSSL 1.0.1d and later */
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LENGTH, tag)) {
		#ifdef DEBUG
			printf("Tag Set Error\n");
		#endif
        return handle_errors(ctx);
	}

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
		#ifdef DEBUG
			printf("DecryptUpdate Error\n");
		#endif
        return handle_errors(ctx);
	}
    plaintext_len = len;

    /*
     * Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    if(ret > 0) {
        /* Success */
        plaintext_len += len;
		EVP_CIPHER_CTX_free(ctx);
        return plaintext_len;
    } else {
		#ifdef DEBUG
			printf("DecryptFinal (probably verify) Error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		#endif
		EVP_CIPHER_CTX_free(ctx);
        /* Verify failed */
        return -1;
    }
}

/************************************************
* 				Key Functions
*************************************************/

void generate_key(unsigned char* key, int size) 
{
	FILE* random = fopen("/dev/urandom", "r");
	fread(key, sizeof(unsigned char), size, random);
	fclose(random);
}

void print_key(const unsigned char* key)
{
	int i = 0;

	printf("Gradebook key: ");
	// print key
	for (i = 0; i < KEY_LENGTH; i++) {
		printf("%02x", key[i]);
	}
	printf("\n");
}

/************************************************
* 				File I/O Functions
*************************************************/

int read_file(const unsigned char *key, Gradebook *gradebook) 
{
	FILE *fp = NULL;
	long file_length = 0;
	unsigned char *ciphertext = NULL;
	unsigned char tag[TAG_LENGTH];
	unsigned char iv[IV_LENGTH];
	unsigned char buffer[sizeof(Gradebook)];

	memset(tag, 0, TAG_LENGTH);
	memset(iv, 0, IV_LENGTH);
	memset(buffer, 0, sizeof(Gradebook));

	fp = fopen(gradebook->name, "rb");
	if (fp == NULL) {
		#ifdef DEBUG 
			printf("%s\n", gradebook->name);
			printf("Couldn't open file\n");
		#endif
		printf("invalid\n");
		return ERR;
	}

	// Determine the length of the encrypted file
	fseek(fp, 0, SEEK_END);
	file_length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ciphertext = (char *) malloc(file_length - TAG_LENGTH - IV_LENGTH);

	// Read from the file
	fread(tag, TAG_LENGTH, 1, fp);
	fread(iv, IV_LENGTH, 1, fp);
	fread(ciphertext, file_length - TAG_LENGTH - IV_LENGTH, 1, fp);

	if (gcm_decrypt(ciphertext, file_length - TAG_LENGTH - IV_LENGTH, 
			tag, key, iv, IV_LENGTH, buffer) != sizeof(Gradebook)) {
		#ifdef DEBUG
			printf("Decrypt failed\n");
		#endif
		free(ciphertext);
		fclose(fp);
		return ERR;
	} else {
		free(ciphertext);
		fclose(fp);
		memcpy(gradebook, buffer, sizeof(Gradebook));
		return 1;
	}
}

int write_file(const unsigned char *key, Gradebook *gradebook)
{
	FILE *fp = NULL;
	unsigned char *ciphertext = NULL;
	unsigned char tag[TAG_LENGTH];
	unsigned char iv[IV_LENGTH];
	int ciphertext_len = 0;

	ciphertext = (unsigned char *) calloc(1, sizeof(Gradebook) + 8);
	generate_key(iv, IV_LENGTH);

	fp = fopen(gradebook->name, "wb");
	if (fp == NULL) {
		#ifdef DEBUG 
			printf("Couldn't open file");
		#endif
		printf("invalid\n");
		free(ciphertext);
		return ERR;
	}
	
	ciphertext_len = gcm_encrypt((unsigned char *) gradebook, sizeof(Gradebook), 
		key, iv, IV_LENGTH, ciphertext, tag);

	if (ciphertext_len <= 0) {
		#ifdef DEBUG
			printf("Encrypt Error\n");
		#endif
		printf("invalid\n");
		free(ciphertext);
		return ERR;
	}
	
	// Write (tag, iv, ciphertext) to the gradebook file
	fwrite(tag, 1, TAG_LENGTH, fp);
	fwrite(iv, 1, IV_LENGTH, fp);
	fwrite(ciphertext, 1, ciphertext_len, fp);

	// Cleanup
	fclose(fp);
	free(ciphertext);
	return 1;
}

/************************************************
 				Input Parsing Functions
*************************************************/

int parse_filename(const char* filename, Gradebook* gradebook) 
{
	int i = 0;

	// Checks if the gradebook name is valid (matches regexp `[azAZ_.]{1, 127}`)
	for (i = 0; i < strlen(filename); i++) {
		if (i == BUFFER_SIZE - 1) { // Gradebook name is too long
			#ifdef DEBUG
				printf("Gradebook name is too long\n");
			#endif
			return ERR;
		}
		// Check if the current character is valid
		char curr = filename[i];
		if (('a' <= curr && curr <= 'z') || ('A' <= curr && curr <= 'Z') || curr == '_' || curr == '.') { // valid character
			gradebook->name[i] = curr;
		} else { // invalid character
			#ifdef DEBUG
				printf("Gradebook name has invalid character\n");
			#endif
			return ERR;
		}
	}
	gradebook->name[i] = 0;
	return 0;
}

int hex_digit(char c) {
	if ('a' <= c && c <= 'f') {
		return c - 'a' + 10;
	} else if ('0' <= c && c <= '9') {
		return c  - '0';
	} else {
		return -1;
	}
} 

int parse_key(const unsigned char* src, char* dest)
{
	int i = 0;

	if (strlen(src) != 2 * KEY_LENGTH) {
		#ifdef DEBUG
			printf("Key length error, got %d\n", (int) strlen(src));
		#endif
		return ERR;
	}

	for (i = 0; i < 2 * KEY_LENGTH; i++) {
		int val = 0;
		if ((val = hex_digit(src[i])) != -1) {
			if (i % 2 == 0) {
				dest[i / 2] = 0 | ((unsigned int) val << 4);
			} else {
				dest[i / 2] = dest[i / 2] | (unsigned int) val;
			}
		} else {
			return ERR;
		}
	}
	return 0;
}

int parse_assignment_name(char* src, char* dest) 
{
	int i = 0;

	for (i = 0; i < strlen(src); i++) {
		if (i == BUFFER_SIZE - 1) {
			#ifdef DEBUG
				printf("Assignment name too long\n");
			#endif
			return ERR;
		}
		char curr = src[i];
		if (('a' <= curr && curr <= 'z') || ('A' <= curr && curr <= 'Z') || ('0' <= curr && curr <= '9')) {
			dest[i] = curr;
		} else {
			#ifdef DEBUG
				printf("Invalid char %c in %s\n", curr, src);
			#endif
			return ERR;
		}
	}
	dest[i] = 0;
	return 0;
}

int is_numeric(char* src) 
{
	for (int i = 0; i < strlen(src); i++) {
		char curr = src[i];
		if (('0' <= curr && curr <= '9') || curr == '.') {
			continue;
		} else {
			return 0;
		}
	}
	return 1;
}

int parse_points(char* src, int* dest) 
{
	if (!is_numeric(src)) {
		#ifdef DEBUG
			printf("Error: %s is not numeric\n", src);
		#endif
		return ERR;
	}
	int points = atoi(src);
	if (points < 0) {
		#ifdef DEBUG
			printf("Negative point value in %s\n", src);
		#endif
		return ERR;
	} else {
		*dest = points;
		return 0;
	}
}

int parse_weight(char* src, float* dest) 
{
	if (!is_numeric(src)) {
		#ifdef DEBUG
			printf("Error: %s is not numeric\n", src);
		#endif
		return ERR;
	}
	float weight = (float) atof(src);
	if (0 <= weight && weight <= 1) {
		*dest = weight;
		return 0;
	} else {
		#ifdef DEBUG
			printf("Error: %f is not in range [0,1]\n", weight);
		#endif
		return ERR;
	}
}

int parse_student_name(char* src, char* dest) 
{
	int i = 0;
	for (i = 0; i < strlen(src); i++) {
		if (i == BUFFER_SIZE - 1) {
			#ifdef DEBUG
				printf("Student name too long\n");
			#endif
			return ERR;
		}
		char curr = src[i];
		if (('a' <= curr && curr <= 'z') || ('A' <= curr && curr <= 'Z')) {
			dest[i] = curr;
		} else {
			#ifdef DEBUG
				printf("Invalid char %c in %s\n", curr, src);
			#endif
			return ERR;
		}
	}
	dest[i] = 0;
	return 0;
}

/************************************************
* 				Gradebook Functions
*************************************************/

int get_student_id(Gradebook* gradebook, char* first_name, char* last_name)
{
	if (gradebook->num_students == 0) {
		return -1;
	}
	Student *students = gradebook->students;
	for (int i = 0; i < gradebook->num_students; i++) {
		if (strcmp(students[i].first_name, first_name) == 0 && strcmp(students[i].last_name, last_name) == 0) {
			return students[i].id;
		}
	}
	return -1;
}

Assignment* get_assignment(Gradebook* gradebook, char* name)
{
	for (int i = 0; i < gradebook->num_assignments; i++) {
		if (strcmp(gradebook->assignments[i].name, name) == 0) {
			return &gradebook->assignments[i];
		}
	}
	return NULL;
}

float get_final_grade(Gradebook* gradebook, char* first_name, char* last_name)
{
	float final_grade = 0.0;
	unsigned int student_id = get_student_id(gradebook, first_name, last_name);
	int student_index = get_student_index(gradebook, student_id);

	for (int i = 0; i < gradebook->num_assignments; i++) {
		final_grade += gradebook->assignments[i].weight * (gradebook->assignments[i].grades[student_index].grade / (float) gradebook->assignments[i].points);
	}
	return final_grade;
}

// If I was using a hashtable this wouldn't be nessecary but I'm too lazy to import things
int get_student_index(Gradebook* gradebook, int student_id) 
{
	for (int i = 0; i < gradebook->num_students; i++) {
		if (gradebook->students[i].id == student_id) {
			return i;
		}
	}
	return -1;
}

/************************************************
* 				Linked List Functions
*************************************************/

void free_list(SList* list) 
{
	Node *curr = list->head;
	while (curr != NULL) {
		Node *next = curr->next;
		free(curr);
		curr = next;
	}
}

void copy_student(Gradebook* gradebook, int student_index, Assignment *assignment, Node* node)
{
	Student* students = gradebook->students;
	memcpy(node->first_name, students[student_index].first_name, BUFFER_SIZE);
	memcpy(node->last_name, students[student_index].last_name, BUFFER_SIZE);
	if (assignment == NULL) {
		node->final_grade = get_final_grade(gradebook, students[student_index].first_name, students[student_index].last_name);
	} else {
		node->assignment_grade = assignment->grades[student_index].grade;
	}
}

// Similar return conditions as strcmp, but it first comparse last names, if they are equal then first names.
// Compares the given names with the student at gradebook->students[index]
int compare_names(Gradebook* gradebook, int index, char* first_name, char* last_name)
{
	int res1 = strcmp(last_name, gradebook->students[index].last_name);
	return (res1 == 0) ? strcmp(first_name, gradebook->students[index].first_name) : res1;
}

void create_list(Gradebook* gradebook, SList* list, Assignment* assignment, char* option)
{
	Student* students = gradebook->students;

	list->head = calloc(1, sizeof(Node));
	for (int i = 0; i < gradebook->num_students; i++) { // i is student_index
		if (i == 0) { // boring base case
			copy_student(gradebook, i, assignment, list->head);
		} else { // interesting main case
			Node* curr = list->head;
			int added = -1;

			while(curr != NULL) {
				int res = 0;

				if (strcmp(option, "-A") == 0) {
					res = compare_names(gradebook, i, curr->first_name, curr->last_name);
				} else {
					if (assignment == NULL) {
						res = (int) floor(curr->final_grade - get_final_grade(gradebook, students[i].first_name, students[i].last_name));
					} else {
						res = assignment->grades[i].grade - curr->assignment_grade;
					}
				}

				if (res > 0) { // Insert the student at index i
					Node* temp = calloc(1, sizeof(Node));

					memcpy(temp, curr, sizeof(Node));
					copy_student(gradebook, i, assignment, curr);
					curr->next = temp;
					curr = temp;
					added = 0;
					break;
				} else {
					if (curr->next == NULL && added == -1) {
						curr->next = calloc(1, sizeof(Node));
						copy_student(gradebook, i, assignment, curr->next);
						break;
					} else {
						curr = curr->next;
					}
				}
			}
		}
	}
}
