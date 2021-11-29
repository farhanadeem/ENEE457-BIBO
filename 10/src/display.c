
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include "utils.h"
#include "data.h"
#include "db.h"
#include "print.h"
#include "display.h"


/*******************************************************************************
 *  Prototypes
 ******************************************************************************/

static void validate_and_execute(void);


/*******************************************************************************
 *  Private Data
 ******************************************************************************/

static int got[NUM_ARGS] = {0};
static int got_action = 0;
static int got_sort = 0;
static int sort;

static struct option opts[] = {
    {"N", required_argument, &got[ARG_N], 1},
    {"K", required_argument, &got[ARG_K], 1},
    {"PA", no_argument, &got[ARG_PA], 1},
    {"PS", no_argument, &got[ARG_PS], 1},
    {"PF", no_argument, &got[ARG_PF], 1},
    {"AN", required_argument, &got[ARG_AN], 1},
    {"FN", required_argument, &got[ARG_FN], 1},
    {"LN", required_argument, &got[ARG_LN], 1},
    {"A", no_argument, &got[ARG_A], 1},
    {"G", no_argument, &got[ARG_G], 1},
    {0},
};

static char db_name[MAX_STR_LEN+1];
static char key_str[MAX_STR_LEN+1];
static char assignment_name[MAX_STR_LEN+1];
static char first_name[MAX_STR_LEN+1];
static char last_name[MAX_STR_LEN+1];

static uint8_t key[KEY_BYTES];


/*******************************************************************************
 *  Public Functions
 ******************************************************************************/

int main(int argc, char **argv) {
    int arg_index, retval;

    // disable error messages
    opterr = 0;

    // parse options
    while ((retval = getopt_long_only(argc, argv, "", opts, &arg_index)) != -1) {
        ASSERT(retval != '?');

        switch (arg_index) {
            case ARG_N:
                strncpy(db_name, optarg, MAX_STR_LEN);
                break;
            case ARG_K:
                ASSERT(got[ARG_N]);
                strncpy(key_str, optarg, MAX_STR_LEN);
                break;
            case ARG_PA:
            case ARG_PS:
            case ARG_PF:
                ASSERT(got[ARG_K]);
                ASSERT(got_action == 0);
                got_action = 1;
                break;
            case ARG_AN:
                ASSERT(got_action);
                strncpy(assignment_name, optarg, MAX_STR_LEN);
                break;
            case ARG_FN:
                ASSERT(got_action);
                strncpy(first_name, optarg, MAX_STR_LEN);
                break;
            case ARG_LN:
                ASSERT(got_action);
                strncpy(last_name, optarg, MAX_STR_LEN);
                break;
            case ARG_A:
                ASSERT(got_action);
                ASSERT(!got[ARG_G]);
                got_sort = 1;
                sort = NAME_SORT;
                break;
            case ARG_G:
                ASSERT(got_action);
                ASSERT(!got[ARG_A]);
                got_sort = 1;
                sort = GRADE_SORT;
                break;
            default:
                break;
        }
    }

    // validate and execute requested action
    validate_and_execute();

    return 0;
}


/*******************************************************************************
 *  Private Functions
 ******************************************************************************/

static void validate_and_execute(void) {
    Gradebook *book;

    // ensure proper arguments were received
    ASSERT(got[ARG_N]);
    validate_string(REGEX_FILENAME, db_name);

    ASSERT(got[ARG_K]);
    validate_string(REGEX_KEY, key_str);

    ASSERT(got_action);

    // load key value and decrypt gradbook
    str2hex(key_str, key, KEY_BYTES);
    book = load_gradebook(db_name, key);

    // validate and execute requested action
    if (got[ARG_PA]) {
        ASSERT(!got[ARG_FN]);
        ASSERT(!got[ARG_LN]);

        ASSERT(got[ARG_AN]);
        validate_string(REGEX_ASSIGNMENT, assignment_name);
        ASSERT(got_sort);
        print_assignment(book, assignment_name, sort);
    } else if (got[ARG_PS]) {
        ASSERT(!got[ARG_AN]);
        ASSERT(!got_sort);

        ASSERT(got[ARG_FN]);
        validate_string(REGEX_NAME, first_name);
        ASSERT(got[ARG_LN]);
        validate_string(REGEX_NAME, last_name);
        print_student(book, first_name, last_name, sort);
    } else if (got[ARG_PF]) {
        ASSERT(!got[ARG_AN]);
        ASSERT(!got[ARG_FN]);
        ASSERT(!got[ARG_LN]);

        ASSERT(got_sort);
        print_final(book, sort);
    } else {
        ASSERT(0);
    }
}

