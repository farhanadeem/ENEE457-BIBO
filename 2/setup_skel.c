#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "data.h"
#include "mycrypto.h"

/* test whether the file exists */
bool file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

int main(int argc, char** argv) {
  FILE* fp;
  regex_t regex;
  int regResult;

  if (argc != 3) {
    printf("invalid\n");
    return (255);
  }

  if (strcmp(argv[1], "-N") != 0) {
    printf("invalid\n");
    return (255);
  }

  regResult = regcomp(&regex, "^[0-9A-za-z._]+$", REG_EXTENDED);
  if (regResult) {
    printf("invalid\n");
    return (255);
  }

  regResult = regexec(&regex, argv[2], 0, NULL, 0);
  if (regResult) {
    printf("invalid\n");
    return (255);
  }

  regfree(&regex);

  if (strlen(argv[2]) > 100) {
    printf("invalid\n");
    return (255);
  }

  if (file_test(argv[2])) {
    printf("invalid\n");
    return (255);
  }

  fp = fopen(argv[2], "w");
  if (fp == NULL) {
    printf("invalid\n");
    return (255);
  }

  //  create cryptographic key(s)

  int keyLEN = 32;  //256 bit key
  int ivLEN = 16;   //128 bit iv
  unsigned char key[keyLEN];
  int randResult;
  randResult = RAND_priv_bytes(key, keyLEN);
  if (randResult != 1) {
    printf("invalid\n");
    fclose(fp);
    return (255);
  }
  unsigned char hexKEY[2 * keyLEN + 1];
  for (int i = 0; i < keyLEN; i++)
    sprintf(&hexKEY[2 * i], "%.2x", key[i]);
  hexKEY[2 * keyLEN] = '\0';
  printf("Key is: %s\n", hexKEY);

  // IV generation
  unsigned char iv[ivLEN];
  randResult = RAND_bytes(iv, ivLEN);
  if (randResult != 1) {
    printf("invalid\n");
    fclose(fp);
    return (255);
  }
  unsigned char tag[ivLEN];

  unsigned char* emp = "empty";

  // encrypt emp, and store IV + tag + cipher
  int cipherLEN;
  unsigned char ciphertext[256];
  cipherLEN = encrypt(emp, strlen((unsigned char*)emp), hexKEY, iv, ciphertext, tag);
  unsigned char output[cipherLEN + (2*ivLEN)];
  memcpy(output, iv, ivLEN);
  memcpy(output + ivLEN,tag,ivLEN);
  memcpy(output + 2 * ivLEN, ciphertext, cipherLEN);

  // now write to file
  fwrite(output, sizeof(unsigned char), (2 * ivLEN + cipherLEN), fp);

  fclose(fp);
  return (0);
}
