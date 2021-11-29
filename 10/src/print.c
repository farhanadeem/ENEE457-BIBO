
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "data.h"
#include "print.h"


/*******************************************************************************
 *  Prototypes
 ******************************************************************************/

static void build_student_tuple(Print_Tuple *t, Student *s, Grade *g);
static void build_assignment_tuple(Print_Tuple *t, Assignment *s, Grade *g);
static void sort_tuples(Print_Tuple *arr, int size, int type, int val_type);
static int compare_names(Print_Tuple *t1, Print_Tuple *t2, int type);
static int compare_grades(Print_Tuple *t1, Print_Tuple *t2, int type);
static void sort(Print_Tuple *arr, int size, int type, int (*compare)(Print_Tuple *, Print_Tuple *, int));
static void swap(Print_Tuple *t1, Print_Tuple *t2);
static void print_tuple(Print_Tuple *t, Grade_Type type);


/*******************************************************************************
 *  Public Functions
 ******************************************************************************/

void print_assignment(Gradebook *book, char *name, int type) {
    Print_Tuple *tuples;
    Assignment *a;
    Student *s;
    Grade *g;
    int name_len, index, i;

    // assert preconditions
    ASSERT(book);
    ASSERT(name);

    // find matching assignment
    name_len = strnlen(name, MAX_STR_LEN);
    a = book->assignments;
    for (index = 0; index < book->num_assignments; index++) {
        ASSERT(a);
        if (name_len == a->name_len)
            if (strncmp(a->name, name, name_len) == 0)
                break;
        a = a->next;
    }
    ASSERT(index != book->num_assignments);
    ASSERT(a);

    // navigate to corresponding row of grade grid
    g = book->grades;
    ASSERT(g);
    for (i = 0; i < index; i++) {
        g = g->next_assignment;
        ASSERT(g);
    }

    // build tuple array
    tuples = (Print_Tuple *)malloc(book->num_students*sizeof(*tuples));
    s = book->students;
    for (i = 0; s && g; i++) {
        ASSERT(s);
        ASSERT(g);
        build_student_tuple(&tuples[i], s, g);
        s = s->next;
        g = g->next_student;
    }
    ASSERT(i == book->num_students);

    // sort tuple array
    sort_tuples(tuples, book->num_students, type, INT_GRADE);

    // print tuple array
    for (i = 0; i < book->num_students; i++)
        print_tuple(&tuples[i], INT_GRADE);
}

void print_student(Gradebook *book, char *fn, char *ln, int type) {
    Print_Tuple *tuples;
    Assignment *a;
    Student *s;
    Grade *g;
    int fn_len, ln_len, index, i;

    // assert preconditions
    ASSERT(book);
    ASSERT(fn);
    ASSERT(ln);

    // find matching student
    fn_len = strnlen(fn, MAX_STR_LEN);
    ln_len = strnlen(ln, MAX_STR_LEN);
    s = book->students;
    for (index = 0; index < book->num_students; index++) {
        ASSERT(s);
        if ((fn_len == s->fn_len) && (ln_len == s->ln_len))
            if (strncmp(s->fn, fn, fn_len) == 0)
                if (strncmp(s->ln, ln, ln_len) == 0)
                    break;
        s = s->next;
    }
    ASSERT(index != book->num_students);
    ASSERT(s);

    // navigate to corresponding col of grade grid
    g = book->grades;
    ASSERT(g);
    for (i = 0; i < index; i++) {
        g = g->next_student;
        ASSERT(g);
    }

    // navigate to end of linked-lists
    a = book->assignments;
    while (a->next && g->next_assignment) {
        a = a->next;
        g = g->next_assignment;
    }
    ASSERT(a->next == NULL);
    ASSERT(g->next_assignment == NULL);

    // build tuple array
    tuples = (Print_Tuple *)malloc(book->num_assignments*sizeof(*tuples));
    for (i = 0; a && g; i++) {
        build_assignment_tuple(&tuples[i], a, g);
        a = a->prev;
        g = g->prev_assignment;
    }
    ASSERT(i == book->num_assignments);
    ASSERT(a == NULL);
    ASSERT(g == NULL);

    // print tuple array
    for (i = 0; i < book->num_assignments; i++)
        print_tuple(&tuples[i], INT_GRADE);

}

