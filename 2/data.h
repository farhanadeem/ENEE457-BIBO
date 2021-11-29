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
  add_grade
} ActionType;

typedef struct _Assignment {
  unsigned char *name;
  unsigned int pts;
  float weight;
  unsigned int *grades;
} Assignment;

typedef struct _Gradebook {
  unsigned char **names;  // stored as "First Last"
  Assignment **assignments;
  unsigned int assnCount;  // total assignments
  unsigned int rows;       // total entries
} Gradebook;

int read_from_path(Buffer *B, char *path, unsigned char *key);
int grab_IV(Buffer *B, Buffer *T, char *path);
void write_to_path(char *path, Buffer *B, unsigned char *key_data);
int print_gradebook(Buffer *B, Gradebook *R);

int get_Gradebook(Gradebook *R, Buffer *B);

#endif
