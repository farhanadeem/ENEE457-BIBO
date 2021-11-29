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
#include <regex.h>
#include <getopt.h>
#include <string.h>
#include "data.h"

#define LEN 16
// 128 bits
typedef struct _CmdLineResult {
  //TODO probably put more things here
  int     good; //proper command format
  char * gradebook_name; //gradebook name
  unsigned char * key; //key
  ActionType action_type; //gets action type.
  int num_cmds;
  char ** cmd_args; //other arguments provided by user.
} CmdLineResult;

int do_batch(char *);

int validate_key(char * key, FILE * f){
  return 0;
}

CmdLineResult parse_cmdline(int argc, char *argv[]) {
  CmdLineResult R = { 0 };
  int option_index,c, good_args, i=0, value = 0;
  size_t filename_len;
  regex_t regex;
  unsigned char * ptr;


  //preset error values.
  R.good = -1; //not set to 0 until end of function, all premature returns bad.
  R.num_cmds = -1;
  R.gradebook_name = NULL;
  R.key = NULL;
  R.cmd_args = NULL;

  //define options.
  struct option options[] = {
    {"N", required_argument, 0, 'a'},
    {"K", required_argument, 0, 'b'},
    {"AA", no_argument, 0, 'c'},
    {"DA", no_argument, 0, 'd'},
    {"AS", no_argument, 0, 'e'},
    {"DS", no_argument, 0, 'f'},
    {"AG", no_argument, 0, 'g'},
    {"AN", required_argument, 0, 'h'},
    {"FN", required_argument, 0, 'i'},
    {"LN", required_argument, 0, 'j'},
    {"P", required_argument, 0, 'k'},
    {"W", required_argument, 0, 'l'},
    {"G", required_argument, 0, 'm'},
  };

  if(argc < 6){
    return R;
  }

  //a gradebook name must first be specified with "-N"
  if(strcmp(argv[1], "-N") != 0){
    return R;
  }

  //check that next argument is "-K".
  if(strcmp(argv[3], "-K") != 0){
    return R;
  }

  //after k we need an action.

  if(strcmp(argv[5], "-AA") != 0 && strcmp(argv[5], "-DA") != 0 && strcmp(argv[5], "-AS") != 0  &&
      strcmp(argv[5], "-DS") != 0 && strcmp(argv[5], "-AG") != 0 ){
      //if we don't match any of the actions at this argument, fail.
      return R;
  }


  while((c=getopt_long_only(argc, argv, "a:b:cdefgh:i:j:k:l:m:", options, &option_index)) != -1){
    switch (c) {
      case 'a':
        if(R.gradebook_name != NULL){
          return R; //only let us have 1 gradebook name.
        }
        //make sure filename is of correct form.
        //use a regular expression to make sure the title only contains alphanumeric + _.
        value = regcomp(&regex, "^[[:alnum:]_\\.]+$", REG_EXTENDED);
        if(value){
          printf("Could not compile regex.\n");
          return R;
        }

        //match input.
        value = regexec(&regex, optarg, 0,NULL, 0);
        if(value){
          return R;
        }

        //now check it exists.
        if(access(optarg, F_OK) != 0){
          //file does not exist, throw error.
          return R;
        }
        //copy the filename into the return struct.
        filename_len = strlen(optarg);
        R.gradebook_name = (char *) malloc(filename_len + 1);
        strncpy(R.gradebook_name, optarg, filename_len);
        R.gradebook_name[filename_len] = '\0';
        break;
      case 'b':

        if(R.key != NULL){
          return R; //only 1 key.
        }

        //arg should be a 128 bit (16 byte) key.
        if(strlen(optarg) != LEN*2){ //printed 2 chars for each byte.
          return R;
        }


        //copy over the key now that we know it is the right length.
        R.key = (unsigned char *) malloc(sizeof(unsigned char)*LEN);

        ptr = optarg;

        for(i = 0; i < LEN; i++){
          sscanf(ptr, "%2hhx", &R.key[i]);
          ptr += 2;
        }



        break;
      case 'c':
        //add-assignment arg.
        if(R.num_cmds == -1){ //only allow 1 action to be passed on the cmd line.
          R.action_type = add_assignment;
          R.num_cmds = 3;
          R.cmd_args = (char **) malloc(sizeof(char *)*R.num_cmds); //array of 3 pointers to arguments.
          for(i = 0; i < R.num_cmds; i++){
            R.cmd_args[i] = NULL; //set all to NULL so that we can check that they are all there at the end.
          }
        } else {
          printf("invalid\n"); //not allowed to have multiple actions.
          return R;
        }

        break;
      case 'd':
        if(R.num_cmds == -1){
            R.action_type = delete_assignment;
            R.num_cmds = 1;
            R.cmd_args = (char **) malloc(sizeof(char *)*R.num_cmds); //array of 3 pointers to arguments.
            for(i = 0; i < R.num_cmds; i++){
              R.cmd_args[i] = NULL; //set all to NULL so that we can check that they are all there at the end.
            }
        } else {
          return R;
        }


        break;
      case 'e':
        if(R.num_cmds == -1){
            R.action_type = add_student;
            R.num_cmds = 2;
            R.cmd_args = (char **) malloc(sizeof(char *)*R.num_cmds); //array of 2 pointers to arguments.
            for(i = 0; i < R.num_cmds; i++){
              R.cmd_args[i] = NULL; //set all to NULL so that we can check that they are all there at the end.
            }
        } else {
          return R;
        }
        break;
      case 'f':
        if(R.num_cmds == -1){
            R.action_type = delete_student;
            R.num_cmds = 2;
            R.cmd_args = (char **) malloc(sizeof(char *)*R.num_cmds); //array of 2 pointers to arguments.
            for(i = 0; i < R.num_cmds; i++){
              R.cmd_args[i] = NULL; //set all to NULL so that we can check that they are all there at the end.
            }

        } else {
          return R;
        }
        break;
      case 'g':
        if(R.num_cmds == -1){
            R.action_type = add_grade;
            R.num_cmds = 4;
            R.cmd_args = (char **) malloc(sizeof(char *)*R.num_cmds); //array of 3 pointers to arguments.
            for(i = 0; i < R.num_cmds; i++){
              R.cmd_args[i] = NULL; //set all to NULL so that we can check that they are all there at the end.
            }

        } else {
          return R;
        }
        break;
      case 'h':
        //assignment name.
        if(R.num_cmds == -1){
          //we haven't specified an action yet.
          return R;
        }
        //only required for AA, DA, AG.
        if (R.action_type == add_assignment || R.action_type == delete_assignment || R.action_type == add_grade){
          R.cmd_args[0] = optarg; //point to the string specified.
        } else {
          return R;
        }
        break;
      case 'i':
        //first name.
        if(R.num_cmds == -1){
          //we haven't specified an action yet.
          return R;
        }

        //check that first name is only A-Z,a-z
        value = regcomp(&regex, "^[A-Za-z]+$", REG_EXTENDED);
        if(value){
          printf("Could not compile regex.\n");
          return R;
        }

        //match input.
        value = regexec(&regex, optarg, 0,NULL, 0);
        if(value){
          return R; //not an valid name.
        }
        //only required for add student, delete student, add grade.
        if(R.action_type == add_student || R.action_type == delete_student){
          R.cmd_args[0] = optarg; //point to the first name specified.
        } else if(R.action_type == add_grade){
          R.cmd_args[1] = optarg;
        } else {
          return R;  //any other action bad.
        }
        break;
      case 'j':
        //last name
        if(R.num_cmds == -1){
          //we haven't specified an action yet.
          return R;
        }

        //check that last name is only A-Z,a-z
        value = regcomp(&regex, "^[A-Za-z]+$", REG_EXTENDED);
        if(value){
          printf("Could not compile regex.\n");
          return R;
        }

        //match input.
        value = regexec(&regex, optarg, 0,NULL, 0);
        if(value){
          return R; //not a valid name.
        }
        //only required for add student, delete student, add grade.
        if(R.action_type == add_student || R.action_type == delete_student){
          R.cmd_args[1] = optarg;
        } else if(R.action_type == add_grade){
          R.cmd_args[2] = optarg;
        } else {
          return R;  //any other action bad.
        }
        break;
      case 'k':
        //points
        if(R.num_cmds == -1){
          //we haven't specified an action yet.
          return R;
        }

        //check that points is only numbers. (integer);
        value = regcomp(&regex, "^[0-9]+$", REG_EXTENDED);
        if(value){
          printf("Could not compile regex.\n");
          return R;
        }

        //match input.
        value = regexec(&regex, optarg, 0,NULL, 0);
        if(value){
          return R; //not an integer.
        }


        //only required for add-assignment.
        if(R.action_type == add_assignment){
          R.cmd_args[1] = optarg;
        } else {
          return R;  //any other action bad.
        }
        break;
      case 'l':
        //weight.
        if(R.num_cmds == -1){
          //we haven't specified an action yet.
          return R;
        }

        //check that this is at decimal starting with either 0,1 or nothing.
        value = regcomp(&regex, "^[01]?\\.[0-9]*$", REG_EXTENDED);
        if(value){
          printf("Could not compile regex.\n");
          return R;
        }

        //match input.
        value = regexec(&regex, optarg, 0,NULL, 0);
        if(value){
          return R; //not an integer.
        }

        //only required for add_assignment.
        if(R.action_type == add_assignment){
          R.cmd_args[2] = optarg;
        } else {
          return R;  //any other action bad.
        }
        break;
      case 'm':
        //grade
        if(R.num_cmds == -1){
          //we haven't specified an action yet.
          return R;
        }

        //grade should be an integer.
        value = regcomp(&regex, "^[0-9]+$", REG_EXTENDED);
        if(value){
          printf("Could not compile regex.\n");
          return R;
        }

        //match input.
        value = regexec(&regex, optarg, 0,NULL, 0);
        if(value){
          return R; //not an integer.
        }

        //only required for add-grade.
        if(R.action_type == add_grade){
          R.cmd_args[3] = optarg;
        } else {
          return R;  //any other action bad.
        }
        break;
      case '?':
        return R;
    }
  }



  //check that the action and cmd_args are filled out, as well as the filename fits format.
  if(R.gradebook_name != NULL && R.key != NULL && R.num_cmds != -1 && R.cmd_args != NULL){
    //if everything here is fine, then for loop through the arguments and make sure none of them are NULL.
    good_args = 0;
    for(i = 0; i < R.num_cmds; i++){
      if(R.cmd_args[i] != NULL){
        good_args += 1;
      }
    }

    if(good_args == R.num_cmds){ //if we have our key, gradebook name, and arguments all satisfied, we are good.
      R.good = 0;
    }
  }

  return R;
}


