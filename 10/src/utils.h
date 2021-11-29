#ifndef UTILS_H
#define UTILS_H


#include <stdint.h>


#define ERRCODE_255 255
#define MAX_STR_LEN 255
#define PAGE_SIZE 4096

#define REGEX_FILENAME "^\\([[:alnum:]]\\|[_.]\\)\\+$"
#define REGEX_KEY "^[0-9a-f]\\{64\\}$"
#define REGEX_ASSIGNMENT "^[a-zA-Z0-9]\\+$"
#define REGEX_INT "^[0-9]\\+$"
#define REGEX_FLOAT "^[0-9]\\+\\(\\.[0-9]\\+\\)\\?$"
#define REGEX_NAME "^[a-zA-Z]\\+$"

//#define ASSERT(expr) if (!(expr)) {printf("%s:%d\n", __FILE__, __LINE__);terminate();}
#define ASSERT(expr) if (!(expr)) terminate();


typedef struct {
    uint8_t *buf;
    int index;
    int limit;
} VarBuffer;


void terminate(void);
void varbuf_write(VarBuffer *meta, void *data, int size);
void varbuf_read(VarBuffer *meta, void *data, int size);
void gen_rand(uint8_t *buf, int size);
void validate_string(char *regex, char *str);
void str2hex(char *str, uint8_t *buffer, int size);
uint8_t gethex(char *str);


#endif

