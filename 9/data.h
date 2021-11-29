#ifndef _BUFFR_H
#define _BUFFR_H

typedef struct _Grades Grade;
typedef struct _Student Student;
typedef struct _Assignment Assignment;

typedef struct _Buffer {
  unsigned char *buf;
  unsigned long length;
} Buffer;

typedef enum _ActionType {
  add_assignment,
  delete_assignment,
  add_student,
  delete_student,
  add_grade
} ActionType;

typedef enum _PrintType {
  print_assignment,
  print_student,
  print_final  
} PrintType;

struct _Grades{
  int score;
  Grade* next;
};

struct _Student{
  char* firstName;
  char* lastName;
  Grade* grades;
  Student* next;

};

struct _Assignment {
  char* assignmentName;
  int points;
  float weight;
  int assignmentNumber;
  Assignment* next;
};

typedef struct _Gradebook {
  Assignment* assignments;
  Student* students;
  int numAssignments;
  int numStudents;
} Gradebook;

void handleErrors(void);
int checkHex(char* key);
void freeBuffer(Buffer* b);
int compute_Gradebook_size(Gradebook R); 
void dump_assignment(Assignment *A); 
Buffer* read_from_path(FILE* fp, unsigned char *key);
void write_to_path(FILE* fp, Gradebook G, unsigned char *key);
void concat_buffs(Buffer *A, char* buffer, int len);
Buffer print_record(Gradebook *R);
void dump_record(Gradebook *R);
int checkFileName(char* name);
int checkAssignmentName(char* name);
int checkName(char* name);
Gradebook *read_Gradebook_from_path(FILE* fp, unsigned char *key) ;
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char **ciphertext);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *tag, unsigned char **plaintext);
int findAssignmentName(char* assignmentName, Gradebook G);
int findStudentName(char *firstName, char *lastName, Gradebook G);

int read_records_from_path(char *path, unsigned char *key, Gradebook **, unsigned int *);

#endif