int Add_Assignment(Gradebook ** G, char * name, unsigned int points, double weight){
  Assignment * A;
  Assignment ** temp;
  double total_weight = 0.0;
  int i=0;
  //check before doing any work that no assignment with this name exists and that the weights are all <1.0.
  if((*G)->num_assignments > 0){
    for(i=0; i < (*G)->num_assignments; i++){
      //loop through all assignments and check.
      total_weight += (*G)->assignments[i]->weight;
      if(strcmp((*G)->assignments[i]->name, name) == 0){
        return -1; //return -1 if the name matches.
      }
    }
  }

  if(total_weight + weight > 1.0){ //if this grade will make the total more than 1, invalid.
    return -1;
  }


  //create an assignment for this.
  A = (Assignment *) malloc(sizeof(Assignment));
  A->id = (*G)->next_id++; //give the id and increase it.
  A->name_len = strlen(name);
  A->points = points;
  A->weight = weight;
  A->name = (char *) malloc(1+ sizeof(char)*A->name_len);
  strncpy(A->name,name, A->name_len); //copy the name over.
  A->name[A->name_len] = '\0'; //null terminate the name.

  //now add this assignment to the list of assignments in the gradebook.
  if((*G)->num_assignments == 0){
    //if the number of assignments is 0, we haven't allocated memory for the assignments array yet.
    (*G)->assignments =(Assignment **) malloc(sizeof(Assignment *));
  } else {
    //otherwise realloc so there's 1 more assignment.
    temp = realloc((*G)->assignments, sizeof(Assignment *)*((*G)->num_assignments+1));
    if(temp == NULL){
      return -1; //realloc failed.
    } else {
      (*G)->assignments =temp;
    }
  }

  (*G)->assignments[(*G)->num_assignments] = A; //add the assignment to the list of assignments.

  (*G)->num_assignments++; //increase the number of assignments.

  return 0;
}

