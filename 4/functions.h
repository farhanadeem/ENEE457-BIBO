void encryptt(char *fp, char *ckey);
int decryptt(char * fp, unsigned char *ckey);
int file_test(char *filename);
unsigned char* datahex(char* string);
int check(char *str);
int checkAlpha(char *str);
int checkDigit(char *str);
int checkFloat(char *str);
void handleErrors(void);
