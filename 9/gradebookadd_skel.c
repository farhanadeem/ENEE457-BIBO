#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

#define MAX_NAME_SIZE 100
#define MAX_DUB_SIZE 24         
#define MAX_INT_SIZE 10         

typedef struct _CmdLineResult {
  int     good;
  ActionType action;
  FILE* fp;
  char* fileName;
  unsigned char* key;
  char* assignmentName;
  int points;
  double weight;
  char* firstName;
  char* lastName;
  int grade;

} CmdLineResult;

int do_batch(char *);

int getValue(char* str){
  long tmp;
  char* ptr;
  int res;
  if(str == NULL || *str == '\0'){
    printf("No Name Provided\n");
    return -1;
  }
  if(strlen(str) > MAX_INT_SIZE){
    return -1;
  }
  //check that the string is digits
  if(*str < 48 || *str >57){
    return -1;
  }
  tmp = strtol(str,&ptr,10);
  if(tmp > INT_MAX || tmp < 0){
    return -1;
  } 
  return tmp;
}

double getWeight(char* str){
  double res;
  if(str == NULL || *str == '\0'){
    printf("No Name Provided\n");
    return -1;
  }
  //check that the string is digits
  if((*str < 48 && *str > 57) && *str != 46){
    return -1;
  }
  res = atof(str); 
  return res;
}

CmdLineResult* parse_cmdline(int argc, char *argv[]) {
  CmdLineResult* R = calloc(1,sizeof(CmdLineResult));
  int opt = 6;

  //initialize R with invalid values
  R->good = -1;
  R->assignmentName = NULL;
  R->fileName = NULL;
  R->firstName = NULL;
  R->fp = NULL;
  R->grade = -1;
  R->key = NULL;
  R->lastName = NULL;
  R->points = -1;
  R->weight = -1;

  if(argc < 7) {
    printf("Not enough commandline args\n"); 
    handleErrors();
  } else if(argc %2 == 1){
    printf("Cannot have an odd number of arguements\n");
    handleErrors();
  } else{ 
    /*for debugging purposes********************************************
    printf("\nNumber Of Arguments Passed: %d",argc); 
    printf("\n----Following Are The Command Line Arguments Passed----"); 
    for(int counter=0;counter<argc;counter++) {
      printf("\nargv[%d]: %s",counter,argv[counter]); 
    }
    /***********************************************************/
    //check that the gradebook name is specified first
    if(strcmp("-N", argv[1]) != 0){
      printf("Invalid Usage; Second Arg is not -N\n");
      printf("Proper Usage: gradebookadd -N <gradebookname> -K <key value> <action> <action args>\n");
      handleErrors();
    }
    //check that the filename provided is valid
    if(strlen(argv[2]) > MAX_NAME_SIZE){
      printf("Name too long\n");
      handleErrors();
    }
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
      printf("Proper Usage: gradebookadd -N <gradebookname> -K <key value> <action> <action args>\n");
      handleErrors();
    }
    //be careful of buffer overflow when reading in the key
    /*********************************************************
     * need to figure out safe way to do this
     * **********************************************************/
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
     * Parse Add Assignment
     * ***************************************/
    if(strcmp("-AA", argv[5]) == 0){
      //parse for add assignment
      R->action = add_assignment;
      while(opt+1 < argc){
        if(strcmp("-AN", argv[opt]) == 0){
          //check if next value is a valid name
          if(strlen(argv[opt+1]) > MAX_NAME_SIZE){
            printf("Name too long\n");
            handleErrors();
          }
          if(checkAssignmentName(argv[opt+1])){
            R->assignmentName = argv[opt+1];
          } else{
            printf("Invalid: Assignment name not alphanumeric\n");
            handleErrors();
          }
        }else if(strcmp("-P", argv[opt]) == 0){        
          int val = getValue(argv[opt+1]);
          if( val >= 0){
            R->points = val;
          } else{
            printf("Invalid: point value is not a non-negative number\n");
            handleErrors();
          }
        }else if(strcmp("-W", argv[opt]) == 0){
          double val = getWeight(argv[opt+1]);
          //accurate down to 15 0's
          if(val >= 0 && val <= 1){
            R->weight = val;
          } else{
            printf("Invalid: Weight value is not a real number between 0-1\n");
            handleErrors();
          }
        }else {
          printf("Invalid option for Add Assignment\n");
          handleErrors();
        }
        opt+=2;
      }
    } 
    /*******************************************
     * Parse Delete Assignment
     * ***************************************/
    else if(strcmp("-DA", argv[5]) == 0){
      //parse for delete assignment
      R->action = delete_assignment;
      while(opt+1 < argc){
        if(strcmp("-AN", argv[opt]) == 0){
          //check if next value is a valid name
          if(checkAssignmentName(argv[opt+1])){
            R->assignmentName = argv[opt+1];
          } else{
            printf("Invalid: Assignment name not alphanumeric\n");
            handleErrors();
          }
        }else {
          printf("Invalid option for Delete Assignment\n");
          handleErrors();
        }
        opt+=2;
      }
    } 
    /*******************************************
     * Parse Add Student
     * ***************************************/
    else if(strcmp("-AS", argv[5]) == 0){
      R->action = add_student;
      while(opt+1 < argc){
        if(strcmp("-FN", argv[opt]) == 0){
          //check if next value is a valid name
          if(strlen(argv[opt+1]) > MAX_NAME_SIZE){
            printf("Name too long\n");
            handleErrors();
          }
          if(checkName(argv[opt+1])){
            R->firstName = argv[opt+1];
          } else{
            printf("Invalid: first name not alphabetic characters\n");
            handleErrors();
          }
        }else if(strcmp("-LN", argv[opt]) == 0){
          //check if next value is a valid name
          if(strlen(argv[opt+1]) > MAX_NAME_SIZE){
            printf("Name too long\n");
            handleErrors();
          }
          if(checkName(argv[opt+1])){
            R->lastName = argv[opt+1];
          } else{
            printf("Invalid: last name not alphabetic characters\n");
            handleErrors();
          }
        }else {
          printf("Invalid option for Add Student\n");
          handleErrors();
        }
        opt+=2;
      }
    } 
    /*******************************************
     * Parse Delete Student
     * ***************************************/
    else if(strcmp("-DS", argv[5]) == 0){
      //parse for delete student
      R->action = delete_student;
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
          printf("Invalid option for Delete Student\n");
          handleErrors();
        }
        opt+=2;
      }
    } 
    /*******************************************
     * Parse Add Grade
     * ***************************************/
    else if(strcmp("-AG", argv[5]) == 0){
      //parse for add grade
      R->action = add_grade;
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
        }else if(strcmp("-G", argv[opt]) == 0){        
          int val = getValue(argv[opt+1]);
          if( val >= 0){
            R->grade = val;
          } else{
            printf("Invalid: point value is not a non-negative number\n");
            handleErrors();
          }
        }else if(strcmp("-AN", argv[opt]) == 0){
          //check if next value is a valid name
          if(checkAssignmentName(argv[opt+1])){
            R->assignmentName = argv[opt+1];
          } else{
            printf("Invalid: Assignment name not alphanumeric\n");
            handleErrors();
          }
        }else {
          printf("Invalid option for Add Grade\n");
          handleErrors();
        }
        opt+=2;
      }
    } else {
      printf("Invalid Usage; Fifth arg is not a valid action\n");
      printf("Proper Usage: gradebookadd -N <gradebookname> -K <key value> <action> <action args>\n");
      handleErrors();
    }
  } 

  R->good = 1;
  return R;
}

