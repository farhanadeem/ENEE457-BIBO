#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

typedef struct _PrtOrder PrtOrder;

typedef struct _PrtOrder{
  char* firstName;
  char* lastName;
  int grade;
  double finalGrade;
  PrtOrder* next;
} PrtOrder;

double computeGrade(Student* S, Assignment* A){
  Grade* G = S->grades;
  double res = 0;
  // printf("%s %s:\n", S->firstName, S->lastName);
  while(A != NULL){
    double score = G->score, points = A->points;
    double tmp = score/points;
    tmp = tmp*A->weight;
    res += tmp;
    A = A->next;
    G = G->next;
  }
  return res;
}

typedef struct _CmdLineResult {
  int     good;
  PrintType action;
  FILE* fp;
  unsigned char* key;
  char* assignmentName;
  char* fileName;
  double weight;
  char* firstName;
  char* lastName;
  int gradeOrder;
  int alphabeticalOrder;

} CmdLineResult;

int nameCompare(char* fn1, char* ln1, char* fn2, char* ln2) {
  int n = strcasecmp(ln1,ln2);
  if(n == 0){
    //compare first names
    if(strcasecmp(fn1, fn2) < 0){
      return 0;
    }else{
      return 1;
    }
  } else if(n < 0){
    return 0;
  } else{
    return 1;
  }
}

void print_Assignment(Gradebook gradebook, char* assignmentName, int G, int A) {
  PrtOrder* head;
  //get the assignment
  Assignment* currA = gradebook.assignments;
  int found = 0;
  while(currA != NULL && !found){
    if(strcmp(currA->assignmentName, assignmentName) == 0){
      found = 1;
    }else{
      currA = currA->next;
    }
  }
  if(!found){
    printf("Assignment not found\n");
    handleErrors();
  }

  Student* currS = gradebook.students;
  int n = currA->assignmentNumber;
  /**********************************************
   * alphabetical order
   * *******************************************/
  if(G < 0 && A > 0){
    //iterate through students adding them to an ordered linked list
    for(int i = 0; i < gradebook.numStudents; i++){
      //initialize the tuple
      PrtOrder* tuple = malloc(sizeof(PrtOrder));
      tuple->firstName = currS->firstName;
      tuple->lastName = currS->lastName;
      Grade* G = currS->grades;
      for(int j = 1; j < n; j++){
        G = G->next;
      }
      tuple->grade= G->score;
      tuple->next = NULL;
      
      PrtOrder* curr = head;
      PrtOrder* prev = NULL;
      //insert the tuple into the list based on grade order
      if(curr == NULL){
        //list empty
        head = tuple;
      }else{
        
        while(curr != NULL && nameCompare(tuple->firstName,tuple->lastName,curr->firstName,curr->lastName)){
          prev = curr;
          curr = curr->next;
        }
        if(prev == NULL){
          head = tuple;
          tuple->next = curr;
        }else{
          tuple->next = prev->next;
          prev->next = tuple;
        }        
      }
      currS = currS->next;
    }
    //now iterate through the list and print the tuples
      PrtOrder* curr = head;
      while(curr != NULL){
        printf("(%s, %s, %d)\n", curr->lastName,curr->firstName, curr->grade);
        curr = curr->next;
    }
  }
  /**********************************************
   * grade order
   * *******************************************/
  else if(A < 0 && G > 0){
    //iterate through students adding them to an ordered linked list
    for(int i = 0; i < gradebook.numStudents; i++){
      //initialize the tuple
      PrtOrder* tuple = malloc(sizeof(PrtOrder));
      tuple->firstName = currS->firstName;
      tuple->lastName = currS->lastName;
      Grade* G = currS->grades;
      for(int j = 1; j < n; j++){
        G = G->next;
      }
      tuple->grade= G->score;
      tuple->next = NULL;
      
      PrtOrder* curr = head;
      PrtOrder* prev = NULL;
      //insert the tuple into the list based on grade order
      if(curr == NULL){
        //list empty
        head = tuple;
      }else{
        while(curr->next != NULL && tuple->grade < curr->grade){
          prev = curr;
          curr = curr->next;
        }
        if(curr == head && tuple->grade > curr->grade){
          head = tuple;
          tuple->next = curr;
        } else{
          tuple->next = curr->next;
          curr->next = tuple;
        }        
      }
      currS = currS->next;
    }
    //now iterate through the list and print the tuples
      PrtOrder* curr = head;
      while(curr != NULL){
        printf("(%s, %s, %d)\n", curr->lastName,curr->firstName, curr->grade);
        curr = curr->next;
    }
  }else{
    printf("invalid sorting choice made\n");
    handleErrors();
  }
}

