#include "data.h"

int add_error(Gradebook *gradebook) 
{
	printf("invalid\n");
	free(gradebook);
	exit(ERR);
}

int add_assignment(Gradebook *gradebook, char *name, int points, float weight)
{
	float total_weight = 0.0;

	// Ensure no assignment with the same name already exists
	for (int i = 0; i < gradebook->num_assignments; i++) {
		if (strcmp(gradebook->assignments[i].name, name) == 0) {
			#ifdef DEBUG
				printf("Assignment with name %s already exists\n", name);
			#endif
			return ERR;
		}
		total_weight += gradebook->assignments[i].weight;
	}

	// Ensure there is room for the assignment in the gradebook
	if (gradebook->num_assignments == 128) {
		#ifdef DEBUG
			printf("Max number of assignments stored\n");
		#endif
		return ERR;
	}

	// Ensure the total weight of all assignments is still valid
	if (total_weight + weight > 1.0) {
		#ifdef DEBUG
			printf("Adding this assignment makes total_weight > 1\n");
		#endif
		return ERR;
	}

	Assignment *assignment = &gradebook->assignments[gradebook->num_assignments];
	memcpy(assignment->name, name, BUFFER_SIZE);
	assignment->weight = weight;
	assignment->points = points;

	// Add grades of 0 for all students 
	for (int i = 0; i < gradebook->num_students; i++) {
		assignment->grades[i].grade = 0;
		assignment->grades[i].student_id = gradebook->students[i].id;
	}
	
	gradebook->num_assignments++;
}

int delete_assignment(Gradebook *gradebook, char *name)
{
	int assignment_index = -1;

	for (int i = 0; i < gradebook->num_assignments; i++) {
		if (strcmp(gradebook->assignments[i].name, name) == 0) {
			assignment_index = i;
			break;
		}
	}
	
	if (assignment_index == -1) {
		#ifdef DEBUG
			printf("No assignment with name %s exists\n", name);
		#endif
		return ERR;
	}
	// Copy all assignments after the assignment to delete back by one
	for (int i =0 ; i < gradebook->num_assignments; i++) {
		if (i > assignment_index) {
			memcpy(&gradebook->assignments[i - 1], &gradebook->assignments[i], sizeof(Assignment));
		}
	}
	gradebook->num_assignments--;
	return 0;
}

int add_student(Gradebook *gradebook, char *first_name, char *last_name)
{
	int num_students = gradebook->num_students;
	unsigned int id = 0;

	if (get_student_id(gradebook, first_name, last_name) != -1) {
		#ifdef DEBUG
			printf("Student to add already exists\n");
		#endif
		return ERR;
	}
	id = gradebook->max_id++;
	// Add student to list of students
	memcpy(&gradebook->students[num_students].first_name, first_name, BUFFER_SIZE);
	memcpy(&gradebook->students[num_students].last_name, last_name, BUFFER_SIZE);
	gradebook->students[num_students].id = id;

	// Add grades of 0 for this student for all assignments
	for(int i = 0; i < gradebook->num_assignments; i++) {
		Assignment *curr = &gradebook->assignments[i];
		curr->grades[num_students].grade = 0;
		curr->grades[num_students].student_id = id;
	}
	gradebook->num_students++;
	return 0;
}

int delete_student(Gradebook *gradebook, char *first_name, char *last_name)
{
	int student_index = 0;

	student_index = get_student_index(gradebook, get_student_id(gradebook, first_name, last_name));

	if (student_index == -1) {
		#ifdef DEBUG
			printf("Student to delete doesn't exist\n");
		#endif
		return ERR;
	}

	// Remove the student from the list of students
	for (int i = student_index + 1; i < gradebook->num_students; i++) {
		memcpy(&gradebook->students[i - 1], &gradebook->students[i], sizeof(Student));
	}
	// Remove the student's grades for all assignments
	for (int i = 0; i < gradebook->num_assignments; i++) {
		for (int j = student_index + 1; j < gradebook->num_students; j++) {
			memcpy(&gradebook->assignments[i].grades[j - 1], &gradebook->assignments[i].grades[j], sizeof(Grade));
		}
	}
	
	gradebook->num_students--;
	return 0;
}

