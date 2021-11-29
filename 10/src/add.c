
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include "utils.h"
#include "data.h"
#include "db.h"
#include "add.h"


/*******************************************************************************
 *  Prototypes
 ******************************************************************************/

static void validate_and_execute(void);


/*******************************************************************************
 *  Private Data
 ******************************************************************************/

static int got[NUM_ARGS] = {0};
static int got_action = 0;

static struct option opts[] = {
    {"N", required_argument, &got[ARG_N], 1},
    {"K", required_argument, &got[ARG_K], 1},
    {"AA", no_argument, &got[ARG_AA], 1},
    {"DA", no_argument, &got[ARG_DA], 1},
    {"AS", no_argument, &got[ARG_AS], 1},
    {"DS", no_argument, &got[ARG_DS], 1},
    {"AG", no_argument, &got[ARG_AG], 1},
    {"AN", required_argument, &got[ARG_AN], 1},
    {"P", required_argument, &got[ARG_P], 1},
    {"W", required_argument, &got[ARG_W], 1},
    {"FN", required_argument, &got[ARG_FN], 1},
    {"LN", required_argument, &got[ARG_LN], 1},
    {"G", required_argument, &got[ARG_G], 1},
    {0},
};

static char db_name[MAX_STR_LEN+1];
static char key_str[MAX_STR_LEN+1];
static char assignment_name[MAX_STR_LEN+1];
static char points_str[MAX_STR_LEN+1];
static char weight_str[MAX_STR_LEN+1];
static char first_name[MAX_STR_LEN+1];
static char last_name[MAX_STR_LEN+1];
static char grade_str[MAX_STR_LEN+1];

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
            case ARG_AA:
            case ARG_DA:
            case ARG_AS:
            case ARG_DS:
            case ARG_AG:
                ASSERT(got[ARG_K]);
                ASSERT(got_action == 0);
                got_action = 1;
                break;
            case ARG_AN:
                ASSERT(got_action);
                strncpy(assignment_name, optarg, MAX_STR_LEN);
                break;
            case ARG_P:
                ASSERT(got_action);
                strncpy(points_str, optarg, MAX_STR_LEN);
                break;
            case ARG_W:
                ASSERT(got_action);
                strncpy(weight_str, optarg, MAX_STR_LEN);
                break;
            case ARG_FN:
                ASSERT(got_action);
                strncpy(first_name, optarg, MAX_STR_LEN);
                break;
            case ARG_LN:
                ASSERT(got_action);
                strncpy(last_name, optarg, MAX_STR_LEN);
                break;
            case ARG_G:
                ASSERT(got_action);
                strncpy(grade_str, optarg, MAX_STR_LEN);
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
    if (got[ARG_AA]) {
        ASSERT(!got[ARG_FN]);
        ASSERT(!got[ARG_LN]);
        ASSERT(!got[ARG_G]);

        ASSERT(got[ARG_AN]);
        validate_string(REGEX_ASSIGNMENT, assignment_name);
        ASSERT(got[ARG_P]);
        validate_string(REGEX_INT, points_str);
        ASSERT(got[ARG_W]);
        validate_string(REGEX_FLOAT, weight_str);
        create_assignment(book, assignment_name, weight_str, points_str);
    } else if (got[ARG_DA]) {
        ASSERT(!got[ARG_FN]);
        ASSERT(!got[ARG_LN]);
        ASSERT(!got[ARG_P]);
        ASSERT(!got[ARG_W]);
        ASSERT(!got[ARG_G]);

        ASSERT(got[ARG_AN]);
        validate_string(REGEX_ASSIGNMENT, assignment_name);
        remove_assignment(book, assignment_name);
    } else if (got[ARG_AS]) {
        ASSERT(!got[ARG_AN]);
        ASSERT(!got[ARG_P]);
        ASSERT(!got[ARG_W]);
        ASSERT(!got[ARG_G]);

        ASSERT(got[ARG_FN]);
        validate_string(REGEX_NAME, first_name);
        ASSERT(got[ARG_LN]);
        validate_string(REGEX_NAME, last_name);
        create_student(book, first_name, last_name);
    } else if (got[ARG_DS]) {
        ASSERT(!got[ARG_AN]);
        ASSERT(!got[ARG_P]);
        ASSERT(!got[ARG_W]);
        ASSERT(!got[ARG_G]);

        ASSERT(got[ARG_FN]);
        validate_string(REGEX_NAME, first_name);
        ASSERT(got[ARG_LN]);
        validate_string(REGEX_NAME, last_name);
        remove_student(book, first_name, last_name);
    } else if (got[ARG_AG]) {
        ASSERT(!got[ARG_P]);
        ASSERT(!got[ARG_W]);

        ASSERT(got[ARG_FN]);
        validate_string(REGEX_NAME, first_name);
        ASSERT(got[ARG_LN]);
        validate_string(REGEX_NAME, last_name);
        ASSERT(got[ARG_AN]);
        validate_string(REGEX_ASSIGNMENT, assignment_name);
        ASSERT(got[ARG_G]);
        validate_string(REGEX_INT, grade_str);
        update_grade(book, first_name, last_name, assignment_name, grade_str);
    } else {
        ASSERT(0);
    }

    store_gradebook(db_name, book, key);
};

