#ifndef _BUFFR_H
#define _BUFFR_H

#define LEN 16
#define GBSIZE 100
/*gradebookadd FLAGS*/
typedef enum GBAddFlag{
  NoFlag,
  AA, 
  DA, 
  AS, 
  DS, 
  AG,/**/
  AN,/*Requirements start here*/
  FN,
  LN,
  P,
  W,
  G
} GBAddFlag;

typedef enum GBADDCMD{
  NOTVALID,
  ADDASSIGN,
  DELASSIGN,
  ADDSTUD,
  DELSTUD,
  ADDGRADE
} GBADDCMD;

typedef enum GBDisplayFlag{
  NONE,
  PA,
  PS,
  PF,
  DISP_AN,
  DISP_FN,
  DISP_LN,
  DISP_G,
  DISP_A
}GBDisplayFlag;

typedef enum GBDisplayCMD{
  INVALID,
  PRINT_ASSIGN,
  PRINT_STUD,
  PRINT_FINAL
}GBDisplayCMD;

typedef struct _Buffer {
  unsigned char *Buf;
  unsigned long Length;
} Buffer;

typedef enum _ActionType {
  add_assignment,
  delete_assignment,
  add_student,
  delete_student,
  add_grade
} ActionType;

typedef struct Assignment {
  int total;
  int student_grade;
  float weight;
  char assign_name[101];
}Assignment;

typedef struct Student {
  char first_name[101];
  char last_name[101];
  Assignment stud_assigns[GBSIZE];
  float final;
}Student;

typedef struct Gradebook {
  Student student_arr[GBSIZE];
  Assignment all_assign[GBSIZE];
  int num_stud;
  int num_assign;
  float all_weights;
}Gradebook;

/*My Functions**********/
int file_test(char* filename);

int handleErrors();

void key_gen(char* key, int length);

int gcm_encrypt(unsigned char *plaintext, int plaintext_len,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *ciphertext,
                unsigned char *tag);

int gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *tag,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *plaintext);

/*Validate Names*/
int is_book_name_valid(char *name, int len);

int is_stud_name_valid(char *name, int len);

int is_assign_name_valid(char *name, int len);

int is_key_valid(char *key, int len);

int is_points_valid(int input);

int is_grade_valid(int input);

int is_weight_valid(float input);

int is_number(char *arg);

int is_float(char *arg);

/*Validate option names*/
int is_third_opt_valid(char *opt, int len);

int is_fourth_opt_valid(char *opt, int len);

/*Validate specifier options*/
int is_valid_AA(char **str_arr, int arr_len);

int is_valid_DA(char **str_arr, int arr_len);

int is_valid_AS(char **str_arr, int arr_len);

int is_valid_DS(char **str_arr, int arr_len);

int is_valid_AG(char **str_arr, int arr_len);

/***********************/


#endif
