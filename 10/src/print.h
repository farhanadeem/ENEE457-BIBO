#ifndef PRINT_H
#define PRINT_H


#include "data.h"


typedef enum {
    NAME_SORT = 0,
    GRADE_SORT,
} Sort_Type;

typedef enum {
    INT_GRADE = 0,
    FLOAT_GRADE,
} Grade_Type;

typedef struct {
    int name_len;
    char *name;
    int int_grade;
    float float_grade;
} Print_Tuple;


void print_assignment(Gradebook *book, char *name, int sort);
void print_student(Gradebook *book, char *fn, char *ln, int sort);
void print_final(Gradebook *book, int sort);


#endif

