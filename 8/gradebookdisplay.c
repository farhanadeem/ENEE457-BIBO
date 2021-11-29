#include "data.h"

void display_error(Gradebook* gradebook) 
{
	printf("invalid\n");
	free(gradebook);
	exit(ERR);
}

int print_grade(Gradebook* gradebook, Assignment* assignment, char* option)
{
	SList* list = calloc(1, sizeof(SList));
	if (strcmp(option, "-A") == 0 || strcmp(option, "-G") == 0) {
		Node* curr = NULL;

		create_list(gradebook, list, assignment, option);
		curr = list->head;
		
		while (curr != NULL) {
			if (assignment == NULL) {
				printf("(%s, %s, %g)\n", curr->last_name, curr->first_name, round(curr->final_grade * 100000.0) / 100000.0);
			} else {
				printf("(%s, %s, %d)\n", curr->last_name, curr->first_name, curr->assignment_grade);
			}
			curr = curr->next;
		}
	} else {
		#ifdef DEBUG
			printf("Invalid option\n");
		#endif
		return ERR;
	}
	free_list(list);
	return 0;
}

void print_student(Gradebook* gradebook, int student_id)
{
	int student_index = get_student_index(gradebook, student_id);
	if (student_index == 1) {
		display_error(gradebook);
	}

	for (int i = 0; i < gradebook->num_assignments; i++) {
		printf("(%s, %d)\n", gradebook->assignments[i].name, gradebook->assignments[i].grades[student_index].grade);
	}
	return;
}

int main(int argc, char* argv[]) 
{
	unsigned char key[KEY_LENGTH + 1];
	Gradebook *gradebook = (Gradebook *) calloc(1, sizeof(Gradebook));

	if (argc < 7) {
		display_error(gradebook);
	}

	// First argument must be `-N`
	if (strcmp(argv[1], "-N") != 0) {
		#ifdef DEBUG
			printf("arg 1 ");
		#endif
		display_error(gradebook);
	}

	// Second argument must be the name of a valid, existing gradebook
	if (parse_filename(argv[2], gradebook) == ERR) {
		display_error(gradebook);
	}

	// Check if a file with the gradebook name already exists
	if (access(gradebook->name, F_OK) == -1) {
		#ifdef DEBUG
			printf("Gradebook with the given name doesn't exist\n");
		#endif
		display_error(gradebook);
  	}

	// Third argument must be `-K`
	if (strcmp(argv[3], "-K") != 0) {
		#ifdef DEBUG
			printf("arg 3 ");
		#endif
		printf("invalid\n");
		display_error(gradebook);
	}

	// Forth argument must be the key
	if (parse_key(argv[4], key) == ERR) {
		#ifdef DEBUG
			printf("Invalid Key\n");
		#endif
		display_error(gradebook);
	}

	// Reads the encrypted file, decrypts, places the contents into gradebook
	if (read_file(key, gradebook) == ERR) {
		display_error(gradebook);
	}

	// Fifth Argument will be the action type
	if (strcmp(argv[5], "-PA") == 0) { // Print Assignment
		char name[BUFFER_SIZE];
		char *option = NULL;
		Assignment *assignment = NULL;

		memset(name, 0, BUFFER_SIZE);

		// Parse Options
		for (int i =6; i < argc; i++) {
			if (strcmp(argv[i], "-AN") == 0) { // Assignment Name
				if (parse_assignment_name(argv[++i], name) == ERR) display_error(gradebook);
			} else if (strcmp(argv[i], "-A") == 0) { // Alphabetical Order
				if (option == NULL) {
					option = argv[i];
				} else {
					#ifdef DEBUG
						printf("Cannot specify -G/-A more than once\n");
					#endif
					display_error(gradebook);
				}
			} else if (strcmp(argv[i], "-G") == 0) { // Grade Order
				if (option == NULL) {
					option = argv[i];
				} else { 
					#ifdef DEBUG
						printf("Cannot specify -G/-A more than once\n");
					#endif
					display_error(gradebook);
				}
			} else {
				#ifdef DEBUG
					printf("Bad argument on %s\n", argv[i]);
				#endif
				display_error(gradebook);
			}
		}

		if (option == NULL || name[0] == 0) {
			#ifdef DEBUG
				printf("Missing some argument\n");
			#endif
			display_error(gradebook);
		}

		assignment = get_assignment(gradebook, name);
		if (assignment == NULL) {
			#ifdef DEBUG
				printf("Assignment %s does not exist\n", name);
			#endif
			display_error(gradebook);
		} 

		if (print_grade(gradebook, assignment, option) == ERR)  {
			#ifdef DEBUG
				printf("Print Assignment Failed\n");
			#endif
			display_error(gradebook);
		}
	} else if (strcmp(argv[5], "-PS") == 0) { // Print Student
		char first_name[BUFFER_SIZE];
		char last_name[BUFFER_SIZE];
		int student_id = 0;

		memset(first_name, 0, BUFFER_SIZE);
		memset(last_name, 0, BUFFER_SIZE);

		// Parse Options
		for (int i = 6; i < argc; i++) {
			if (strcmp(argv[i], "-FN") == 0) { // First Name
				if (parse_student_name(argv[++i], first_name) == ERR) display_error(gradebook);
			} else if (strcmp(argv[i], "-LN") == 0) { // Last Name
				if (parse_student_name(argv[++i], last_name) == ERR) display_error(gradebook);
			} else {
				#ifdef DEBUG
					printf("Bad argument on %s\n", argv[i]);
				#endif
				display_error(gradebook);
			}
		}

		if (first_name[0] == 0 || last_name[0] == 0) {
			#ifdef DEBUG
				printf("Missing some argument\n");
			#endif
			display_error(gradebook);
		}

		student_id = get_student_id(gradebook, first_name, last_name);
		if (student_id == -1) {
			#ifdef DEBUG
				printf("Student with the given name does not exist\n");
			#endif
			display_error(gradebook);
		}
		print_student(gradebook, student_id);
	} else if (strcmp(argv[5], "-PF") == 0) { // Print Final
		char *option = NULL;

		// Parse Options
		for (int i = 6; i < argc; i++) { // Alphabetical Order
			if (strcmp(argv[i], "-A") == 0) {
				option = argv[i];
			} else if (strcmp(argv[i], "-G") == 0) { // Grade Order
				option = argv[i];
			} else {
				#ifdef DEBUG
					printf("Bad argument on %s\n", argv[i]);
				#endif
				display_error(gradebook);
			}
		}

		if (option == NULL) {
			display_error(gradebook);
		}
		if (print_grade(gradebook, NULL, option) == ERR)  {
			#ifdef DEBUG
				printf("Print Assignment Failed\n");
			#endif
			display_error(gradebook);
		}
	} else {
		#ifdef DEBUG
			printf("Bad argument on %s\n", argv[5]);
		#endif
		display_error(gradebook);
	}

	free(gradebook);
	return 0;
}