int Add_Student(Gradebook **G, char * first, char * last){
  Student * S;
  int i = 0,j=0;
  Student ** temp;


  //make the student.
  S = (Student *) malloc(sizeof(Student));
  S->name_len = strlen(first) + strlen(last) + 2;
  S->name = (char *) malloc(sizeof(char)*(S->name_len + 1));
  //concatenate the first and last name.
  strcpy(S->name, last);
  strcpy((S->name + strlen(last)), ", ");
  strcpy(S->name + (strlen(last) + 2), first);
  S->name[S->name_len] = '\0';

  //this student won't have any grades yet.
  S->num_grades = 0;
  S->assignment_ids = NULL;
  S->grades = NULL;


  //check that no other student has this name.
  for(i = 0; i < (*G)->num_students; i++){
    if(strcmp((*G)->students[i]->name, S->name) == 0){
      //if the names match, this is the same student.
      free(S); //free the student we made.
      return -1;
    }
  }
  //allocate space for it in the students array.
  if((*G)->num_students == 0){
    //no students means there's no space allocated for the array yet.
    (*G)->students =(Student **) malloc(sizeof(Student *));
  } else {
    //otherwise realloc so there's 1 more student.
    temp = realloc((*G)->students, sizeof(Student *)*((*G)->num_students+1));
    if(temp == NULL){
      return -1; //realloc failed.
    } else {
      (*G)->students =temp;
    }
  }

  (*G)->students[(*G)->num_students] = S; //add the student to the list of students.

  (*G)->num_students++; //increase the number of assignments.


  return 0;



}

