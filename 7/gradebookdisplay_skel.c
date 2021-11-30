#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/queue.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <regex.h>
#include <getopt.h>

#include "data.h"

#define LEN 16

int verbose = 0;

typedef struct _CmdLineResult {
  int     good; //proper command format
  char * gradebook_name; //gradebook name
  unsigned char * key; //key
  PrintActions action_type; //gets action type.
  PrintMode mode; //alphabetical = 0, grade order = 1;
  int num_cmds;
  char ** cmd_args; //other arguments provided by user.
} CmdLineResult;




int Print_Assignment(Gradebook ** G, char * assignment_name, PrintMode mode) {
  Assignment * to_print = NULL;
  int i = 0, j=0, k = 0;
  unsigned int num_students = 0, grade1, grade2;
  Student ** students_to_print;
  Student * temp;
  //check that the assignment exists.
  for(i=0; i < (*G)->num_assignments; i++){
    if(strcmp(assignment_name, (*G)->assignments[i]->name) == 0){
      to_print = (*G)->assignments[i];
      break;
    }
  }

  if(to_print == NULL){
    return -1; //could not find the assignment.
  }

  //we found the assignment, now we need to print all students associated with the assignment.
  //first find the number of students associated with this assignment.
  for(i = 0; i < (*G)->num_students; i++){
    for(j = 0; j < (*G)->students[i]->num_grades; j++){
      if((*G)->students[i]->assignment_ids[j] == to_print->id){
        num_students++;
        break; //there are no duplicate assignments.
      }
    }
  }

  //allocate an array to hold pointers of students. so we can copy them in in the specified order.
  students_to_print = (Student **) malloc(sizeof(Student *)*num_students);

  //go back and add all students with a grade for that assignment to the array.
  for(i = 0; i < (*G)->num_students; i++){
    for(j = 0; j < (*G)->students[i]->num_grades; j++){
      if((*G)->students[i]->assignment_ids[j] == to_print->id){
        students_to_print[k++] = (*G)->students[i];
        break;
      }
    }
  }

  if(mode == alphabetical){
    //just use bubble sort to sort student pointers based on name.
    for(i = 0; i < num_students; i++){
      for(j= i+1; j < num_students; j++){
        if(strcasecmp(students_to_print[i]->name, students_to_print[j]->name) > 0){
          temp = students_to_print[i];
          students_to_print[i] = students_to_print[j];
          students_to_print[j] = temp;
        }
      }
    }
  } else if (mode == grade_order ){
    for(i = 0; i < num_students; i++){
      for(j= i+1; j < num_students; j++){
        //first find the grades of each student.
        grade1 = 0;
        grade2 = 0;
        for(k=0; k < students_to_print[i]->num_grades; k++){
          //find this student's grade.
          if(students_to_print[i]->assignment_ids[k] == to_print->id){
            //this is this grade.
            grade1 = students_to_print[i]->grades[k];
            break;
          }
        }

        for(k=0; k < students_to_print[j]->num_grades; k++){
          //find this student's grade.
          if(students_to_print[j]->assignment_ids[k] == to_print->id){
            //this is this grade.
            grade2 = students_to_print[j]->grades[k];
            break;
          }
        }
        if(grade1 < grade2){
          temp = students_to_print[i];
          students_to_print[i] = students_to_print[j];
          students_to_print[j] = temp;
        }
      }
    }
  } else {
    free(students_to_print);
    return -1; //we have already checked this but just to be sure.
  }

  //print the array.
  for(i=0; i < num_students; i++){
    grade1 = 0;
    //find the grade for this student.
    for(k=0; k < students_to_print[i]->num_grades; k++){
      //find this student's grade.
      if(students_to_print[i]->assignment_ids[k] == to_print->id){
        //this is this grade.
        grade1 = students_to_print[i]->grades[k];
        break;
      }
    }

    //print the student and their grade.
    printf("(%s, %d)\n", students_to_print[i]->name, grade1);

  }

  free(students_to_print);
  return 0 ;
}

