#include "data.h"

void setup_error(Gradebook *gradebook) 
{
  printf("invalid\n");
  free(gradebook);
  exit(ERR);
}

int main(int argc, char** argv) 
{
	Gradebook *gradebook = NULL;
	char gradebook_name[BUFFER_SIZE];
	unsigned char key[KEY_LENGTH + 1];

	if (argc < 3) {
		printf("Usage: setup -N <gradebook name>\n");
		exit(ERR);
	} else if (argc > 3) {
		setup_error(NULL);
	}

	if (strcmp(argv[1], "-N") != 0) {
		#ifdef DEBUG
			printf("arg 1 ");
		#endif
		setup_error(NULL);
	}

	gradebook = (Gradebook *) calloc(1, sizeof(Gradebook));
	if (parse_filename(argv[2], gradebook) == ERR) {
		setup_error(gradebook);
	}

	// Check if a file with the gradebook name already exists
	if (access(gradebook->name, F_OK) == 0) {
		#ifdef DEBUG
			printf("Gradebook with the given name already exists\n");
		#endif
		setup_error(gradebook);
  	}

	// Randomly generate the key and iv
	generate_key(key, KEY_LENGTH);
	key[KEY_LENGTH] = 0;
	
	// Encrypt and write the gradebook
	if (write_file(key, gradebook) == ERR) {
		#ifdef DEBUG
			printf("write_file failed\n");
		#endif
		setup_error(gradebook);
	}

	print_key(key);

	free(gradebook);
	return 0;
}