int Add_Grade(Gradebook ** G, char * first, char * last, char * assignment, unsigned int grade){
  char * student_name;
  unsigned int name_len, assignment_id;
  int i =0, j = 0, assignment_found = 0;
  unsigned int * temp;


  //combine the first and last name to get the student name.
  name_len = strlen(first) + strlen(last) + 2;
  student_name = (char *) malloc(sizeof(char)*(name_len + 1));
  //concatenate the first and last name.
  strcpy(student_name, last);
  strcpy((student_name + strlen(last)), ", ");
  strcpy(student_name + (strlen(last) + 2), first);
  student_name[name_len] = '\0';

  //check that the assignment name exists.
  for(i=0; i < (*G)->num_assignments; i++){
    if(strcmp((*G)->assignments[i]->name, assignment) == 0) {
      assignment_found = 1;
      assignment_id = (*G)->assignments[i]->id;
    }
  }

  if(assignment_found  == 0 ){
    //if we did not find the assignment.
    return -1; //an error occurs.
  }

  //search through students to add grade.
  for(i = 0; i < (*G)->num_students; i++){
    if(strcmp((*G)->students[i]->name, student_name) == 0){
      //found the student.
      if((*G)->students[i]->num_grades == 0){
        //if the student has no grades, we need to allocate the two arrays.
        (*G)->students[i]->assignment_ids = (unsigned int *) malloc(sizeof(unsigned int));
        (*G)->students[i]->grades = (unsigned int *) malloc(sizeof(unsigned int));
      } else {


        //check if we already have a grade for this assignment.
        for(j = 0; j< (*G)->students[i]->num_grades; j++){
          if((*G)->students[i]->assignment_ids[j] == assignment_id){
            //we already have this grade, just update it. no need to realloc.
            //after arrays have been allocated, add the grade and assignment id to the student.
            (*G)->students[i]->grades[j] = grade;
            return 0;
          }
        }

        //if we made it here need to realloc arrays b/c we're adding a totally new grade.
        temp = realloc((*G)->students[i]->assignment_ids, sizeof(unsigned int)*((*G)->students[i]->num_grades+1));
        if(temp == NULL){
          return -1; //realloc failed.
        } else {
          (*G)->students[i]->assignment_ids =temp;
        }

        temp = realloc((*G)->students[i]->grades, sizeof(unsigned int)*((*G)->students[i]->num_grades+1));
        if(temp == NULL){
          return -1; //realloc failed.
        } else {
          (*G)->students[i]->grades =temp;
        }

      }

      //after arrays have been allocated, add the grade and assignment id to the student.
      (*G)->students[i]->assignment_ids[(*G)->students[i]->num_grades] = assignment_id;
      (*G)->students[i]->grades[(*G)->students[i]->num_grades] = grade;
      (*G)->students[i]->num_grades++; //add 1 to the number of grades that they have.
      free(student_name);
      return 0;
    }
  }
  return -1;

}
int Delete_Assignment(Gradebook ** G, char * name){
  int i =0, j = 0, k=0, has_assignment = 0;
  Assignment ** temp;
  Assignment * to_delete = NULL;
  unsigned int * new_ids = NULL, * new_grades = NULL;


  for(i=0; i < (*G)->num_assignments; i++){
    if(strcmp((*G)->assignments[i]->name, name) == 0){
      //if we've found the assignment whose name matches.
      to_delete = (*G)->assignments[i];

    }
  }

  if(to_delete == NULL){ //assignment of that name does not exist.
    return -1;
  }


  //remove this assignment from students lists of grades.
  for(i=0; i< (*G)->num_students; i++){

    //check first that it has the right id.
    has_assignment = 0;
    for(j=0; j< (*G)->students[i]->num_grades; j++){
      if((*G)->students[i]->assignment_ids[j] == to_delete->id){
        has_assignment = 1;
        break;
      }
    }

    if(has_assignment){
      //if this is the only assignment, then just free the arrays.
      if((*G)->students[i]->num_grades - 1 == 0){
        free((*G)->students[i]->assignment_ids);
        free((*G)->students[i]->grades);
        (*G)->students[i]->assignment_ids = NULL;
        (*G)->students[i]->grades = NULL;
      } else {
        new_ids = (unsigned int *) malloc(sizeof(unsigned int)*((*G)->students[i]->num_grades -1));
        new_grades = (unsigned int *) malloc(sizeof(unsigned int)*((*G)->students[i]->num_grades -1));
        //if we have the assignment, then we need to redo the grade arrays for each student.
        k = 0;
        for(j=0; j < (*G)->students[i]->num_grades; j++){

          if((*G)->students[i]->assignment_ids[j] != to_delete->id){
            //add this to the list of new ids.
            new_ids[k] = (*G)->students[i]->assignment_ids[j];
            new_grades[k] = (*G)->students[i]->grades[j];
            k++;
          }

        }

        //free the old ids + grades lists + point those toward the new arrays.
        free((*G)->students[i]->assignment_ids);
        free((*G)->students[i]->grades);

        (*G)->students[i]->assignment_ids = new_ids;
        (*G)->students[i]->grades = new_grades;

      }
      (*G)->students[i]->num_grades--; //remove the assignment.

    }


  }

  //after removing the assignment id from all students, we can free the name of the assignment, the assignment
  //and redo the assignments array.
  //allocate a new array for assignments.
  if((*G)->num_assignments -1 == 0){

    //free the to delete  assignment.
    free(to_delete->name);
    free(to_delete);

    //free the assignments array.
    free((*G)->assignments);
    (*G)->num_assignments--;  //remove the assignment.
  } else {
    //need to reallocate array.
    k = 0;
    temp = (Assignment **) malloc(sizeof(Assignment *)*(*G)->num_assignments);
    for(i = 0; i < (*G)->num_assignments; i++){
      if((*G)->assignments[i]->id != to_delete->id){
        temp[k++] = (*G)->assignments[i]; //just copying over the pointer.
      }
    }

    //free the to delete  assignment.
    free(to_delete->name);
    free(to_delete);


    //free the old assignments array and reassign.
    free((*G)->assignments);
    (*G)->assignments = temp;
    (*G)->num_assignments--;



  }



  return 0;
}

