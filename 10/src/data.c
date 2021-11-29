
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "data.h"


/*******************************************************************************
 *  Public Functions
 ******************************************************************************/

void create_assignment(Gradebook *book, char *name, char *weight, char *points) {
    int retval, name_len, i;
    float total_weight;
    Assignment *new, *old;
    Grade *g1, *g2, *g3;

    // assert preconditions
    ASSERT(book);
    ASSERT(name);
    ASSERT(weight);
    ASSERT(points);

    // allocate new assignment
    new = (Assignment *)malloc(sizeof(*new));
    ASSERT(new);

    // populate new assignment
    retval = sscanf(weight, "%f", &new->weight);
    ASSERT(retval == 1);
    ASSERT((new->weight >= 0.0f) && (new->weight <= 1.0f));

    retval = sscanf(points, "%d", &new->points);
    ASSERT(retval == 1);
    ASSERT(new->points >= 0);

    name_len = strnlen(name, MAX_STR_LEN);
    new->name_len = name_len;

    new->name = (char *)malloc(name_len);
    ASSERT(new->name);
    strncpy(new->name, name, name_len);

    new->next = book->assignments;
    new->prev = NULL;

    // validate name and weight
    total_weight = 0.0f;
    old = book->assignments;
    while (old) {
        total_weight += old->weight;
        if (old->name_len == new->name_len)
            ASSERT(strncmp(old->name, new->name, name_len));
        old = old->next;
    }
    ASSERT(total_weight + new->weight <= 1.0f);

    // update assingment linked list
    if (book->assignments)
        book->assignments->prev = new;
    book->assignments = new;

    // update number of assignments
    book->num_assignments++;

    // update grid of grades
    g2 = NULL;
    g3 = book->grades;
    if ((book->num_assignments * book->num_students) != 0) {
        for (i = 0; i < book->num_students; i++) {
            g1 = (Grade *)malloc(sizeof(*g1));
            ASSERT(g1);
            g1->grade = NO_GRADE;
            g1->next_assignment = g3;
            g1->prev_assignment = NULL;
            g1->next_student = NULL;
            g1->prev_student = g2;

            if (i == 0)
                book->grades = g1;
            if (g2)
                g2->next_student = g1;
            if (g3) {
                g3->prev_assignment = g1;
                g3 = g3->next_student;
            }
            g2 = g1;
        }
    }
}

void remove_assignment(Gradebook *book, char *name) {
    Assignment *node;
    Grade *g;
    int name_len, index, i;

    //assert preconditions
    ASSERT(book);
    ASSERT(name);

    // find matching assignment
    name_len = strnlen(name, MAX_STR_LEN);
    node = book->assignments;
    for (index = 0; index < book->num_assignments; index++) {
        ASSERT(node);
        if (name_len == node->name_len)
            if (strncmp(node->name, name, name_len) == 0)
                break;
        node = node->next;
    }
    ASSERT(index != book->num_assignments);
    ASSERT(node);

    // remove assignment from linked list
    book->num_assignments--;
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    if (index == 0)
        book->assignments = node->next;

    // remove assignment from grade grid
    if (book->num_students == 0) {
        ASSERT(book->grades == NULL);
        return;
    }
    if (book->num_assignments == 0) {
        book->grades = NULL;
        return;
    }

    ASSERT(book->grades);
    g = book->grades;
    for (i = 0; i < index; i++) {
        ASSERT(g);
        g = g->next_assignment;
    }
    while (g) {
        if (g->prev_assignment)
            g->prev_assignment->next_assignment = g->next_assignment;
        if (g->next_assignment)
            g->next_assignment->prev_assignment = g->prev_assignment;
        g = g->next_student;
    }
    if (index == 0)
        book->grades = book->grades->next_assignment;
}

void create_student(Gradebook *book, char *fn, char *ln) {
    int fn_len, ln_len, i, fn_cmp, ln_cmp;
    Student *new, *old;
    Grade *g1, *g2, *g3;

    // assert preconditions
    ASSERT(book);
    ASSERT(fn);
    ASSERT(ln);

    // allocate new student
    new = (Student *)malloc(sizeof(*new));
    ASSERT(new);

    // populate new student
    fn_len = strnlen(fn, MAX_STR_LEN);
    new->fn_len = fn_len;

    new->fn = (char *)malloc(fn_len);
    ASSERT(new->fn);
    strncpy(new->fn, fn, fn_len);

    ln_len = strnlen(ln, MAX_STR_LEN);
    new->ln_len = ln_len;

    new->ln = (char *)malloc(ln_len);
    ASSERT(new->ln);
    strncpy(new->ln, ln, ln_len);

    new->next = book->students;
    new->prev = NULL;

    // ensure not a duplicate
    old = book->students;
    while (old) {
        if ((old->fn_len == new->fn_len) && (old->ln_len == new->ln_len)) {
            fn_cmp = strncmp(old->fn, new->fn, fn_len);
            ln_cmp = strncmp(old->ln, new->ln, ln_len);
            ASSERT((fn_cmp != 0) || (ln_cmp != 0));
        }
        old = old->next;
    }

    // update student linked list
    if (book->students)
        book->students->prev = new;
    book->students = new;

    // update number of students
    book->num_students++;

    // update grid of grades
    g2 = NULL;
    g3 = book->grades;
    if ((book->num_students * book->num_assignments) != 0) {
        for (i = 0; i < book->num_assignments; i++) {
            g1 = (Grade *)malloc(sizeof(*g1));
            ASSERT(g1);
            g1->grade = NO_GRADE;
            g1->next_student = g3;
            g1->prev_student = NULL;
            g1->next_assignment = NULL;
            g1->prev_assignment = g2;

            if (i == 0)
                book->grades = g1;
            if (g2)
                g2->next_assignment = g1;
            if (g3) {
                g3->prev_student = g1;
                g3 = g3->next_assignment;
            }
            g2 = g1;
        }
    }
}

