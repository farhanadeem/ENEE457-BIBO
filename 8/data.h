#ifndef DATA_HEADER
#define DATA_HEADER

#include <openssl/evp.h>
#include <unistd.h>
#include <string.h>
#include <openssl/err.h>
#include <math.h>

#define BUFFER_SIZE 128
#define GRADEBOOK_SIZE 128
#define ERR 255
#define KEY_LENGTH 32
#define IV_LENGTH 12
#define TAG_LENGTH 16



typedef struct _Grade {
	unsigned int student_id;
	unsigned short grade;
} Grade;

typedef struct _Assignment {
	char name[BUFFER_SIZE];
	Grade grades[GRADEBOOK_SIZE];
	unsigned short points;
	float weight;
} Assignment;

typedef struct _Student {
	char first_name[BUFFER_SIZE];
	char last_name[BUFFER_SIZE];
	unsigned int id;
} Student;

typedef struct _Gradebook {
	char name[BUFFER_SIZE];
	Assignment assignments[GRADEBOOK_SIZE];
	Student students[GRADEBOOK_SIZE];
	unsigned short num_assignments;
	unsigned short num_students;
	unsigned int max_id;
} Gradebook;

typedef struct _Node {
	unsigned short assignment_grade;
	float final_grade;
	char first_name[BUFFER_SIZE];
	char last_name[BUFFER_SIZE];
	struct _Node *next;
} Node;

typedef struct _SList {
	Node *head;
} SList;

int read_file(const unsigned char* key, Gradebook* gradebook);
int write_file(const unsigned char* key, Gradebook *gradebook);
int parse_filename(const char *filename, Gradebook* gradebook);
void print_key(const unsigned char* key);
Assignment* get_assignment(Gradebook *gradebook, char* name);
int parse_key(const unsigned char *src, char* key);
void generate_key(unsigned char* key, int size);
int parse_assignment_name(char* src, char* dest);
int parse_points(char* src, int* dest);
int parse_weight(char* src, float* dest);
int parse_student_name(char* src, char* dest);
int get_student_id(Gradebook* gradebook, char* first_name, char* last_name);
int get_student_index(Gradebook* gradebook, int student_id);
void free_list(SList* list);
void create_list(Gradebook* gradebook, SList* list, Assignment* assignment, char* option);
#endif