
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <regex.h>
#include "utils.h"


/*******************************************************************************
 *  Public Functions
 ******************************************************************************/

void terminate() {
    printf("invalid\n");
    exit(ERRCODE_255);
}

void varbuf_write(VarBuffer *meta, void *data, int size) {
    // assert preconditions
    ASSERT(meta);
    ASSERT(data);
    ASSERT(size > 0);

    // ensure sufficient space
    while (meta->index+size > meta->limit) {
        meta->limit += PAGE_SIZE;
        meta->buf = (uint8_t *)realloc(meta->buf, meta->limit);
        ASSERT(meta->buf);
    }

    // write data
    memcpy(&meta->buf[meta->index], data, size);
    meta->index += size;
}

void varbuf_read(VarBuffer *meta, void *data, int size) {
    // assert preconditions
    ASSERT(meta);
    ASSERT(data);
    ASSERT(size > 0);

    // ensure no overflow
    ASSERT(meta->index+size <= meta->limit);

    // write data
    memcpy(data, &meta->buf[meta->index], size);
    meta->index += size;
}

void gen_rand(uint8_t *buf, int size) {
    FILE *rand;

    // assert preconditions
    ASSERT(buf);

    rand = fopen("/dev/urandom", "r");
    ASSERT(rand != NULL);

    ASSERT(fread(buf, size, 1, rand));
    fclose(rand);
}

void validate_string(char *regex, char *str) {
    int retval;
    regex_t preg;

    retval = regcomp(&preg, regex, 0); //0x7fff
    ASSERT(retval != REG_BADBR);
    ASSERT(retval != REG_BADPAT);
    ASSERT(retval != REG_BADRPT);
    ASSERT(retval != REG_EBRACE);
    ASSERT(retval != REG_EBRACK);
    ASSERT(retval != REG_ECOLLATE);
    ASSERT(retval != REG_ECTYPE);
    ASSERT(retval != REG_EEND);
    ASSERT(retval != REG_EESCAPE);
    ASSERT(retval != REG_EPAREN);
    ASSERT(retval != REG_ERANGE);
    ASSERT(retval != REG_ESIZE);
    ASSERT(retval != REG_ESPACE);
    ASSERT(retval != REG_ESUBREG);
    ASSERT(retval == 0);

    retval = regexec(&preg, str, 0, NULL, 0);
    ASSERT(retval == 0);

    regfree(&preg);
}

void str2hex(char *str, uint8_t *buffer, int bytes) {
    int i;

    for (i = 0; i < bytes; i++)
        buffer[i] = gethex(&str[2*i]);
}

uint8_t gethex(char *str) {
    uint8_t retval;

    sscanf(str, "%02hhx", &retval);

    return retval;
}

