
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/evp.h>
#include "utils.h"
#include "data.h"
#include "db.h"


static int flatten(Gradebook *book, uint8_t **buffer);
static Student *flatten_student(Student *student, VarBuffer *meta);
static Assignment *flatten_assignment(Assignment *assignment, VarBuffer *meta);

static Gradebook *unflatten(uint8_t *buffer, int size);
static Student *unflatten_student(Student *prev, VarBuffer *meta);
static Assignment *unflatten_assignment(Assignment *prev, VarBuffer *meta);
static Grade *unflatten_grades(VarBuffer *meta, int rows, int cols);

static void encrypt(char *filename, uint8_t *key, uint8_t *buffer, int size);
static int decrypt(char *filename, uint8_t *key, uint8_t **buffer);


/*******************************************************************************
 *  Public Functions
 ******************************************************************************/

Gradebook *load_gradebook(char *filename, uint8_t *key) {
    Gradebook *book;
    uint8_t *buffer;
    int size;

    // assert preconditions
    ASSERT(filename);

    // load the database
    size = decrypt(filename, key, &buffer);
    book = unflatten(buffer, size);

    return book;
}

void store_gradebook(char *filename, Gradebook *book, uint8_t *key) {
    int size;
    uint8_t *buffer;

    // assert preconditions
    ASSERT(filename);
    ASSERT(book);
    ASSERT(key);

    // store the database
    size = flatten(book, &buffer);
    encrypt(filename, key, buffer, size);
}


/*******************************************************************************
 *  Private Functions
 ******************************************************************************/

static int flatten(Gradebook *book, uint8_t **buffer) {
    VarBuffer meta;
    Student *student;
    Assignment *assignment;
    Grade *row, *col;
    int i;

    // assert preconditions
    ASSERT(book);
    ASSERT(buffer);

    // allocate buffer
    meta.index = 0;
    meta.limit = PAGE_SIZE;
    meta.buf = (uint8_t *)malloc(meta.limit);
    ASSERT(meta.buf);

    // write number of students
    varbuf_write(&meta, &book->num_students, sizeof(book->num_students));

    // write student block
    student = book->students;
    for (i = 0; i < book->num_students; i++)
        student = flatten_student(student, &meta);

    // write number of assignments
    varbuf_write(&meta, &book->num_assignments, sizeof(book->num_assignments));

    // write assignment block
    assignment = book->assignments;
    for (i = 0; i < book->num_assignments; i++)
        assignment = flatten_assignment(assignment, &meta);

    // write grade block
    row = book->grades;
    while (row) {
        col = row;
        while (col) {
            varbuf_write(&meta, &col->grade, sizeof(col->grade));
            col = col->next_student;
        }
        row = row->next_assignment;
    }

    // return the buffer and size
    *buffer = meta.buf;
    return meta.index;
}

static Student *flatten_student(Student *student, VarBuffer *meta) {
    // assert preconditions
    ASSERT(student);
    ASSERT(meta);

    // write first name length
    varbuf_write(meta, &student->fn_len, sizeof(student->fn_len));

    // write first name
    varbuf_write(meta, student->fn, student->fn_len);

    // write last name length
    varbuf_write(meta, &student->ln_len, sizeof(student->ln_len));

    // write last name
    varbuf_write(meta, student->ln, student->ln_len);

    return student->next;
}

static Assignment *flatten_assignment(Assignment *assignment, VarBuffer *meta) {
    // assert preconditions
    ASSERT(assignment);
    ASSERT(meta);

    // write weight
    varbuf_write(meta, &assignment->weight, sizeof(assignment->weight));

    // write points
    varbuf_write(meta, &assignment->points, sizeof(assignment->points));

    // write name length
    varbuf_write(meta, &assignment->name_len, sizeof(assignment->name_len));

    // write name
    varbuf_write(meta, assignment->name, assignment->name_len);

    return assignment->next;
}

