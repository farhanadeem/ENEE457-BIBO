#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wordexp.h>

#include "data.h"
#include "mycrypto.h"

typedef struct _CmdLineResult {
  unsigned char *gbName;
  unsigned char *key;
  unsigned char *mode;
  unsigned char **args;
  int good;
} CmdLineResult;

typedef struct _asnPrint {
  unsigned char *last;
  unsigned char *first;
  int grade;
} asnPrint;

typedef struct _stdPrint {
  unsigned char *name;
  int grade;
} stdPrint;

typedef struct _finPrint {
  unsigned char *last;
  unsigned char *first;
  float grade;
} finPrint;

bool file_test(char *filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

CmdLineResult parse_cmdline(int argc, char *argv[]) {
  CmdLineResult R;
  regex_t regexGB, regexKEY, regexAN, regexSN;
  int regResult, rgGB, rgKEY, rgAN, rgSN;

  R.good = 0;

  if (argc == 1) {
    printf("invalid\n");
  }

  rgGB = regcomp(&regexGB, "^[0-9A-za-z._]+$", REG_EXTENDED);
  rgKEY = regcomp(&regexKEY, "^[0-9a-f]+$", REG_EXTENDED);
  rgAN = regcomp(&regexAN, "^[0-9a-zA-Z]+$", REG_EXTENDED);
  rgSN = regcomp(&regexSN, "^[A-Za-z]+$", REG_EXTENDED);

  if (rgGB || rgKEY || rgAN || rgSN) {
    printf("invalid\n");
    R.good = -1;
  }

  if (argc >= 7) {
    if (strcmp(argv[1], "-N") != 0 || strcmp(argv[3], "-K") != 0) {
      R.good = -1;
      printf("invalid\n");
    }
    regResult = regexec(&regexGB, argv[2], 0, NULL, 0);
    if (regResult || strlen(argv[2]) > 100) {
      printf("invalid\n");
      R.good = -1;
    }
    regResult = regexec(&regexKEY, argv[4], 0, NULL, 0);
    if (regResult || strlen(argv[4]) != 64) {
      printf("invalid\n");
      R.good = -1;
    }

    R.gbName = argv[2];
    R.key = argv[4];

    if (strcmp(argv[5], "-PA") == 0) {
      R.mode = argv[5];
      unsigned char **input = (unsigned char **)malloc(2 * sizeof(unsigned char *));
      int set = 0;
      int modeset = 0;
      // expect -AN, and -A/-G flags
      for (int i = 6; i < argc; i++) {
        if (strcmp(argv[i], "-AN") == 0) {
          if (i + 1 < argc) {
            ++i;
            regResult = regexec(&regexAN, argv[i], 0, NULL, 0);
            if (regResult || strlen(argv[i]) > 30) {
              printf("invalid\n");
              R.good = -1;
            }
            input[0] = argv[i];
          } else {
            printf("invalid\n");
            R.good = -1;
          }
          set = set | 1;
        } else if (strcmp(argv[i], "-A") == 0) {
          if (modeset == 1) {
            if (strcmp(input[1], "-G") == 0) {
              printf("invalid\n");
              R.good = -1;
            }
          }
          input[1] = argv[i];
          modeset = 1;
          set = set | (1 << 1);
        } else if (strcmp(argv[i], "-G") == 0) {
          if (modeset == 1) {
            if (strcmp(input[1], "-A") == 0) {
              printf("invalid\n");
              R.good = -1;
            }
          }
          modeset = 1;
          input[1] = argv[i];
          set = set | (1 << 1);
        } else {
          printf("invalid\n");
          R.good = -1;
        }
      }
      if (set == 3) {
        R.args = input;
      } else {
        printf("invalid\n");
        R.good = -1;
      }
    } else if (strcmp(argv[5], "-PS") == 0) {
      R.mode = argv[5];
      unsigned char **input = (unsigned char **)malloc(2 * sizeof(unsigned char *));
      int set = 0;
      // expect -FN, -LN
      for (int i = 6; i < argc - 1; i += 2) {
        if (strcmp(argv[i], "-FN") == 0) {
          regResult = regexec(&regexSN, argv[i + 1], 0, NULL, 0);
          if (regResult || strlen(argv[i + 1]) > 30) {
            printf("invalid\n");
            R.good = -1;
          } else {
            input[0] = argv[i + 1];
          }
          set = set | 1;
        } else if (strcmp(argv[i], "-LN") == 0) {
          regResult = regexec(&regexSN, argv[i + 1], 0, NULL, 0);
          if (regResult || strlen(argv[i + 1]) > 30) {
            printf("invalid\n");
            R.good = -1;
          } else {
            input[1] = argv[i + 1];
          }
          set = set | (1 << 1);
        } else {
          printf("invalid\n");
          R.good = -1;
        }
      }
      if (set == 3) {
        R.args = input;
      } else {
        printf("invalid\n");
        R.good = -1;
      }
    } else if (strcmp(argv[5], "-PF") == 0) {
      R.mode = argv[5];
      unsigned char **input = (unsigned char **)malloc(1 * sizeof(unsigned char *));
      int set = 0;
      int modeset = 0;
      // expect -A/-G
      for (int i = 6; i < argc; i++) {
        if (strcmp(argv[i], "-A") == 0) {
          if (modeset == 1) {
            if (strcmp(input[0], "-G") == 0) {
              printf("invalid\n");
              R.good = -1;
            }
          }
          input[0] = argv[i];
          modeset = 1;
          set = set | 1;
        } else if (strcmp(argv[i], "-G") == 0) {
          if (modeset == 1) {
            if (strcmp(input[0], "-A") == 0) {
              printf("invalid\n");
              R.good = -1;
            }
          }
          modeset = 1;
          input[0] = argv[i];
          set = set | 1;
        } else {
          printf("invalid\n");
          R.good = -1;
        }
      }
      if (set == 1) {
        R.args = input;
      } else {
        printf("invalid\n");
        R.good = -1;
      }
    } else {
      printf("invalid\n");
      R.good = -1;
    }
  } else {
    printf("invalid\n");
    R.good = -1;
  }

  regfree(&regexGB);
  regfree(&regexKEY);
  regfree(&regexAN);
  regfree(&regexSN);

  return R;
}

int compareNamePA(const void *a, const void *b) {
  const asnPrint *at = a;
  const asnPrint *bt = b;
  char *alast = at->last;
  char *blast = bt->last;
  if (strcmp(alast, blast) == 0) {
    return strcmp(at->first, bt->first);
  }
  return strcmp(alast, blast);
}

int compareGPA(const void *a, const void *b) {
  const asnPrint *at = a;
  const asnPrint *bt = b;
  int ag = at->grade;
  int bg = bt->grade;
  return bg - ag;
}

int compareNamePF(const void *a, const void *b) {
  const finPrint *at = a;
  const finPrint *bt = b;
  char *alast = at->last;
  char *blast = bt->last;
  if (strcmp(alast, blast) == 0) {
    return strcmp(at->first, bt->first);
  }
  return strcmp(alast, blast);
}

int compareGPF(const void *a, const void *b) {
  const finPrint *at = a;
  const finPrint *bt = b;
  float ag = at->grade;
  float bg = bt->grade;
  return ag - bg;
}

void print_Assignment(Gradebook *G, Assignment *assignment, char *order) {
  asnPrint tuples[G->rows];
  for (int i = 0; i < G->rows; i++) {
    asnPrint student;
    int splits = 2;
    char delim[] = " ";
    unsigned char copy[62];
    unsigned char *last = malloc(31 * sizeof(unsigned char));
    unsigned char *first = malloc(31 * sizeof(unsigned char));
    strcpy(copy, G->names[i]);
    unsigned char *ptr = strtok(copy, delim);
    strcpy(first, ptr);
    ptr = strtok(NULL, delim);
    strcpy(last, ptr);
    student.first = first;
    student.last = last;
    student.grade = assignment->grades[i];
    tuples[i] = student;
  }
  if (strcmp(order, "-A") == 0) {
    qsort(tuples, G->rows, sizeof(asnPrint), compareNamePA);
  } else {
    qsort(tuples, G->rows, sizeof(asnPrint), compareGPA);
  }
  for (int i = 0; i < G->rows; i++) {
    printf("(%s, %s, %d)\n", tuples[i].last, tuples[i].first, tuples[i].grade);
    free(tuples[i].first);
    free(tuples[i].last);
  }
  return;
}

void print_Student(Gradebook *G, char *name, int index) {
  stdPrint tuples[G->assnCount];
  int count = 0;
  for (int i = 0; i < G->assnCount; i++) {
    if (G->assignments[i]->grades[index] != -1) {
      stdPrint asn;
      unsigned char *name = malloc(31 * sizeof(unsigned char));
      strcpy(name, G->assignments[i]->name);
      asn.name = name;
      asn.grade = G->assignments[i]->grades[index];
      tuples[i] = asn;
      count++;
    }
  }

  if (count == 0) {
    printf("invalid\n");
  }
  for (int i = 0; i < count; i++) {
    printf("(%s, %d)\n", tuples[i].name, tuples[i].grade);
    free(tuples[i].name);
  }
  return;
}

void print_Final(Gradebook *G, char *order) {
  finPrint tuples[G->rows];
  int count = 0;
  for (int i = 0; i < G->rows; i++) {
    finPrint student;
    int splits = 2;
    char delim[] = " ";
    unsigned char copy[62];
    unsigned char *last = malloc(31 * sizeof(unsigned char));
    unsigned char *first = malloc(31 * sizeof(unsigned char));
    strcpy(copy, G->names[i]);
    unsigned char *ptr = strtok(copy, delim);
    strcpy(first, ptr);
    ptr = strtok(NULL, delim);
    strcpy(last, ptr);
    student.first = first;
    student.last = last;
    float grade = 0.0;
    for (int j = 0; j < G->assnCount; j++) {
      Assignment *A = G->assignments[j];
      int gd = A->grades[i];
      if (gd == -1) {
        gd = 0;
      }
      int pts = A->pts;
      grade += A->weight * ((float)gd / pts);
    }
    student.grade = grade;
    tuples[i] = student;
  }
  if (strcmp(order, "-A") == 0) {
    qsort(tuples, G->rows, sizeof(finPrint), compareNamePF);
  } else {
    qsort(tuples, G->rows, sizeof(finPrint), compareGPF);
  }
  for (int i = 0; i < G->rows; i++) {
    printf("(%s, %s, %g)\n", tuples[i].last, tuples[i].first, tuples[i].grade);
    free(tuples[i].first);
    free(tuples[i].last);
  }
  return;
}

int main(int argc, char *argv[]) {
  CmdLineResult R;
  int r = 0;

  R = parse_cmdline(argc, argv);

  if (R.good == 0) {
    Buffer *IV = malloc(sizeof(Buffer));
    Buffer *TAG = malloc(sizeof(Buffer));
    grab_IV(IV, TAG, R.gbName);
    Buffer *cipher = malloc(sizeof(Buffer));
    read_from_path(cipher, R.gbName, R.key);
    Buffer decText;
    unsigned char decbuf[cipher->Length];
    decText.Buf = decbuf;
    decText.Length = decrypt(cipher->Buf, cipher->Length, R.key, IV->Buf, decText.Buf, TAG->Buf);
    decText.Buf[decText.Length] = '\0';

    //printf("deciphered plaintext: %s\n", decText.Buf);

    free(IV->Buf);
    free(IV);
    free(TAG->Buf);
    free(TAG);
    free(cipher->Buf);
    free(cipher);

    Buffer output;

    if (strncmp(decText.Buf, "empty", 5) == 0) {
      printf("invalid\n");
      r = 255;
    } else {
      Gradebook *G = (Gradebook *)malloc(sizeof(Gradebook));
      get_Gradebook(G, &decText);

      if (strcmp(R.mode, "-PA") == 0) {
        if (G->assnCount > 0 && G->rows > 0) {
          int ran = 0;
          for (int i = 0; i < G->assnCount; i++) {
            if (strcmp(R.args[0], G->assignments[i]->name) == 0) {
              print_Assignment(G, G->assignments[i], R.args[1]);
              ran = 1;
            }
          }
          if (ran == 0) {
            printf("invalid\n");
            r = 255;
          }
        } else {
          printf("invalid\n");
          r = 255;
        }
      } else if (strcmp(R.mode, "-PS") == 0) {
        if (G->rows > 0 && G->assnCount > 0) {
          int len = 62;
          unsigned char *name = malloc(len * sizeof(unsigned char));
          strcpy(name, R.args[0]);
          strcat(name, " ");
          strcat(name, R.args[1]);
          int index = -1;
          for (int i = 0; i < G->rows; i++) {
            if (strcmp(name, G->names[i]) == 0) {
              index = i;
              print_Student(G, name, index);
            }
          }
          if (index == -1) {
            printf("invalid\n");
            r = 255;
          }
        } else {
          printf("invalid\n");
          r = 255;
        }
      } else if (strcmp(R.mode, "-PF") == 0) {
        if (G->rows > 0 && G->assnCount > 0) {
          print_Final(G, R.args[0]);
        } else {
          printf("invalid\n");
        }
      }

      for (int i = 0; i < G->assnCount; i++) {
        free(G->assignments[i]->name);
        free(G->assignments[i]->grades);
        free(G->assignments[i]);
      }
      if (G->assignments != NULL) {
        free(G->assignments);
      }
      for (int i = 0; i < G->rows; i++) {
        free(G->names[i]);
      }
      if (G->names != NULL) {
        free(G->names);
      }
      free(G);
    }
  } else {
    printf("invalid\n");
    r = 255;
  }

  free(R.args);
  return r;
}