int main(int argc, char *argv[]) {
  int r;
  CmdLineResult R;
  CmdLineResult* res;
  char* path;
  Buffer* buf;
  unsigned char* key;
  Gradebook* gradebook;
  
  res = parse_cmdline(argc, argv);
  R = *res;

  if(R.good == -1) {
    printf("Error in parsing\n");
    handleErrors();
  }

  //parse file into gradebook
  gradebook = read_Gradebook_from_path(R.fp, R.key);
  //check if the read was valid (passed authentication and decryption)
  if(gradebook == NULL){
    printf("Failed to read/authenticate\n");
    handleErrors();
  }
  fclose(R.fp);
  /*********************************
   * Carry out action
   * ******************************/
  //ADD ASSIGNMENT
  if(R.action == add_assignment){
    //check all values needed given in commandline args
    if(R.assignmentName == NULL || R.points == -1 || R.weight == -1){
      printf("not all args provided at command line for add assignment\n");
      handleErrors();
    }
    //check if assignment already exists
    if(findAssignmentName(R.assignmentName, *gradebook) != -1){
      printf("Assignment already exists\n");
      handleErrors();
    }    
    //place the new assignment in the gradebook
    Assignment* new = malloc(sizeof(Assignment));
    Assignment* currA;
    double weightTot = R.weight;
    int assignNum = gradebook->numAssignments+1;
    if(gradebook->assignments == NULL){
      if(R.weight <= 1){
        gradebook->assignments = new;
      }else{
        printf("Weight exceeds 1\n");
        handleErrors();
      }      
    }else{
      currA = gradebook->assignments;
      weightTot += currA->weight;      
      while(currA->next != NULL){
        currA = currA->next;
        weightTot += currA->weight;
      }
      if(weightTot > 1){
        printf("Total weight over 1\n");
        handleErrors();
      }
      currA->next = new;
    }
    //set all of new's values
    new->next = NULL;
    new->assignmentNumber = assignNum;
    new->assignmentName = R.assignmentName;
    new->points = R.points;
    new->weight = R.weight;
    //add 1 to number of assignments
    gradebook->numAssignments++;
    //add assignment to all of the students
    Student* currS = gradebook->students;
    while(currS != NULL){
      //iterate through all the grades and add 0 to the end
      Grade* newG = malloc(sizeof(Grade));
      Grade* currG = currS->grades;
      if(currG == NULL){
        currS->grades = newG;
      }else{
        while(currG->next != NULL){
          currG = currG->next;
        }
        currG->next = newG;
      }
      
      //set newG values
      newG->next = NULL;
      newG->score = 0;
      //move to next student
      currS = currS->next;
    }
    // printf("Done Add Assignment\n");    
  }
  //DELETE ASSIGNMENT
  else if(R.action == delete_assignment){
    if(R.assignmentName == NULL){
      printf("not all args provided at command line for delete assignment\n");
      handleErrors();
    }
    //find the asssignment number for the given name (-1 if it doesn't exist)
    int n = findAssignmentName(R.assignmentName, *gradebook);
    if(n == -1){
      printf("Assignment doesn't exists\n");
      handleErrors();
    }
    
    //remove the nth assignment from the grade book
    Assignment* prev = NULL;
    Assignment* currA = gradebook->assignments;
    for(int i = 1; i < n; i++){
      prev = currA;
      currA = currA->next;
    }
    if(prev == NULL){
      gradebook->assignments = currA->next;
    }else{
      prev->next = currA->next;
    }
    gradebook->numAssignments--;

    //remove the nth grade from each student
    Student* S = gradebook->students;
    for(int i = 0; i < gradebook->numStudents; i++){
      Grade* prev;
      Grade* currG = S->grades;
      for(int j = 1; j < n; j++){
        prev = currG;
        currG = currG->next;
      }
      if(prev == NULL){
        S->grades = currG->next;
      }else{
        prev->next = currG->next;
      }
      S = S->next;
    }

    // printf("Done Delete assignment\n");
  }
  //ADD STUDENT
  else if(R.action == add_student){
    //check if student already exists
    if(R.firstName == NULL || R.lastName == NULL){
      printf("not all args provided at command line for add Student\n");
      handleErrors();
    }
    if(findStudentName(R.firstName, R.lastName, *gradebook) != -1){
      printf("student already exists\n");
      handleErrors();
    }
    //add to numStudents
    gradebook->numStudents++;
    //put new student in the gradebook
    Student* new = malloc(sizeof(Student));
    Student* currS;
    if(gradebook->students == NULL){
      gradebook->students = new;
    }else{
      currS = gradebook->students;
      while(currS->next != NULL){
        currS = currS->next;
      }
      currS->next = new;
    }
    //initialize new student
    new->firstName = R.firstName;
    new->lastName = R.lastName;
    new->next = NULL;
    if(gradebook->numAssignments == 0){
      new->grades = NULL;
    }
    //initialize grade list
    Grade* prev;
    for(int i = 0; i < gradebook->numAssignments; i++){
      Grade* G = malloc(sizeof(Grade));
      if(i == 0){
        new->grades = G;
      }else{
        prev->next = G;
      }
      prev = G;
      //initialize values
      G->score = 0;
      G->next = NULL;
    }
    // printf("Done Add Student\n");
  }
  //DELETE STUDENT
  else if(R.action == delete_student){
    //check for valid input
    if(R.firstName == NULL || R.lastName == NULL ){
      printf("not all args provided at command line for delete Student\n");
      handleErrors();
    }
    int n = findStudentName(R.firstName, R.lastName, *gradebook);
    if(n == -1){
      printf("Student does not exist\n");
      handleErrors();
    }
    //find the nth student and remove them from the student list
    Student* prev = NULL;
    Student* currS = gradebook->students;
    for(int i = 1; i < n; i++){
      prev = currS;
      currS = currS->next;
    }
    if(prev == NULL){
      gradebook->students = currS->next;
    }else{
      prev->next = currS->next;
    }
    gradebook->numStudents--;

    // printf("Done Delete Student\n");
  }
  //ADD GRADE
  else if(R.action == add_grade){
    //check for null
    if (R.firstName == NULL || R.lastName == NULL || R.assignmentName == NULL || R.grade == -1){
      printf("not all args provided at command line for add Grade\n");
      handleErrors();
    }
    int assignNum = findAssignmentName(R.assignmentName, *gradebook);
    if(assignNum == -1){
      printf("Assignment does not exists\n");
      handleErrors();
    }
    int numStud = findStudentName(R.firstName, R.lastName, *gradebook);
    if(numStud == -1){
      printf("Student does not exists\n");
      handleErrors();
    }
    //get the desired student object
    Student* currS = gradebook->students;
    for(int i = 1; i < numStud; i++){
      currS = currS->next;
    }
    //find corresponding grade
    Grade* currG = currS->grades;
    for(int i = 1; i < assignNum; i++){
      currG = currG->next;
    }
    currG->score = R.grade;

    // printf("Done Add Grade\n");
  }else{
    printf("invalid action\n");
    handleErrors();
  }
    
  /*********************************
   * write the resulting gradebook back out to the file
   * ***********************************/
  FILE* fp = fopen(R.fileName, "w");
  write_to_path(fp, *gradebook, R.key); 
  // printf("SUCCESSFUL GRADEBOOKADD\n"); 
  return 0;
}