int Print_Student(Gradebook ** G, char * first, char * last) {
  unsigned int name_len;
  char * student_name;
  Student * student = NULL;
  int i = 0, j = 0;

  name_len = strlen(first) + strlen(last) + 2;
  student_name = (char *) malloc(sizeof(char)*(name_len + 1));
  //concatenate the first and last name.
  strcpy(student_name, last);
  strcpy((student_name + strlen(last)), ", ");
  strcpy(student_name + (strlen(last) + 2), first);
  student_name[name_len] = '\0';

  //make sure that the student exists.
  for(i=0; i < (*G)->num_students; i++){
    if(strcmp(student_name, (*G)->students[i]->name) == 0){
      student = (*G)->students[i];
    }
  }

  if(student == NULL){
    return -1; //student did not exist.
  }

  //free student name, we're done with it.
  free(student_name);
  //print all the grades for that student.
  for(i=0; i < student->num_grades;i++){
    //go find an assignment with this id.
    for(j = 0; j < (*G)->num_assignments; j++){
      if((*G)->assignments[j]->id == student->assignment_ids[i]){
        //print this assignment.
        printf("(%s, %d)\n", (*G)->assignments[j]->name, student->grades[i]);
      }
    }
  }

  return 0 ;
}

int Print_Final(Gradebook ** G, PrintMode mode){
  Student ** students;
  Student * temp;
  int i =0, j=0, k=0;
  unsigned int grade1, grade2;
  double final_grade;
  double temp_grade;
  double * final_grades;

  //print the final grades for each student.

  //first copy all the students into a new array and sort them either by name or grade.
  //allocate an array to hold pointers of students. so we can copy them in in the specified order.
  students = (Student **) malloc(sizeof(Student *)*(*G)->num_students);
  final_grades = (double *) malloc(sizeof(double)*(*G)->num_students);
  //add all students to the array.
  //and calculate their final grades.
  for(i = 0; i < (*G)->num_students; i++){
    students[i] = (*G)->students[i];
    final_grade = 0;
    for(j = 0; j < (*G)->students[i]->num_grades; j++){
      //for each one of their assignments that they have, got through and multiply

      for(k=0; k < (*G)->num_assignments; k++){
          //check for the assigment id that matches the one from the student.
          if((*G)->assignments[k]->id == (*G)->students[i]->assignment_ids[j]){
            //calculate final grade.
            final_grade += (((double) students[i]->grades[j])/((double)(*G)->assignments[k]->points))*(*G)->assignments[k]->weight;
          }
      }
    }

    final_grades[i] = final_grade;

  }


  if(mode == alphabetical){
    //just use bubble sort to sort student pointers based on name.
    for(i = 0; i < (*G)->num_students; i++){
      for(j= i+1; j < (*G)->num_students; j++){
        if(strcasecmp(students[i]->name, students[j]->name) > 0){
          //switch the students and switch the final grades.
          temp = students[i];
          students[i] = students[j];
          students[j] = temp;

          temp_grade = final_grades[i];
          final_grades[i] = final_grades[j];
          final_grades[j] = temp_grade;

        }
      }
    }
  } else if (mode == grade_order ){
    for(i = 0; i < (*G)->num_students; i++){
      for(j= i+1; j < (*G)->num_students; j++){
        if(final_grades[i] < final_grades[j]){
          temp = students[i];
          students[i] = students[j];
          students[j] = temp;

          temp_grade = final_grades[i];
          final_grades[i] = final_grades[j];
          final_grades[j] = temp_grade;
        }
      }
    }
  } else {
    free(students);
    return -1; //we have already checked this but just to be sure.
  }

  //now print the students and their corresponding final grades.

  for(i=0; i < (*G)->num_students; i++){
    printf("(%s, %g)\n", students[i]->name, final_grades[i]);
  }

  free(students); //free students.
  free(final_grades); //free final grades.
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
  R.mode = none;

  //define options.
  struct option options[] = {
    {"N", required_argument, 0, 'a'},
    {"K", required_argument, 0, 'b'},
    {"PA", no_argument, 0, 'c'},
    {"PS", no_argument, 0, 'd'},
    {"PF", no_argument, 0, 'e'},
    {"A", no_argument, 0, 'f'},
    {"G", no_argument, 0, 'g'},
    {"AN", required_argument, 0, 'h'},
    {"FN", required_argument, 0, 'i'},
    {"LN", required_argument, 0, 'j'},
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
  if(strcmp(argv[5], "-PA") != 0 && strcmp(argv[5], "-PS") != 0 && strcmp(argv[5], "-PF") != 0){
      //if we don't match any of the actions at this argument, fail.
      return R;
  }


  while((c=getopt_long_only(argc, argv, "a:b:cdefgh:i:j:", options, &option_index)) != -1){
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
        if(R.num_cmds == -1){ //needs to be the only action.
            R.action_type = print_assignment;
            R.num_cmds = 1;
            R.cmd_args = (char **) malloc(sizeof(char *)*R.num_cmds); //array of 1 pointers to arguments.
            for(i = 0; i < R.num_cmds; i++){
              R.cmd_args[i] = NULL; //set all to NULL so that we can check that they are all there at the end.
            }
        } else {
          return R;
        }
        break;
      case 'd':
        if(R.num_cmds == -1){ //needs to be the only action.
            R.action_type = print_student;
            R.num_cmds = 2;
            R.cmd_args = (char **) malloc(sizeof(char *)*R.num_cmds); //array of 1 pointers to arguments.
            for(i = 0; i < R.num_cmds; i++){
              R.cmd_args[i] = NULL; //set all to NULL so that we can check that they are all there at the end.
            }
        } else {
          return R;
        }
        break;
      case 'e':
        if(R.num_cmds == -1){ //needs to be the only action.
            R.action_type = print_final;
            R.num_cmds = 0;
        } else {
          return R;
        }
        break;
      case 'f':
        //alphabetical
        if(R.num_cmds == -1 || R.mode != none){
          //we haven't specified an action yet.
          //or the mode has already been specified.
          return R;
        }

        R.mode = alphabetical;
        break;
      case 'g':
        //grade order
        if(R.num_cmds == -1 || R.mode != none){
          //we haven't specified an action yet.
          return R;
        }

        R.mode = grade_order;
        break;
      case 'h':
        //assignment name
        if(R.num_cmds == -1){
          //we haven't specified an action yet.
          return R;
        }
        //only required for AA, DA, AG.
        if (R.action_type == print_assignment){
          R.cmd_args[0] = optarg; //point to the string specified.
        } else {
          return R;
        }
        break;
        break;
      case 'i':
        //first name
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
        if(R.action_type == print_student ){
          R.cmd_args[0] = optarg; //point to the first name specified.
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
        if(R.action_type == print_student){
          R.cmd_args[1] = optarg;
        } else {
          return R;  //any other action bad.
        }
        break;

    }
  }

  //check that the action and cmd_args are filled out, as well as the filename fits format.
  if(R.gradebook_name != NULL && R.key != NULL && R.num_cmds != -1){
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


int main(int argc, char *argv[]) {
  int   opt,len;
  char  *logpath = NULL;
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

  switch(R.action_type){
    case print_assignment:
      success = Print_Assignment(&G, R.cmd_args[0], R.mode);
      if(success == -1){
        printf("invalid\n");
        return(255);
      }
      break;
    case print_student:
      if(R.mode != none){
        //providing A/G when not required.
        printf("invalid\n");
        return(255);
      }

      success = Print_Student(&G, R.cmd_args[0], R.cmd_args[1]);
      if(success == -1){
        printf("invalid\n");
        return(255);
      }
      break;
    case print_final:

      success = Print_Final(&G, R.mode);

      if(success == -1){
        printf("invalid\n");
        return(255);
      }
      break;

  }
  //write back to file because we want to change the IV after giving access to the plaintext.
  write_Gradebook_to_path(R.gradebook_name, R.key, G);
  return 0;

}