static Gradebook *unflatten(uint8_t *buffer, int size) {
    VarBuffer meta;
    Gradebook *book;
    Student *student;
    Assignment *assignment;
    int i, num_grades;

    // assert preconditions
    ASSERT(buffer);
    ASSERT(size > 0);

    // initialize VarBuffer
    meta.buf = buffer;
    meta.index = 0;
    meta.limit = size;

    // initialize gradebook
    book = (Gradebook *)malloc(sizeof(*book));
    ASSERT(book);

    // initialize students
    varbuf_read(&meta, &book->num_students, sizeof(book->num_students));

    if (book->num_students == 0) {
        book->students = NULL;
    } else {
        student = NULL;
        for (i = 0; i < book->num_students; i++) {
            student = unflatten_student(student, &meta);
            if (i == 0)
                book->students = student;
        }
    }

    // initialize assignments
    varbuf_read(&meta, &book->num_assignments, sizeof(book->num_assignments));

    if (book->num_assignments == 0) {
        book->assignments = NULL;
    } else {
        assignment = NULL;
        for (i = 0; i < book->num_assignments; i++) {
            assignment = unflatten_assignment(assignment, &meta);
            if (i == 0)
                book->assignments = assignment;
        }
    }

    // initialize grades
    num_grades = book->num_students * book->num_assignments;
    ASSERT((meta.limit - meta.index) == (num_grades * sizeof(int)));

    book->grades = unflatten_grades(&meta, book->num_assignments, book->num_students);

    return book;
}

static Student *unflatten_student(Student *prev, VarBuffer *meta) {
    Student *next;

    // assert preconditions
    ASSERT(meta);

    // allocate next student
    next = (Student *)malloc(sizeof(*next));
    ASSERT(next);

    // initialize first name
    varbuf_read(meta, &next->fn_len, sizeof(next->fn_len));
    next->fn = (char *)malloc(next->fn_len);
    ASSERT(next->fn);
    varbuf_read(meta, next->fn, next->fn_len);

    // initialize last name
    varbuf_read(meta, &next->ln_len, sizeof(next->ln_len));
    next->ln = (char *)malloc(next->ln_len);
    ASSERT(next->ln);
    varbuf_read(meta, next->ln, next->ln_len);

    // initialize linked list pointers
    next->next = NULL;
    next->prev = prev;

    if (prev)
        prev->next = next;

    return next;
}

static Assignment *unflatten_assignment(Assignment *prev, VarBuffer *meta) {
    Assignment *next;

    // assert preconditions
    ASSERT(meta);

    // allocate next assignment
    next = (Assignment *)malloc(sizeof(*next));
    ASSERT(next);

    // initialize weight, points, and name
    varbuf_read(meta, &next->weight, sizeof(next->weight));
    varbuf_read(meta, &next->points, sizeof(next->points));
    varbuf_read(meta, &next->name_len, sizeof(next->name_len));
    next->name = (char *)malloc(next->name_len);
    ASSERT(next->name);
    varbuf_read(meta, next->name, next->name_len);

    // initialize linked list pointers
    next->next = NULL;
    next->prev = prev;

    if (prev)
        prev->next = next;

    return next;
}

static Grade *unflatten_grades(VarBuffer *meta, int rows, int cols) {
    Grade *base, *g;
    int i, j, size;

    // assert preconditions
    ASSERT(meta);

    // check for emptry grid of grades
    if ((rows * cols) == 0)
        return NULL;

    // allocate 2D array of Grades
    size = rows * cols * sizeof(Grade);
    base = (Grade *)malloc(size);
    ASSERT(base);
    memset(base, 0, size);

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            g = base + i*cols + j;
            varbuf_read(meta, &g->grade, sizeof(g->grade));
            if (i > 0)
                g->prev_assignment = &base[(i-1)*cols + j];
            if (j > 0)
                g->prev_student = &base[i*cols + j-1];
            if (i < rows-1)
                g->next_assignment = &base[(i+1)*cols + j];
            if (j < cols-1)
                g->next_student = &base[i*cols + j+1];
        }
    }

    return base;
}