void print_Student(Gradebook gradebook, char* firstName, char* lastName) {
  //find the student
  Student* S = gradebook.students;
  int found = 0;

  while(S != NULL && !found){
    if(strcmp(S->firstName, firstName) == 0 && strcmp(S->lastName, lastName) == 0){
      found = 1;
    }else{
      S = S->next;
    }
  }
  if(!found){
    printf("Student not found\n");
    handleErrors();
  }
  Assignment* A = gradebook.assignments;
  Grade* G = S->grades;
  //print out all the assignments
  for(int i = 0; i < gradebook.numAssignments; i++){ 
    
    printf("(%s,%d)\n", A->assignmentName, G->score);
    A = A->next;
    G = G->next;
  }
}

void print_Final(Gradebook gradebook, int G, int A){
  PrtOrder* head;
  Student* currS = gradebook.students;
  /**********************************************
   * alphabetical order
   * *******************************************/
  if(G < 0 && A > 0){
    //iterate through students adding them to an ordered linked list
    for(int i = 0; i < gradebook.numStudents; i++){
      //initialize the tuple
      PrtOrder* tuple = malloc(sizeof(PrtOrder));
      tuple->firstName = currS->firstName;
      tuple->lastName = currS->lastName;
      tuple->finalGrade = computeGrade(currS, gradebook.assignments);
      tuple->next = NULL;
      
      PrtOrder* curr = head;
      PrtOrder* prev = NULL;
      //insert the tuple into the list based on grade order
      if(curr == NULL){
        //list empty
        head = tuple;
      }else{
        
        while(curr != NULL && nameCompare(tuple->firstName,tuple->lastName,curr->firstName,curr->lastName)){
          prev = curr;
          curr = curr->next;
        }
        if(prev == NULL){
          head = tuple;
          tuple->next = curr;
        }else{
          tuple->next = prev->next;
          prev->next = tuple;
        }        
      }
      currS = currS->next;
    }
    //now iterate through the list and print the tuples
      PrtOrder* curr = head;
      while(curr != NULL){
        printf("(%s, %s, %g)\n", curr->lastName,curr->firstName, curr->finalGrade);
        curr = curr->next;
    }
  }
  /**********************************************
   * grade order
   * *******************************************/
  else if(A < 0 && G > 0){
    //iterate through students adding them to an ordered linked list
    for(int i = 0; i < gradebook.numStudents; i++){
      //initialize the tuple
      PrtOrder* tuple = malloc(sizeof(PrtOrder));
      tuple->firstName = currS->firstName;
      tuple->lastName = currS->lastName;
      tuple->finalGrade = computeGrade(currS, gradebook.assignments);
      tuple->next = NULL;
      
      PrtOrder* curr = head;
      PrtOrder* prev;
      //insert the tuple into the list based on grade order
      if(curr == NULL){
        //list empty
        head = tuple;
      }else{
        while(curr->next != NULL && tuple->finalGrade < curr->finalGrade){
          prev = curr;
          curr = curr->next;
        }
        if(curr == head && tuple->finalGrade > curr->finalGrade){
          head = tuple;
          tuple->next = curr;
        } else{
          tuple->next = curr->next;
          curr->next = tuple;
        }        
      }
      currS = currS->next;
    }
    //now iterate through the list and print the tuples
      PrtOrder* curr = head;
      while(curr != NULL){
        printf("(%s, %s, %g)\n", curr->lastName,curr->firstName, curr->finalGrade);
        curr = curr->next;
    }
  }else{
    printf("invalid sorting choice made\n");
    handleErrors();
  }
}