int add_grade(Gradebook *gradebook, char *first_name, char *last_name, char *assignment_name, int grade)
{
	Assignment *assignment = get_assignment(gradebook, assignment_name);
	unsigned int id = get_student_id(gradebook, first_name, last_name);
	int student_index = get_student_index(gradebook, id);

	if (id == -1 || student_index == -1) { // Student is not in the gradebook
		#ifdef DEBUG
			printf("Student with given name does not exist\n");
		#endif
		return ERR;
	} else if (assignment == NULL) {
		#ifdef DEBUG
			printf("Assignment with the given name does not exist\n");
		#endif
		return ERR; 
	}
	#ifdef DEBUG
		if (assignment->grades[student_index].student_id != id) printf("BAD THINGS");
	#endif
	assignment->grades[student_index].grade = grade;
	
	return 0;
}

int main(int argc, char **argv) 
{
	unsigned char key[KEY_LENGTH + 1];
	Gradebook *gradebook = (Gradebook *) calloc(1, sizeof(Gradebook));

	if (argc < 8) {
		add_error(gradebook);
	}

	// First argument must be `-N`
	if (strcmp(argv[1], "-N") != 0) {
		#ifdef DEBUG
			printf("arg 1 ");
		#endif
		add_error(gradebook);
	}

	// Second argument must be the name of a valid, existing gradebook
	if (parse_filename(argv[2], gradebook) == ERR) {
		add_error(gradebook);
	}

	// Check if a file with the gradebook name already exists
	if (access(gradebook->name, F_OK) == -1) {
		#ifdef DEBUG
			printf("Gradebook with the given name doesn't exist\n");
		#endif
		add_error(gradebook);
  	}

	// Third argument must be `-K`
	if (strcmp(argv[3], "-K") != 0) {
		#ifdef DEBUG
			printf("arg 3 ");
		#endif
		printf("invalid\n");
		add_error(gradebook);
	}

	// Forth argument must be the key
	if (parse_key(argv[4], key) == ERR) {
		#ifdef DEBUG
			printf("Invalid Key\n");
		#endif
		add_error(gradebook);
	}

	// Reads the encrypted file, decrypts, places the contents into gradebook
	if (read_file(key, gradebook) == ERR) {
		add_error(gradebook);
	}

	// Fifth Argument will be the action type
	if (strcmp(argv[5], "-AA") == 0) { // Add Assignment
		char name[BUFFER_SIZE];
		int points = -1;
		float weight = -1.0;

		memset(name, 0, BUFFER_SIZE);

		// Parse Options
		for (int i = 6; i < argc; i++) {
			if (strcmp(argv[i], "-AN") == 0) { // Assignment Name
				if (parse_assignment_name(argv[++i], name) == ERR) add_error(gradebook);
			} else if (strcmp(argv[i], "-P") == 0) { // Points
				if (parse_points(argv[++i], &points) == ERR) add_error(gradebook);
			} else if (strcmp(argv[i], "-W") == 0) { // Weight
				if (parse_weight(argv[++i], &weight) == ERR) add_error(gradebook);
			} else {
				#ifdef DEBUG
					printf("Bad argument on %s\n", argv[i]);
				#endif
				add_error(gradebook);
			}
		}

		if (name[0] == 0 || points == -1 || weight < 0) {
			#ifdef DEBUG
				printf("Missing some argument\n");
			#endif
			add_error(gradebook);
		}

		if (add_assignment(gradebook, name, points, weight) == ERR) {
			add_error(gradebook);
		}
	} else if (strcmp(argv[5], "-DA") == 0) { // Delete Assignment
		char name[BUFFER_SIZE];

		memset(name, 0, BUFFER_SIZE);

		// Parse Option
		for (int i = 6; i < argc; i++) {
			if (strcmp(argv[i], "-AN") == 0) { // Assignment Name
				if (parse_assignment_name(argv[++i], name) == ERR) add_error(gradebook);
			} else {
				#ifdef DEBUG
					printf("Bad argument on %s\n", argv[i]);
				#endif
				add_error(gradebook);
			}
		}

		if (name[0] == 0) {
			#ifdef DEBUG
				printf("Missing some argument\n");
			#endif
			add_error(gradebook);
		}

		if (delete_assignment(gradebook, name) == ERR) {
			add_error(gradebook);
		}
	} else if (strcmp(argv[5], "-AS") == 0) { // Add Student
		char first_name[BUFFER_SIZE];
		char last_name[BUFFER_SIZE];

		memset(first_name, 0, BUFFER_SIZE);
		memset(last_name, 0, BUFFER_SIZE);

		// Parse Options
		for (int i = 6; i < argc; i++) {
			if (strcmp(argv[i], "-FN") == 0) { // First Name
				if (parse_student_name(argv[++i], first_name) == ERR) add_error(gradebook);
			} else if (strcmp(argv[i], "-LN") == 0) { // Last Name
				if (parse_student_name(argv[++i], last_name) == ERR) add_error(gradebook);
			} else {
				#ifdef DEBUG
					printf("Bad argument on %s\n", argv[i]);
				#endif
				add_error(gradebook);
			}
		}

		if (first_name[0] == 0 || last_name[0] == 0) {
			#ifdef DEBUG
				printf("Missing some argument\n");
			#endif
			add_error(gradebook);
		}
		
		if (add_student(gradebook, first_name, last_name) == ERR) {
			add_error(gradebook);
		}
	} else if (strcmp(argv[5], "-DS") == 0) { // Delete Student
		char first_name[BUFFER_SIZE];
		char last_name[BUFFER_SIZE];

		memset(first_name, 0, BUFFER_SIZE);
		memset(last_name, 0, BUFFER_SIZE);

		// Parse Options
		for (int i = 6; i < argc; i++) {
			if (strcmp(argv[i], "-FN") == 0) { // First Name
				if (parse_student_name(argv[++i], first_name) == ERR) add_error(gradebook);
			} else if (strcmp(argv[i], "-LN") == 0) { // Last Name
				if (parse_student_name(argv[++i], last_name) == ERR) add_error(gradebook);
			} else {
				#ifdef DEBUG
					printf("Bad argument on %s\n", argv[i]);
				#endif
				add_error(gradebook);
			}
		}

		if (first_name[0] == 0 || last_name[0] == 0) {
			#ifdef DEBUG
				printf("Missing some argument\n");
			#endif
			add_error(gradebook);
		}
		
		if (delete_student(gradebook, first_name, last_name) == ERR) {
			add_error(gradebook);
		}
	} else if (strcmp(argv[5], "-AG") == 0) { // Add Grade
		char first_name[BUFFER_SIZE];
		char last_name[BUFFER_SIZE];
		char assignment_name[BUFFER_SIZE];
		int points = -1;

		memset(first_name, 0, BUFFER_SIZE);
		memset(last_name, 0, BUFFER_SIZE);
		memset(assignment_name, 0, BUFFER_SIZE);

		// Parse Options
		for (int i = 6; i < argc; i++) {
			if (strcmp(argv[i], "-FN") == 0) { // First Name
				if (parse_student_name(argv[++i], first_name) == ERR) add_error(gradebook);
			} else if (strcmp(argv[i], "-LN") == 0) { // Last Name
				if (parse_student_name(argv[++i], last_name) == ERR) add_error(gradebook);
			} else if (strcmp(argv[i], "-AN") == 0) { // Assignment Name
				if (parse_assignment_name(argv[++i], assignment_name) == ERR) add_error(gradebook);
			} else if (strcmp(argv[i], "-G") == 0) { // Grade
				if (parse_points(argv[++i], &points) == ERR) add_error(gradebook);
			} else {
				add_error(gradebook);
			}
		}

		if (first_name[0] == 0 || last_name[0] == 0 || assignment_name[0] == 0 || points == -1) {
			#ifdef DEBUG
				printf("Missing some argument\n");
			#endif
			add_error(gradebook);
		}

		if (add_grade(gradebook, first_name, last_name, assignment_name, points) == ERR) {
			add_error(gradebook);
		}
	} else {
		#ifdef DEBUG
			printf("Invalid Action Type Argument\n");
		#endif
		add_error(gradebook);
	}

	if (write_file(key, gradebook) == ERR) {
		add_error(gradebook);
	}

	return 0;
}
