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

bool file_test(char *filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

/*
  This function verifies that the passed in arguments are okay. It sanitizes
  all of them to ensure they don't use weird characters. It also ensures that 
  enough arguments are passed in to actually do something. It makes sure that
  things with a custom length aren't exceedingly large within reasonable
  boundaries. It makes sure arguments are passed in the expected order, with
  the exception of the last set of arguments. If the user inputs improper
  characters within that last set of arguments then it will fail prematurely.
*/
CmdLineResult parse_cmdline(int argc, char *argv[]) {
  CmdLineResult R;
  regex_t regexGB, regexKEY, regexAN, regexAP, regexAW, regexSN;
  int regResult, rgGB, rgKEY, rgAN, rgAP, rgAW, rgSN;

  R.good = 0;

  if (argc == 1) {
    printf("invalid\n");
  }

  rgGB = regcomp(&regexGB, "^[0-9A-za-z._]+$", REG_EXTENDED);
  rgKEY = regcomp(&regexKEY, "^[0-9a-f]+$", REG_EXTENDED);
  rgAN = regcomp(&regexAN, "^[0-9a-zA-Z]+$", REG_EXTENDED);
  rgAP = regcomp(&regexAP, "^[0-9]+$", REG_EXTENDED);
  rgAW = regcomp(&regexAW, "^(0([.0-9]+)?|1([.]0)?)$", REG_EXTENDED);
  rgSN = regcomp(&regexSN, "^[A-Za-z]+$", REG_EXTENDED);

  if (rgGB || rgKEY || rgAN || rgAP || rgAW || rgSN) {
    printf("invalid\n");
    R.good = -1;
  }

  if (argc >= 7) {
    if (strcmp(argv[1], "-N") != 0 || strcmp(argv[3], "-K") != 0) {
      R.good = -1;
      printf("invalid\n");
    }
    regResult = regexec(&regexGB, argv[2], 0, NULL, 0);
    if (regResult != 0 || strlen(argv[2]) > 100) {
      printf("invalid\n");
      R.good = -1;
    }
    regResult = regexec(&regexKEY, argv[4], 0, NULL, 0);
    if (regResult != 0 || strlen(argv[4]) != 64) {
      printf("invalid\n");
      R.good = -1;
    }

    R.gbName = argv[2];
    R.key = argv[4];

    if (strcmp(argv[5], "-AA") == 0) {
      R.mode = argv[5];
      unsigned char **input = (unsigned char **)malloc(3 * sizeof(unsigned char *));
      int set = 0;
      // expect -AN, -P, and -W flags
      for (int i = 6; i < argc - 1; i += 2) {
        if (strcmp(argv[i], "-AN") == 0) {
          regResult = regexec(&regexAN, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 30) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[0] = argv[i + 1];
          }
          set = set | 1;
        } else if (strcmp(argv[i], "-P") == 0) {
          regResult = regexec(&regexAP, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 5) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[1] = argv[i + 1];
          }
          set = set | (1 << 1);
        } else if (strcmp(argv[i], "-W") == 0) {
          regResult = regexec(&regexAW, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 5) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[2] = argv[i + 1];
          }
          set = set | (1 << 2);
        } else {
          printf("invalid\n");
          R.good = -1;
        }
      }
      if (set == 7) {
        R.args = input;
      } else {
        printf("invalid\n");
        R.good = -1;
      }
    } else if (strcmp(argv[5], "-DA") == 0) {
      R.mode = "-DA";
      unsigned char **input = (unsigned char **)malloc(1 * sizeof(unsigned char *));
      int set = 0;
      // expect -AN
      for (int i = 6; i < argc - 1; i += 2) {
        if (strcmp(argv[i], "-AN") == 0) {
          regResult = regexec(&regexAN, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 30) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[0] = argv[i + 1];
          }
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
    } else if (strcmp(argv[5], "-AS") == 0) {
      R.mode = "-AS";
      unsigned char **input = (unsigned char **)malloc(2 * sizeof(unsigned char *));
      int set = 0;
      // expect -FN, -LN
      for (int i = 6; i < argc - 1; i += 2) {
        if (strcmp(argv[i], "-FN") == 0) {
          regResult = regexec(&regexSN, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 30) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[0] = argv[i + 1];
          }
          set = set | 1;
        } else if (strcmp(argv[i], "-LN") == 0) {
          regResult = regexec(&regexSN, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 30) {
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
    } else if (strcmp(argv[5], "-DS") == 0) {
      R.mode = "-DS";
      unsigned char **input = (unsigned char **)malloc(2 * sizeof(unsigned char *));
      int set = 0;
      // expect -FN, -LN
      for (int i = 6; i < argc - 1; i += 2) {
        if (strcmp(argv[i], "-FN") == 0) {
          regResult = regexec(&regexSN, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 30) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[0] = argv[i + 1];
          }
          set = set | 1;
        } else if (strcmp(argv[i], "-LN") == 0) {
          regResult = regexec(&regexSN, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 30) {
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
    } else if (strcmp(argv[5], "-AG") == 0) {
      R.mode = "-AG";
      unsigned char **input = (unsigned char **)malloc(4 * sizeof(unsigned char *));
      int set = 0;
      // expect -AN, -P, and -W flags
      for (int i = 6; i < argc - 1; i += 2) {
        if (strcmp(argv[i], "-FN") == 0) {
          regResult = regexec(&regexSN, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 30) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[0] = argv[i + 1];
          }
          set = set | 1;
        } else if (strcmp(argv[i], "-LN") == 0) {
          regResult = regexec(&regexSN, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 30) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[1] = argv[i + 1];
          }
          set = set | (1 << 1);
        } else if (strcmp(argv[i], "-AN") == 0) {
          regResult = regexec(&regexAN, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 30) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[2] = argv[i + 1];
          }
          set = set | (1 << 2);
        } else if (strcmp(argv[i], "-G") == 0) {
          regResult = regexec(&regexAP, argv[i + 1], 0, NULL, 0);
          if (regResult != 0 || strlen(argv[i + 1]) > 5) {
            printf("invalid\n");
            R.good = -1;

          } else {
            input[3] = argv[i + 1];
          }
          set = set | (1 << 3);
        } else {
          printf("invalid\n");
          R.good = -1;
        }
      }
      if (set == 15) {
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
  regfree(&regexAP);
  regfree(&regexAW);
  regfree(&regexSN);

  return R;
}

int main(int argc, char *argv[]) {
  int r = 0;
  CmdLineResult R;

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

    Buffer *output = malloc(sizeof(Buffer));

    if (strncmp(decText.Buf, "empty", 5) == 0) {
      // then new file! write contents
      if (strcmp(R.mode, "-DA") == 0 || strcmp(R.mode, "-DS") == 0 || strcmp(R.mode, "-AG") == 0) {
        free(R.args);
        printf("invalid\n");
        r = 255;
      } else {
        if (strcmp(R.mode, "-AA") == 0) {
          Gradebook G;
          G.names = NULL;
          G.assnCount = 1;
          G.rows = 0;
          Assignment *A = malloc(sizeof(Assignment));
          char *name = malloc(30 * sizeof(char));
          strcpy(name, R.args[0]);
          A->name = name;
          A->grades = NULL;
          A->pts = atoi(R.args[1]);
          A->weight = atof(R.args[2]);
          Assignment **asns = malloc(G.assnCount * sizeof(Assignment *));
          asns[0] = A;
          G.assignments = asns;
          print_gradebook(output, &G);
          for (int i = 0; i < G.assnCount; i++) {
            free(G.assignments[i]->name);
            free(G.assignments[i]);
          }
          free(G.assignments);
        } else if (strcmp(R.mode, "-AS") == 0) {
          Gradebook G = {0};
          G.rows = 1;
          G.assignments = NULL;
          G.assnCount = 0;
          unsigned char name[62];
          strcpy(name, R.args[0]);
          name[strlen(R.args[0])] = ' ';
          strcat(name, R.args[1]);
          unsigned char **names = malloc(G.rows * sizeof(unsigned char *));
          names[0] = name;
          G.names = names;
          print_gradebook(output, &G);

          free(names);
        }
      }

      write_to_path(R.gbName, output, R.key);
      free(output->Buf);
      free(output);
    } else {
      // TODO: Go through all modes and do error cases defined in the docs
      // already a file, lets find the mode
      Gradebook *G = (Gradebook *)malloc(sizeof(Gradebook));
      get_Gradebook(G, &decText);

      if (strcmp(R.mode, "-AA") == 0) {
        //add assignment mode
        Assignment **asns = malloc((G->assnCount + 1) * sizeof(Assignment *));
        float totWeight = atof(R.args[2]);
        for (int i = 0; i < G->assnCount; i++) {
          totWeight += G->assignments[i]->weight;
          if (strcmp(G->assignments[i]->name, R.args[0]) == 0 || totWeight > 1.0) {
            free(asns);
            printf("invalid\n");
            r = 255;
          }
          asns[i] = G->assignments[i];
        }
        Assignment *A = malloc(sizeof(Assignment));
        unsigned char *aname = malloc(31 * sizeof(unsigned char));
        strcpy(aname, R.args[0]);
        A->name = aname;
        A->pts = atoi(R.args[1]);
        A->weight = atof(R.args[2]);
        if (G->rows > 0) {
          int *grades = malloc(G->rows * sizeof(int));
          for (int j = 0; j < G->rows; j++) {
            grades[j] = -1;
          }
          A->grades = grades;
        } else {
          A->grades = NULL;
        }
        asns[G->assnCount] = A;
        G->assignments = asns;
        G->assnCount += 1;
      } else if (strcmp(R.mode, "-DA") == 0) {
        //delete assignment case
        Assignment **asns = malloc((G->assnCount - 1) * sizeof(Assignment *));
        int j = 0;
        int deleted = 0;
        for (int i = 0; i < G->assnCount; i++) {
          if (strcmp(R.args[0], G->assignments[i]->name) != 0) {
            asns[j] = G->assignments[i];
            j++;
          } else {
            deleted = 1;
            free(G->assignments[i]->grades);
            free(G->assignments[i]);
          }
        }
        G->assignments = asns;
        G->assnCount -= 1;
        if (deleted == 0) {
          free(asns);
          printf("invalid\n");
          r = 255;
        }
      } else if (strcmp(R.mode, "-AS") == 0) {
        //add student mode
        int len = 62;  // max size (overkill?)
        unsigned char *name = malloc(len * sizeof(unsigned char));
        strcpy(name, R.args[0]);
        strcat(name, " ");
        strcat(name, R.args[1]);
        unsigned char **names = malloc((G->rows + 1) * sizeof(unsigned char *));
        if (G->rows > 0) {
          for (int i = 0; i < G->rows; i++) {
            if (strcmp(G->names[i], name) == 0) {
              free(name);
              free(names);
              printf("invalid\n");
              r = 255;
            }
            names[i] = G->names[i];
          }
          free(G->names);
        }

        names[G->rows] = name;
        G->names = names;
        // update assignment lists
        for (int i = 0; i < G->assnCount; i++) {
          int *grades = malloc((G->rows + 1) * sizeof(int));
          // copy grades that are there
          for (int k = 0; k < G->rows; k++) {
            grades[k] = G->assignments[i]->grades[k];
          }
          grades[G->rows] = -1;
          if (G->assignments[i]->grades != NULL) {
            free(G->assignments[i]->grades);
          }
          G->assignments[i]->grades = grades;
        }
        G->rows += 1;
      } else if (strcmp(R.mode, "-DS") == 0) {
        unsigned char **names = malloc((G->rows + 1) * sizeof(unsigned char *));
        //delete student mode
        if (G->rows > 0) {
          int len = 62;
          unsigned char *name = malloc(len * sizeof(unsigned char));
          strcpy(name, R.args[0]);
          strcat(name, " ");
          strcat(name, R.args[1]);
          if (G->rows == 1 && strcmp(name, G->names[0]) == 0) {
            free(G->names);
            G->names = NULL;
            G->rows = 0;
            for (int i = 0; i < G->assnCount; i++) {
              free(G->assignments[i]->grades);
              G->assignments[i]->grades = NULL;
            }
          } else if (G->rows == 1) {
            free(name);
            free(names);
            printf("invalid\n");
            r = 255;
          } else {
            int index = -1;
            int j = 0;
            // remove student if found from names list
            for (int i = 0; i < G->rows; i++) {
              if (strcmp(G->names[i], name) != 0) {
                names[j] = G->names[i];
                j++;
              } else {
                index = i;
                free(G->names[i]);
                free(G->names);
              }
            }
            G->names = names;
            // if we deleted a student, update asns grades lists
            if (index != -1) {
              G->rows -= 1;
              for (int i = 0; i < G->assnCount; i++) {
                int *grades = malloc(G->rows * sizeof(int));
                int j = 0;
                // copy grades that do exist
                for (int k = 0; k < G->rows; k++) {
                  if (k != index) {
                    grades[j] = G->assignments[i]->grades[k];
                    j++;
                  }
                }
                free(G->assignments[i]->grades);
                G->assignments[i]->grades = grades;
              }
            } else {
              free(name);
              free(names);
              printf("invalid\n");
              r = 255;
            }
          }
        } else {
          free(names);
          printf("invalid\n");
          r = 255;
        }
      } else if (strcmp(R.mode, "-AG") == 0) {
        //Add grade mode
        if (G->rows > 0 && G->assnCount > 0) {
          int len = 62;
          unsigned char *name = malloc(len * sizeof(unsigned char));
          strcpy(name, R.args[0]);
          strcat(name, " ");
          strcat(name, R.args[1]);
          int index = -1;
          // find the index of the student
          for (int i = 0; i < G->rows; i++) {
            if (strcmp(name, G->names[i]) == 0) {
              index = i;
              break;
            }
          }
          // find the assignment and update grade
          if (index != -1) {
            int updated = 0;
            for (int i = 0; i < G->assnCount; i++) {
              if (strcmp(R.args[2], G->assignments[i]->name) == 0) {
                G->assignments[i]->grades[index] = atoi(R.args[3]);
                updated = 1;
              }
            }
            if (updated == 0) {
              free(name);
              printf("invalid\n");
              r = 255;
            }
          } else {
            printf("invalid\n");
            r = 255;
          }

        } else {
          printf("invalid\n");
          r = 255;
        }
      } else {
        printf("invalid\n");
        r = 255;
      }  // end of mode handling

      // now write the new gradebook to file bb
      print_gradebook(output, G);
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
      write_to_path(R.gbName, output, R.key);
      free(output->Buf);
      free(output);
    }
  } else {
    printf("invalid\n");
    r = 255;
  }

  free(R.args);
  return r;
}