void print_final(Gradebook *book, int type) {
    Print_Tuple *tuples;
    Assignment *a;
    Student *s;
    Grade *g, *top;
    float final;
    int i, j;

    // assert preconditions
    ASSERT(book);

    // build tuple array
    tuples = (Print_Tuple *)malloc(book->num_students*sizeof(*tuples));

    // compute final grades for each student
    s = book->students;
    top = book->grades;
    for (i = 0; s && top; i++) {
        final = 0.0f;
        a = book->assignments;
        g = top;

        for (j = 0; a && g; j++) {
            final += (a->weight * (float)g->grade) / (float)a->points;
            a = a->next;
            g = g->next_assignment;
        }
        ASSERT(a == NULL);
        ASSERT(g == NULL);
        ASSERT(j == book->num_assignments);

        // add student to array
        build_student_tuple(&tuples[i], s, top);
        tuples[i].float_grade = final;

        s = s->next;
        top = top->next_student;
    }
    ASSERT(s == NULL);
    ASSERT(top == NULL);
    ASSERT(i == book->num_students);

    // sort tuple array
    sort_tuples(tuples, book->num_students, type, FLOAT_GRADE);

    // print tuple array
    for (i = 0; i < book->num_students; i++)
        print_tuple(&tuples[i], FLOAT_GRADE);
}


/*******************************************************************************
 *  Private Functions
 ******************************************************************************/

static void build_student_tuple(Print_Tuple *t, Student *s, Grade *g) {
    int name_len;
    char *name;

    // assert preconditions
    ASSERT(t);
    ASSERT(s);
    ASSERT(g);

    // allocate name string
    name_len = s->fn_len + s->ln_len + 3;
    name = (char *)malloc(name_len);
    ASSERT(name);
    memset(name, 0, name_len);

    // build name string
    memcpy(name, s->ln, s->ln_len);
    name[s->ln_len] = ',';
    name[s->ln_len+1] = ' ';
    memcpy(&name[s->ln_len+2], s->fn, s->fn_len);

    // populate tuple
    t->name_len = name_len;
    t->name = name;
    t->int_grade = g->grade;
}

static void build_assignment_tuple(Print_Tuple *t, Assignment *a, Grade *g) {
    // assert preconditions
    ASSERT(t);
    ASSERT(a);
    ASSERT(g);

    // populate tuple
    t->name_len = a->name_len;
    t->name = a->name;
    t->int_grade = g->grade;
}

static void sort_tuples(Print_Tuple *arr, int size, int type, int val_type) {
    // assert preconditions
    ASSERT(arr);
    ASSERT(size > 0);

    if (type == NAME_SORT) {
        sort(arr, size, val_type, compare_names);
    } else {
        sort(arr, size, val_type, compare_grades);
    }
}

static int compare_names(Print_Tuple *t1, Print_Tuple *t2, int type) {
    int len, retval;

    // assert preconditions
    ASSERT(t1);
    ASSERT(t2);

    // compare names
    len = (t1->name_len > t2->name_len) ? t2->name_len : t1->name_len;
    retval = -strncmp(t1->name, t2->name, len);
    if (retval == 0)
        return (t1->name_len < t2->name_len);

    return retval;
}

static int compare_grades(Print_Tuple *t1, Print_Tuple *t2, int type) {
    // assert preconditions
    ASSERT(t1);
    ASSERT(t2);

    // compare grades
    if (type == INT_GRADE) {
        return t1->int_grade > t2->int_grade;
    } else if (type == FLOAT_GRADE) {
        return t1->float_grade > t2->float_grade;
    } else {
        ASSERT(0);
    }
    return 0;
}

static void sort(Print_Tuple *arr, int size, int type, int (*compare)(Print_Tuple *, Print_Tuple *, int)) {
    int i, j;

    // assert preconditions
    ASSERT(arr);

    // perform insertion sort
    for (i = 0; i < size; i++)
        for (j = i+1; j < size; j++)
            if (compare(&arr[j], &arr[i], type) > 0)
                swap(&arr[i], &arr[j]);
}

static void swap(Print_Tuple *t1, Print_Tuple *t2) {
    Print_Tuple temp;

    // assert preconditions
    ASSERT(t1);
    ASSERT(t2);

    // perform swap
    temp = *t1;
    *t1 = *t2;
    *t2 = temp;
}

static void print_tuple(Print_Tuple *t, Grade_Type type) {
    ASSERT(t);
    if (type == INT_GRADE) {
        printf("(%.*s, %d)\n", t->name_len, t->name, t->int_grade);
    } else if (type == FLOAT_GRADE) {
        printf("(%.*s, %g)\n", t->name_len, t->name, t->float_grade);
    } else {
        ASSERT(0);
    }
}