static void encrypt(char *filename, uint8_t *key, uint8_t *buffer, int size) {
    EVP_CIPHER_CTX *ctx;
    uint8_t iv[IV_BYTES], tag[TAG_BYTES], *ciphertext, *rand_data;
    int retval, rand_len, len, final_len, rand_size;
    FILE *db_file;

    // assert preconditions
    ASSERT(filename);
    ASSERT(key);
    ASSERT(buffer);
    ASSERT(size > 0);

    // create randomized padding
    rand_data = (uint8_t *)malloc(256);
    ASSERT(rand_data);
    gen_rand(rand_data, 256);
    rand_size = rand_data[0]+1;

    // allocate buffer for ciphertext
    ciphertext = (uint8_t *)malloc(rand_size+size);
    ASSERT(ciphertext);

    // generate random IV
    gen_rand(iv, IV_BYTES);

    // setup AES-256 GCM cipher
    ctx = EVP_CIPHER_CTX_new();
    ASSERT(ctx);

    retval = EVP_EncryptInit(ctx, EVP_aes_256_gcm(), key, iv);
    ASSERT(retval);

    // encrypt randomized padding
    retval = EVP_EncryptUpdate(ctx, ciphertext, &rand_len, rand_data, rand_size);
    ASSERT(retval);

    // encrypt buffer
    retval = EVP_EncryptUpdate(ctx, ciphertext+rand_len, &len, buffer, size);
    ASSERT(retval);

    // finalize encryption
    retval = EVP_EncryptFinal(ctx, ciphertext+rand_len+len, &final_len);
    ASSERT(retval);

    // get the authentication tag
    retval = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_BYTES, tag);
    ASSERT(retval);

    // store the iv, tag, and ciphertext
    db_file = fopen(filename, "wb");
    ASSERT(db_file);

    ASSERT(fwrite(iv, IV_BYTES, 1, db_file));
    ASSERT(fwrite(tag, TAG_BYTES, 1, db_file));
    ASSERT(fwrite(ciphertext, rand_len+len+final_len, 1, db_file));

    free(ciphertext);
    fclose(db_file);
}

static int decrypt(char *filename, uint8_t *key, uint8_t **buffer) {
    EVP_CIPHER_CTX *ctx;
    uint8_t iv[IV_BYTES], tag[TAG_BYTES], *ciphertext, *plaintext;
    struct stat file_metadata;
    FILE *db_file;
    int rand_size, data_size, retval, len, final_len;

    // assert preconditions
    ASSERT(filename);
    ASSERT(key);
    ASSERT(buffer);

    // determine filesize
    retval = stat(filename, &file_metadata);
    ASSERT(retval == 0);
    data_size = file_metadata.st_size - (IV_BYTES + TAG_BYTES);

    // allocate buffers for ciphertext and plaintext
    ciphertext = malloc(data_size);
    plaintext = malloc(data_size);
    ASSERT(ciphertext);
    ASSERT(plaintext);

    // read the iv, tag, and ciphertext
    db_file = fopen(filename, "rb");
    ASSERT(db_file);

    ASSERT(fread(iv, IV_BYTES, 1, db_file));
    ASSERT(fread(tag, TAG_BYTES, 1, db_file));
    ASSERT(fread(ciphertext, data_size, 1, db_file));

    fclose(db_file);

    // setup AES-256 GCM cipher
    ctx = EVP_CIPHER_CTX_new();
    ASSERT(ctx);

    retval = EVP_DecryptInit(ctx, EVP_aes_256_gcm(), key, iv);
    ASSERT(retval);

    // decrypt buffer
    retval = EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, data_size);
    ASSERT(retval);

    // set expected tag value
    retval = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_BYTES, tag);
    ASSERT(retval);

    // finalize decryption and verify tag
    retval = EVP_DecryptFinal(ctx, plaintext + len, &final_len);
    ASSERT(retval > 0);
    ASSERT((len+final_len) == data_size);

    // discard randomized padding
    rand_size = plaintext[0] + 1;
    plaintext = plaintext + rand_size;

    // return buffer and size
    free(ciphertext);
    *buffer = plaintext;
    return data_size - rand_size;
}

