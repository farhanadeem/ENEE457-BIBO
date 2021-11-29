
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <argp.h>
#include "utils.h"
#include "data.h"
#include "db.h"
#include "setup.h"


/*******************************************************************************
 *  Prototypes
 ******************************************************************************/

static error_t parse_opt(int key, char *arg, struct argp_state *state);
static void setup_gradebook(char *filename);
static void print_key(uint8_t *key);


/*******************************************************************************
 *  Private Data
 ******************************************************************************/

static const struct argp_option options[] = {
    {0, GRADEBOOK_NAME_ARG, "FILENAME", 0},
    {0},
};

static const struct argp argp = {
    options,
    parse_opt,
    0,
    0,
};


/*******************************************************************************
 *  Public Functions
 ******************************************************************************/

int main(int argc, char **argv) {
    unsigned flags = ARGP_NO_ERRS;
    int num_args, retval;

    ASSERT(argc == 3);

    retval = argp_parse(&argp, argc, argv, flags, &num_args, NULL);
    ASSERT(retval == 0);

    return 0;
}


/*******************************************************************************
 *  Private Functions
 ******************************************************************************/

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    switch (key) {
        case GRADEBOOK_NAME_ARG:
            setup_gradebook(arg);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static void setup_gradebook(char *filename) {
    Gradebook book;
    uint8_t key[KEY_BYTES];
    char path[MAX_PATH_LEN+1];

    // assert preconditions
    ASSERT(filename);

    // ensure path is null-terminated
    memset(path, 0, sizeof(path));
    strncpy(path, filename, MAX_PATH_LEN);

    // validate legal filename characters
    validate_string(REGEX_FILENAME, path);

    // initialize empty gradebook
    book.num_students = 0;
    book.students = NULL;
    book.num_assignments = 0;
    book.assignments = NULL;
    book.grades = NULL;

    // store encrypted gradebook to file
    gen_rand(key, KEY_BYTES);
    store_gradebook(filename, &book, key);
    print_key(key);
}

static void print_key(uint8_t *key) {
    int i;

    // assert preconditions
    ASSERT(key);

    for (i = 0; i < KEY_BYTES; i++)
        printf("%02x", key[i]);

    printf("\n");
}


