#ifndef DB_H
#define DB_H


#include "data.h"


#define KEY_BYTES 32
#define KEY_BITS (8*(KEY_BYTES))

#define IV_BYTES 12
#define IV_BITS (8*(KEY_BYTES))

#define TAG_BYTES 16
#define TAG_BITS (8*(KEY_BYTES))


Gradebook *load_gradebook(char *filename, uint8_t *key);
void store_gradebook(char *filename, Gradebook *book, uint8_t *key);


#endif

