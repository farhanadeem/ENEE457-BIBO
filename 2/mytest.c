#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

int main() {
     /*char str[] = "project,50,0.25,-1,49,50\nJohn Smith,Carl Hoover";
    int splits = 2;
    char delim[] = "\n";

    char* ptr = strtok(str, delim);

    for (int i = 0; i < splits; i++) {
        printf("item: %s\n",ptr);
        ptr = strtok(NULL, delim);
    }
    */
   FILE *fp;
   char str[33];

   /* opening file for reading */
   fp = fopen("grades" , "r");
   if(fp == NULL) {
      perror("Error opening file");
      return(-1);
   }
   if( fgets (str, 33, fp)!=NULL ) {
      /* writing content to stdout */
      puts(str);
   }

   struct stat st;
  stat("grades", &st);
  // st.st_size is filesize, first 33 bytes are my IV and \n
  char* cipher = malloc(st.st_size - 32);
  fseek(fp, 33, SEEK_SET);
  fread(cipher, 1, st.st_size, fp);
  cipher[st.st_size -32] = '\0';

  printf("cipher: %s\n",cipher);

  char* num = "1";
  float f1 = atof(num);
  printf("float: %.2f\n",f1);
    return 0;
}