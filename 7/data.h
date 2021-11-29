#ifndef _BUFFR_H
#define _BUFFR_H

typedef struct _Buffer {
  unsigned char *Buf;
  unsigned long Length;
} Buffer;

typedef enum _ActionType {
  add_assignment,
  delete_assignment,
  add_student,
  delete_student,
  add_grade,
} ActionType;

typedef enum _PrintActions {
  print_assignment,
  print_student,
  print_final,
} PrintActions;

typedef enum _PrintMode {
  none,
  alphabetical,
  grade_order,
} PrintMode;

typedef struct _Student {
  unsigned int name_len;
  int num_grades;
  char * name;
  unsigned int * assignment_ids;
  unsigned int * grades;
} Student;

typedef struct _Assignment {
  unsigned int id;
  unsigned int points; //number of points its out of.
  double weight; //weight of this assignment.
  unsigned int name_len;
  char * name; //assignment name. 
} Assignment;

typedef struct _Gradebook {
  unsigned int num_assignments;
  unsigned int next_id;
  unsigned int num_students;
  Assignment ** assignments;
  Student ** students;
} Gradebook;


Buffer read_from_path(char *path, unsigned char *key);
void write_to_path(char *path, Buffer *B, unsigned char *key);
Buffer concat_buffs(Buffer *A, Buffer *B);
Buffer print_record(Gradebook *R);
void dump_record(Gradebook *R);

int read_Gradebook_from_path(char *path, unsigned char *key, Gradebook **outbuf, unsigned int *outnum);
int write_Gradebook_to_path(char *path, unsigned char* key, Gradebook * G);
#endif