void remove_student(Gradebook *book, char *fn, char *ln) {
    Student *node;
    Grade *g;
    int fn_len, ln_len, index, i;

    //assert preconditions
    ASSERT(book);
    ASSERT(fn);
    ASSERT(ln);

    // find matching student
    fn_len = strnlen(fn, MAX_STR_LEN);
    ln_len = strnlen(ln, MAX_STR_LEN);
    node = book->students;
    for (index = 0; index < book->num_students; index++) {
        ASSERT(node);
        if ((fn_len == node->fn_len) && (ln_len == node->ln_len))
            if (strncmp(node->fn, fn, fn_len) == 0)
                if (strncmp(node->ln, ln, ln_len) == 0)
                    break;
        node = node->next;
    }
    ASSERT(index != book->num_students);
    ASSERT(node);

    // remove student from linked list
    book->num_students--;
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    if (index == 0)
        book->students = node->next;

    // remove student from grade grid
    if (book->num_assignments == 0) {
        ASSERT(book->grades == NULL);
        return;
    }
    if (book->num_students == 0) {
        book->grades = NULL;
        return;
    }

    ASSERT(book->grades);
    g = book->grades;
    for (i = 0; i < index; i++) {
        ASSERT(g);
        g = g->next_student;
    }
    while (g) {
        if (g->prev_student)
            g->prev_student->next_student = g->next_student;
        if (g->next_student)
            g->next_student->prev_student = g->prev_student;
        g = g->next_assignment;
    }
    if (index == 0)
        book->grades = book->grades->next_student;
}

void update_grade(Gradebook *book, char *fn, char *ln, char *a_name, char *grade) {
    Student *student;
    Assignment *assignment;
    Grade *g;
    int retval, a_index, a_len, s_index, fn_len, ln_len, g_val, i;

    // assert preconditions
    ASSERT(book);
    ASSERT(fn);
    ASSERT(ln);
    ASSERT(a_name);

    // parse grade value
    retval = sscanf(grade, "%d", &g_val);
    ASSERT(retval);
    ASSERT(g_val > 0);

    // find matching assignment
    a_len = strnlen(a_name, MAX_STR_LEN);
    assignment = book->assignments;
    for (a_index = 0; a_index < book->num_assignments; a_index++) {
        ASSERT(assignment);
        if (a_len == assignment->name_len)
            if (strncmp(assignment->name, a_name, a_len) == 0)
                break;
        assignment = assignment->next;
    }
    ASSERT(a_index != book->num_assignments);
    ASSERT(assignment);

    // find matching student
    fn_len = strnlen(fn, MAX_STR_LEN);
    ln_len = strnlen(ln, MAX_STR_LEN);
    student = book->students;
    for (s_index = 0; s_index < book->num_students; s_index++) {
        ASSERT(student);
        if ((fn_len == student->fn_len) && (ln_len == student->ln_len))
            if (strncmp(student->fn, fn, fn_len) == 0)
                if (strncmp(student->ln, ln, ln_len) == 0)
                    break;
        student = student->next;
    }
    ASSERT(s_index != book->num_students);
    ASSERT(student);

    // update grade in grid
    g = book->grades;
    ASSERT(g);
    for (i = 0; i < a_index; i++) {
        g = g->next_assignment;
        ASSERT(g);
    }
    for (i = 0; i < s_index; i++) {
        g = g->next_student;
        ASSERT(g);
    }
    g->grade = g_val;
}

void print_gradebook(Gradebook *book) {
    Student *s;
    Assignment *a;
    Grade *row, *col;

    // assert preconditions
    ASSERT(book);

    printf("--- GRADEBOOK ---\n");

    // print students
    printf("num_students: %d\n", book->num_students);
    s = book->students;
    while (s) {
        printf("  %.*s %.*s\n", s->fn_len, s->fn, s->ln_len, s->ln);
        s = s->next;
    }

    // print assignments
    printf("num_assignments: %d\n", book->num_assignments);
    a = book->assignments;
    while (a) {
        printf("  %.*s, %f, %d\n", a->name_len, a->name, a->weight, a->points);
        a = a->next;
    }

    // print grades
    row = book->grades;
    while (row) {
        col = row;
        while (col) {
            printf("%d ", col->grade);
            col = col->next_student;
        }
        printf("\n");
        row = row->next_assignment;
    }
    printf("\n");
}

