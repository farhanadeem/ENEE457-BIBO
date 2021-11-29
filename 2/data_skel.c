#include <fcntl.h>
#include <openssl/bio.h>
#include <openssl/comp.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "data.h"
#include "mycrypto.h"

int print_gradebook(Buffer *B, Gradebook *R) {
  char *buf = malloc(100 * sizeof(char));
  B->Length = 100;
  int index = 0;
  sprintf(buf, "%d,%d,", R->rows, R->assnCount);
  index = strlen(buf);
  int i = 0;
  for (i = 0; i < R->rows; i++) {
    char *name = R->names[i];
    if (index + strlen(name) + 2 > B->Length) {
      buf = realloc(buf, 100);
      B->Length += 100;
    }
    index += strlen(name);
    strcat(buf, name);
    strcat(buf, ",");
    index++;
  }

  for (i = 0; i < R->assnCount; i++) {
    Assignment *A = R->assignments[i];
    char *preamble = malloc(50 * sizeof(unsigned char));
    strcpy(preamble, A->name);
    char *nums = malloc(15 * sizeof(unsigned char));
    sprintf(nums, ",%d,%.3f,", A->pts, A->weight);
    strcat(preamble, nums);
    if (index + strlen(preamble) + 2 > B->Length) {
      buf = realloc(buf, 100);
      B->Length += 100;
    }
    index += strlen(preamble);
    strcat(buf, preamble);
    if (A->grades != NULL) {
      for (int j = 0; j < R->rows; j++) {
        char grade[6];
        sprintf(grade, "%d", A->grades[j]);
        if (index + strlen(grade) + 2 > B->Length) {
          buf = realloc(buf, 100);
          B->Length += 100;
        }
        index += strlen(grade);
        strcat(buf, grade);
        strcat(buf, ",");
        index++;
      }
    }
  }
  B->Length = index;
  B->Buf = buf;
  return 0;
}

void write_to_path(char *path, Buffer *B, unsigned char *key_data) {
  FILE *fp;
  fp = fopen(path, "w");
  if (fp == NULL) {
    printf("invalid\n");
    return;
  }
  // generate IV, encrypt
  int ivLEN = 16;
  unsigned char iv[ivLEN];
  int randResult = RAND_bytes(iv, ivLEN);
  if (randResult != 1) {
    printf("invalid\n");
    return;
  }
  unsigned char tag[ivLEN];

  int cipherLEN;
  unsigned char ciphertext[B->Length + 16];
  cipherLEN = encrypt(B->Buf, strlen((unsigned char *)B->Buf), key_data, iv, ciphertext, tag);

  unsigned char output[cipherLEN + (2*ivLEN)];
  memcpy(output, iv, ivLEN);
  memcpy(output + ivLEN,tag,ivLEN);
  memcpy(output + 2 * ivLEN, ciphertext, cipherLEN);

  // now write to file
  fwrite(output, sizeof(unsigned char), (2 * ivLEN + cipherLEN), fp);

  fclose(fp);
  return;
}

int grab_IV(Buffer *B, Buffer *T, char *path) {
  FILE *fp;
  int ivLEN = 16;
  unsigned char *fileIV = malloc(ivLEN * sizeof(unsigned char));
  unsigned char *tag = malloc(ivLEN * sizeof(unsigned char));
  fp = fopen(path, "r");
  if (fp == NULL) {
    printf("invalid\n");
    B->Length = -1;
    return -1;
  }
  fread(fileIV, sizeof(unsigned char), ivLEN, fp);
  fread(tag, sizeof(unsigned char), ivLEN, fp);
  fclose(fp);
  B->Buf = fileIV;
  B->Length = ivLEN;
  T->Buf = tag;
  T->Length = ivLEN;
  return 0;
}

int read_from_path(Buffer *B, char *path, unsigned char *key) {
  FILE *fp;
  fp = fopen(path, "rb");
  if (fp == NULL) {
    printf("invalid\n");
    B->Length = -1;
    return -1;
  }
  struct stat st;
  stat(path, &st);
  // st.st_size is filesize, first 32 IV + TAG
  unsigned char *cipher = malloc(st.st_size - 32);
  fseek(fp, 32, SEEK_SET);
  fread(cipher, sizeof(unsigned char), st.st_size, fp);

  B->Buf = cipher;
  B->Length = st.st_size - 32;
  return 0;
}

int get_Gradebook(Gradebook *R, Buffer *B) {
  int students, assnCount, len;

  char delim[] = ",";
  char *ptr = strtok(B->Buf, delim);
  students = atoi(ptr);
  ptr = strtok(NULL, delim);
  assnCount = atoi(ptr);

  R->rows = students;
  R->assnCount = assnCount;

  if (students > 0) {
    unsigned char **names = malloc(students * sizeof(unsigned char *));
    for (int i = 0; i < students; i++) {
      ptr = strtok(NULL, delim);
      len = 62;
      unsigned char *name = malloc(len * sizeof(unsigned char));
      strcpy(name, ptr);
      names[i] = name;
    }
    R->names = names;
  } else {
    R->names = NULL;
  }

  if (assnCount > 0) {
    Assignment **asns = malloc(assnCount * sizeof(Assignment));
    for (int i = 0; i < assnCount; i++) {
      Assignment *A = malloc(sizeof(Assignment));
      ptr = strtok(NULL, delim);
      len = strlen(ptr);
      unsigned char *aname = malloc(31 * sizeof(unsigned char));
      strcpy(aname, ptr);
      A->name = aname;
      A->pts = atoi(strtok(NULL, delim));
      A->weight = atof(strtok(NULL, delim));
      if (students > 0) {
        int *grades = malloc(students * sizeof(int));
        for (int j = 0; j < students; j++) {
          grades[j] = atoi(strtok(NULL, delim));
        }
        A->grades = grades;
      } else {
        A->grades = NULL;
      }
      asns[i] = A;
    }
    R->assignments = asns;
  } else {
    R->assignments = NULL;
  }

  return 0;
}