int Delete_Student(Gradebook ** G, char * first, char * last){
  char * student_name;
  int i, k;
  unsigned int name_len;
  Student ** temp;
  Student * to_delete = NULL;
  //combine the first and last name to get the student name.
  name_len = strlen(first) + strlen(last) + 2;
  student_name = (char *) malloc(sizeof(char)*(name_len + 1));
  //concatenate the first and last name.
  strcpy(student_name, last);
  strcpy((student_name + strlen(last)), ", ");
  strcpy(student_name + (strlen(last) + 2), first);
  student_name[name_len] = '\0';


  //first just check to make sure that the student exists.
  for(i=0; i < (*G)->num_students; i++){
    if(strcmp(student_name, (*G)->students[i]->name) == 0){
      to_delete  = (*G)->students[i];
    }
  }


  if(to_delete == NULL){
    //name not found.
    free(student_name);
    return -1; //error.
  }

  //found the student.

  if((*G)->num_students -1 == 0 ){
    //if this is going to be the last student to remove, then just free the student structs
    //and return.
    free(to_delete->name);
    free(to_delete->assignment_ids);
    free(to_delete->grades);
    free(to_delete);
    free((*G)->students);
    (*G)->num_students--; //remove the last student.
  } else {
    //need to reallocate the array of students.
    k=0;
    temp = (Student **) malloc(sizeof(Student *)*(*G)->num_students -1);
    for(i=0; i < (*G)->num_students; i++){
      if(strcmp(student_name, (*G)->students[i]->name) != 0){
        temp[k++] = (*G)->students[i];
      }
    }

    //after we have a new list allocated, we can delete the node and replace students.
    free(to_delete->name);
    free(to_delete->assignment_ids);
    free(to_delete->grades);
    free(to_delete);
    free((*G)->students);
    (*G)->students = temp;
    (*G)->num_students--;
  }


  return 0;

}
int main(int argc, char *argv[]) {
  CmdLineResult R;
  Gradebook * G;
  int i,j, p, success, gradebook_bytes, g;
  double w;
  R = parse_cmdline(argc, argv);

  if(R.good != 0){
    printf("invalid\n");
    return(255);
  }

  //now that the command line input is good, we can go ahead and validate the Gradebook.
  success = read_Gradebook_from_path(R.gradebook_name, R.key, &G, &gradebook_bytes);

  if(success == -1){ //failed to read from file
    //or failed to decrypt. Key is either wrong, or gradebook was tampered with.
    printf("invalid\n");
    return(255);
  }


  //make changes to gradebook according to action.
  switch(R.action_type){
    case add_assignment:
      //check that point + weights are numbers.
      p = atoi(R.cmd_args[1]);
      if(p < 0){
        //invalid, p should be unsigned.
        printf("invalid\n");
        return(255);
      }

      w = strtod(R.cmd_args[2], NULL);
      if(w > 1.0 || w < 0.0){
        printf("invalid\n");
        return(255);
      }


      success = Add_Assignment(&G,R.cmd_args[0],(unsigned int) p, w);
      //printf("added assignment\n");
      if(success == -1){ //if assignment failed, print invalid and return.
        printf("invalid\n");
        return(255);
      }
      break;
    case delete_assignment:
      success = Delete_Assignment(&G, R.cmd_args[0]);
      if(success == -1){
        printf("invalid\n");
        return(255);
      }
      break;
    case add_student:
      success = Add_Student(&G, R.cmd_args[0], R.cmd_args[1]);
      if(success == -1){
        printf("invalid\n");
        return(255);
      }
      break;
    case delete_student:
      success = Delete_Student(&G, R.cmd_args[0], R.cmd_args[1]);
      if(success == -1){
        printf("invalid\n");
        return(255);
      }
      break;
    case add_grade:
      g = atoi(R.cmd_args[3]);
      if(g < 0){
        //invalid, g should be unsigned.
        printf("invalid\n");
        return(255);
      }

      success = Add_Grade(&G, R.cmd_args[1], R.cmd_args[2], R.cmd_args[0], g);

      if(success == -1){
        printf("invalid\n");
        return(255);
      }
      break;
  }

  //printf("Num Assignments: %d\n", G->num_assignments);
//  printf("Assignments: \n");

  /*for(i=0; i < G->num_assignments; i++){
    //printf("\n", i);
    printf("name length: %d\n", G->assignments[i]->name_len);
    printf("%s\n", G->assignments[i]->name);
  }

  printf("finished printing assignments\n");

  printf("Num students: %d\n", G->num_students);
  printf("Students: \n");
  */
/*  for(i=0; i < G->num_students; i++){
    //printf("\n", i);
    //printf("name length: %d\n", G->students[i]->name_len);
    printf("%s\n", G->students[i]->name);
    for(j=0; j < G->students[i]->num_grades; j++){
      printf("Grade: %d, %d\n", G->students[i]->assignment_ids[j], G->students[i]->grades[j]);
    }
  }

  printf("finished printing students\n");
  printf("writing to gradebook\n");*/
  //write changes to gradebook back to file.
  write_Gradebook_to_path(R.gradebook_name, R.key, G);


  return(0);
}