CmdLineResult* parse_cmdline(int argc, char* argv[]){
  CmdLineResult* R = calloc(1,sizeof(CmdLineResult));
  int opt = 6;

  //initialize R with invalid values
  R->good = -1;
  R->assignmentName = NULL;
  R->fileName = NULL;
  R->firstName = NULL;
  R->fp = NULL;
  R->gradeOrder = -1;
  R->key = NULL;
  R->lastName = NULL;
  R->alphabeticalOrder = -1;

  if(argc < 7) {
    printf("Not enough commandline args\n"); 
    handleErrors();
  } else{
    //parse the cmdline args
    //check that the gradebook name is specified first
    if(strcmp("-N", argv[1]) != 0){
      printf("Invalid Usage; Second Arg is not -N\n");
      printf("Proper Usage: gradebookadd -N <gradebookname> -K <key value> <action> <action args>\n");
      handleErrors();
    }
    //check that the filename provided is valid
    if(checkFileName(argv[2]) == 0){
      printf("Invalid Usage; Invalid Name Provided\n");
      handleErrors();
    } 
    //read in gradebook name to make sure it exists then
    //save the name so you can write to it later
    R->fp = fopen(argv[2], "r");
    if (R->fp == NULL){
      printf("Invalid: no file found\n");
      handleErrors();
    }
    R->fileName = argv[2];

    //read in the key
    if(strcmp("-K", argv[3]) != 0){
      printf("Invalid Usage; Fourth Arg is not -K\n");
      printf("Proper Usage: gradebookdisplay -N <gradebookname> -K <key value> <action> <action args>\n");
      handleErrors();
    }
    //be careful of buffer overflow when reading in the key
    if(strlen(argv[4]) != 64){
      printf("Invalid Key length\n");
      handleErrors();
    }
    if(!checkHex(argv[4])){
      printf("invalid hex for key\n");
      handleErrors();
    }
    //convert hex string to bytes
    char *p;
    unsigned char *k = (unsigned char *)malloc(32), *r;
    unsigned int x;
    for (p = argv[4], r = k; *p; p += 2) {
        if (sscanf(p, "%02X", (unsigned int *)&x) != 1) {
           printf("Error parsing key\n");
           handleErrors(); // Didn't parse as expected
        }
        *r++ = x;
    }
    R->key = k;
    
    //parse what action is specified
    /*******************************************
     * Parse Print Assignment
     * ***************************************/
    if(strcmp("-PA", argv[5]) == 0){
      R->action = print_assignment;
      while(opt < argc){
        if(strcmp("-AN", argv[opt]) == 0){
          if(opt+1 >= argc){
            printf("Missing the name\n");
            handleErrors();
          } 
          if(checkAssignmentName(argv[opt+1])){
            R->assignmentName = argv[opt+1];
          } else{
            printf("Invalid: first name not alphabetic characters\n");
            handleErrors();
          }
          opt+=2;
        } else if(strcmp("-A", argv[opt]) == 0){
          if(R->gradeOrder == 1){
            printf("only pick one option\n");
            handleErrors();
          }else{
            R->alphabeticalOrder = 1;
          }
          opt++;
        } else if(strcmp("-G", argv[opt]) == 0){
          if(R->alphabeticalOrder == 1){
            printf("only pick one option\n");
            handleErrors();
          }else{
            R->gradeOrder = 1;
          }
          opt++;
        }else {
          printf("Invalid option for print assignment\n");
          handleErrors();
        }
      }
    }
    /*******************************************
     * Parse Print Student
     * ***************************************/
    else if(strcmp("-PS", argv[5]) == 0){
      R->action = print_student;
      if(argc % 2 == 1){
        printf("cannot have an odd number of args for print student\n");
        handleErrors();
      }
      while(opt+1 < argc){
        if(strcmp("-FN", argv[opt]) == 0){
          //check if next value is a valid name
          if(checkName(argv[opt+1])){
            R->firstName = argv[opt+1];
          } else{
            printf("Invalid: first name not alphabetic characters\n");
            handleErrors();
          }
        }else if(strcmp("-LN", argv[opt]) == 0){
          //check if next value is a valid name
          if(checkName(argv[opt+1])){
            R->lastName = argv[opt+1];
          } else{
            printf("Invalid: last name not alphabetic characters\n");
            handleErrors();
          }
        }else {
          printf("Invalid option for print Student\n");
          handleErrors();
        }
        opt+=2;
      }
    }
    /*******************************************
     * Parse Print Final
     * ***************************************/
    else if(strcmp("-PF", argv[5]) == 0){
      R->action = print_final;
      if(argc >=8){
        printf("To many Args for print Final\n");
        handleErrors();
      }
      if(strcmp(argv[6], "-G") == 0){
        R->gradeOrder = 1;
      }else if(strcmp(argv[6], "-A") == 0){
        R->alphabeticalOrder = 1;
      }else{
        printf("Invalid option for Print Final; want -A or -G\n");
        handleErrors();
      }
    }
  }

  R->good = 1;
  return R;
}

int main(int argc, char *argv[]) {
  int   opt,len;
  char  *logpath = NULL;
  CmdLineResult R;
  CmdLineResult* res;

  res = parse_cmdline(argc, argv);
  R = *res;

  if(R.good == -1) {
    printf("Error in parsing\n");
    handleErrors();
  }

  //read in gradebook
  Gradebook* gradebook = read_Gradebook_from_path(R.fp, R.key);
  //check if the read was valid (passed authentication and decryption)
  if(gradebook == NULL){
    printf("Failed to read/authenticate\n");
    handleErrors();
  }
  fclose(R.fp);

  /***************************************************
   * Do the action
   * ***************************************************/
  if(R.action == print_student){
    print_Student(*gradebook, R.firstName, R.lastName);
  }else if(R.action == print_assignment){
    print_Assignment(*gradebook, R.assignmentName, R.gradeOrder, R.alphabeticalOrder);
  }else if(R.action == print_final){
    print_Final(*gradebook, R.gradeOrder, R.alphabeticalOrder);
  }else{
    handleErrors();
  }
}
