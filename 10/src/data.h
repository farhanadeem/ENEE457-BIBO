#ifndef DATA_H
#define DATA_H


#define NO_GRADE -1


typedef struct _Student {
    int fn_len;
    char *fn;
    int ln_len;
    char *ln;
    struct _Student *next;
    struct _Student *prev;
} Student;

typedef struct _Assignment {
    float weight;
    int points;
    int name_len;
    char *name;
    struct _Assignment *next;
    struct _Assignment *prev;
} Assignment;

typedef struct _Grade {
    int grade;
    struct _Grade *next_student;
    struct _Grade *prev_student;
    struct _Grade *next_assignment;
    struct _Grade *prev_assignment;
} Grade;

typedef struct {
    int num_students;
    Student *students;
    int num_assignments;
    Assignment *assignments;
    Grade *grades;
} Gradebook;

void create_assignment(Gradebook *book, char *name, char *weight, char *points);
void remove_assignment(Gradebook *book, char *name);
void create_student(Gradebook *book, char *fn, char *ln);
void remove_student(Gradebook *book, char *fn, char *ln);
void update_grade(Gradebook *book, char *fn, char *ln, char *a_name, char *grade);
void print_gradebook(Gradebook *book);


#endif

