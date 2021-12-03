typedef enum _ActionType {
//for gradebookadd and display
  add_assignment,
  delete_assignment,
  add_student,
  delete_student,
  add_grade,
  
  print_assignment,
  print_student,
  print_final,
  nonce
} ActionType;

typedef struct Student {
  //TODO put some other stuff here
  char *first_name;
  char *last_name;
  int grade;
  float final_grade;
  struct Student *next;
} Student;

typedef struct Assignment { //or could make this _Assignment and struct _Assignment *next below
  //put some things here
  char* assignment_name;
  int points;
  float weight;
  Student *students;
  
  struct Assignment *next;
} Assignment;

typedef struct Gradebook {
  int num_of_assignments;
  int num_of_students;
  float sum_of_weights;
  //and a linked list of assignment and student
  Assignment *assignments;
  Student *students;//THIS IS TO KEEP TRACK OF WHETHER A STUDENT EXISTS IN A GRADEBOOK. grade will be null for this parameter.
  
  int cipher;
  int MAC_output;
} Gradebook;

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
                
                
void handleErrors(void);

