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

GBAddFlag find_action(char *option, int len){
  GBAddFlag result = NoFlag;

  if(strncmp(option, "-AA", len) == 0 && strlen("-AA") == strnlen(option, len))
    result = AA;
  else if(strncmp(option, "-DA", len) == 0 && strlen("-DA") == strnlen(option, len))
    result = DA;
  else if(strncmp(option, "-AS", len) == 0 && strlen("-AS") == strnlen(option, len))
    result = AS;
  else if(strncmp(option, "-DS", len) == 0 && strlen("-DS") == strnlen(option, len))
    result = DS;
  else if(strncmp(option, "-AG", len) == 0 && strlen("-AG") == strnlen(option, len))
    result = AG;
  return result;
}

GBAddFlag find_req(char *option, int len){
  GBAddFlag result = NoFlag;
  if(strncmp(option, "-AN", len) == 0 && strlen("-AN") == strnlen(option, len))
    result = AN;
  else if(strncmp(option, "-FN", len) == 0 && strlen("-FN") == strnlen(option, len))
    result = FN;
  else if(strncmp(option, "-LN", len) == 0 && strlen("-LN") == strnlen(option, len))
    result = LN;
  else if(strncmp(option, "-P", len) == 0 && strlen("-P") == strnlen(option, len))
    result = P;
  else if(strncmp(option, "-W", len) == 0 && strlen("-W") == strnlen(option, len))
    result = W;
  else if(strncmp(option, "-G", len) == 0 && strlen("-G") == strnlen(option, len))
    result = G;
  return result;
}

/*Must do struct stuff in main*/
int main(int argc, char *argv[]) {
  int decryp_result;
  FILE *fp;
  unsigned char *struct_buffer;
  unsigned char *modified_buff;
  /**/
  unsigned char *encrypted_buffer;
  unsigned char *decrypted_buffer;
  


  unsigned char *key;
  unsigned char *iv;
  unsigned char *tag;

  unsigned char *new_tag;
  unsigned char *new_iv;
  /**/
  unsigned char *re_encryp_buffer;
  
  /*For holding the action option*/
  GBAddFlag act_flag;

  /*For holding the required option*/
  GBAddFlag req_flag;
  
  /*The gradebook to load*/
  Gradebook loaded_book;
  int i;
  int decrypt_result;

  GBADDCMD curr_cmd = NOTVALID;

  /*For holding relevant requirements*/
  char assign_name[101] = {'\0'};
  char f_name[101] = {'\0'};
  char l_name[101] = {'\0'};
  int points = -1;
  int grade = -1;
  float weight = -1;

  if(argc < 6){
    printf("Not enough arguements to run gradebookadd\n");
    handleErrors();
  }

  /*Check if argv[1] is -N*/
  if(strncmp(argv[1], "-N", strlen(argv[1])) != 0 || strlen("-N") != strlen(argv[1])){
    printf("Needs -N option\n");
    handleErrors();
  }

  /*Check if argv[2] is a valid gradebook name*/
  if(is_book_name_valid(argv[2], strlen(argv[2])) == 0){
    printf("Invalid gradebook name\n");
    handleErrors();
  }

  if(file_test(argv[2]) == 0){
    printf("Gradebook does not exist\n");
    handleErrors();
  }

  /*Test if argv[3] is -K*/
  if(strncmp(argv[3], "-K", strlen(argv[3])) != 0 || strlen("-K") != strlen(argv[3])){
    printf("Needs -K option\n");
    handleErrors();
  }

  /*Test if argv[4] is a valid key*/
  if(is_key_valid(argv[4], strlen(argv[4])) == 0){
    printf("Invalid key\n");
    handleErrors();
  }

  /*Must determine which, if any action flag is present*/
  act_flag = find_action(argv[5], strlen(argv[5]));
  if(act_flag == NoFlag){
    printf("Invalid action option. Please enter: -AA, -DA, -AS, -DS, -AG\n");
    handleErrors();
  }
  

  switch(act_flag){
    case AA: /*Add Assignment*/
      for(i = 6; i < argc; i+=2){
        int stop = i + 1;
        if(stop == argc){
          printf("Missing arguement\n");
          handleErrors();
        }
        req_flag = find_req(argv[i], strlen(argv[i]));
        if(req_flag == NoFlag || req_flag == FN || req_flag == LN || req_flag == G){/*Invalid options*/
          printf("Missing/Incorrect -AA option\n");
          handleErrors();
        }
        /*If option was valid*/
        if(req_flag == AN){
          if(is_assign_name_valid(argv[i+1], strlen(argv[i+1]))){
            strncpy(assign_name, argv[i+1], 99);
            assign_name[100] = '\0';
          }else{
            printf("Invalid Assignment Name\n");
            handleErrors();
          }
        }
        else if(req_flag == P){
          if(!is_number(argv[i+1])){
            printf("Argument for -P must be a number\n");
            handleErrors();
          }
          int input = atoi(argv[i+1]);
          if(is_points_valid(input)){
            points = input;
          }else{
            printf("Invalid -P points given\n");
            handleErrors();
          }
        }
        else if(req_flag == W){
          if(!is_float(argv[i+1])){
            printf("Argument for -W must be a float\n");
            handleErrors();
          }
          float input = atof(argv[i+1]);
          if(is_weight_valid(input)){
            weight = input;
          }else{
            printf("Invalid -W weight given\n");
            handleErrors();
          }
        }
      }
      curr_cmd = ADDASSIGN;
      break;
    case DA: /*Delete Assignment*/
      for(i = 6; i < argc; i+=2){
        int stop = i + 1;
        if(stop == argc){
          printf("Missing arguement\n");
          handleErrors();
        }
        req_flag = find_req(argv[i], strlen(argv[i]));
        if(req_flag == NoFlag || req_flag == FN || req_flag == LN || req_flag == G || req_flag == P || req_flag == W){/*Invalid options*/
          printf("Missing/Incorrect -DA option\n");
          handleErrors();
        }
        /*If option was valid*/
        else{
          if(is_assign_name_valid(argv[i+1], strlen(argv[i+1]))){
            strncpy(assign_name, argv[i+1], 99);
            assign_name[100] = '\0';
          }else{
            printf("Invalid Assignment Name\n");
            handleErrors();
          }
        }
      }
      curr_cmd = DELASSIGN;
      break;
    case AS: /*Add Student*/
      for(i = 6; i < argc; i+=2){
        int stop = i + 1;
        if(stop == argc){
          printf("Missing arguement\n");
          handleErrors();
        }
        req_flag = find_req(argv[i], strlen(argv[i]));
        if(req_flag == NoFlag || req_flag == AN || req_flag == G || req_flag == P || req_flag == W){/*Invalid options*/
          printf("Missing/Incorrect -AS option\n");
          handleErrors();
        }
        /*If option was valid*/
        if(req_flag == FN){
          if(is_stud_name_valid(argv[i+1], strlen(argv[i+1]))){
            strncpy(f_name, argv[i+1], 99);
            f_name[100] = '\0';
          }else{
            printf("Invalid Student First Name\n");
            handleErrors();
          }
        }
        else {
          if(is_stud_name_valid(argv[i+1], strlen(argv[i+1]))){
            strncpy(l_name, argv[i+1], 99);
            l_name[100] = '\0';
          }else{
            printf("Invalid Student Last Name\n");
            handleErrors();
          }
        }
      }
      curr_cmd = ADDSTUD;
      break;
    case DS: /*Delete Student*/
      for(i = 6; i < argc; i+=2){
        int stop = i + 1;
        if(stop == argc){
          printf("Missing arguement\n");
          handleErrors();
        }
        req_flag = find_req(argv[i], strlen(argv[i]));
        if(req_flag == NoFlag || req_flag == AN || req_flag == G || req_flag == P || req_flag == W){/*Invalid options*/
          printf("Missing/Incorrect -DS option\n");
          handleErrors();
        }
        /*If option was valid*/
        if(req_flag == FN){
          if(is_stud_name_valid(argv[i+1], strlen(argv[i+1]))){
            strncpy(f_name, argv[i+1], 99);
            f_name[100] = '\0';
          }else{
            printf("Invalid Student First Name\n");
            handleErrors();
          }
        }
        else {
          if(is_stud_name_valid(argv[i+1], strlen(argv[i+1]))){
            strncpy(l_name, argv[i+1], 99);
            l_name[100] = '\0';
          }else{
            printf("Invalid Student Last Name\n");
            handleErrors();
          }
        }
      }
      curr_cmd = DELSTUD;
      break;
    case AG: /*Add Grade*/
      for(i = 6; i < argc; i+=2){
        int stop = i + 1;
        if(stop == argc){
          printf("Missing arguement\n");
          handleErrors();
        }
        req_flag = find_req(argv[i], strlen(argv[i]));
        if(req_flag == NoFlag || req_flag == P || req_flag == W){/*Invalid options*/
          printf("Missing/Incorrect -AG option\n");
          handleErrors();
        }
        /*If option was valid*/
        if(req_flag == AN){
          if(is_assign_name_valid(argv[i+1], strlen(argv[i+1]))){
            strncpy(assign_name, argv[i+1], 99);
            assign_name[100] = '\0';
          }else{
            printf("Invalid Assignment Name\n");
            handleErrors();
          }
        }
        else if(req_flag == FN){
          if(is_stud_name_valid(argv[i+1], strlen(argv[i+1]))){
            strncpy(f_name, argv[i+1], 99);
            f_name[100] = '\0';
          }else{
            printf("Invalid Student First Name\n");
            handleErrors();
          }
        }
        else if(req_flag == LN){
          if(is_stud_name_valid(argv[i+1], strlen(argv[i+1]))){
            strncpy(l_name, argv[i+1], 99);
            l_name[100] = '\0';
          }else{
            printf("Invalid Student Last Name\n");
            handleErrors();
          }
        }
        else if(req_flag == G){
          if(!is_number(argv[i+1])){
            printf("Argument for -G must be a number\n");
            handleErrors();
          }
          int input = atoi(argv[i+1]);
          if(is_points_valid(input)){
            grade = input;
          }else{
            printf("Invalid -G grade given\n");
            handleErrors();
          }
        }
      }
      curr_cmd = ADDGRADE;
      break;
  }


  switch(curr_cmd){
    case ADDASSIGN:
      if(is_assign_name_valid(assign_name, strlen(assign_name)) &&
      is_points_valid(points) && is_weight_valid(weight)){
        curr_cmd = ADDASSIGN;
      } else {
        printf("Failure\n");
        curr_cmd = NOTVALID;
      }
      break;
    case DELASSIGN:
      if(is_assign_name_valid(assign_name, strlen(assign_name))){
        curr_cmd = DELASSIGN;
      } else {
        printf("Failure\n");
        curr_cmd = NOTVALID;
      }
      break;
    case ADDSTUD:
      if(is_stud_name_valid(f_name, strlen(f_name)) && is_stud_name_valid(l_name, strlen(l_name))){
        curr_cmd = ADDSTUD;
      }else{
        curr_cmd = NOTVALID;
      }
      break;
    case DELSTUD:
      if(is_stud_name_valid(f_name, strlen(f_name)) && is_stud_name_valid(l_name, strlen(l_name))){
        curr_cmd = DELSTUD;
      }else{
        curr_cmd = NOTVALID;
      }
      break;
    case ADDGRADE:
      if(is_assign_name_valid(assign_name, strlen(assign_name)) && 
      is_stud_name_valid(f_name, strlen(f_name)) && 
      is_stud_name_valid(l_name, strlen(l_name)) && 
      is_grade_valid(grade)){
        curr_cmd = ADDGRADE;
      }else{
        curr_cmd = NOTVALID;
      }
      break;
  }
  if(curr_cmd == NOTVALID){
    printf("Unacceptable input\n");
    handleErrors();
  }

  /*Open up the gradebook and perform further validation before performing action. Use curr_cmd flag to determine action*/

  /*Attempt to open the file*/
  fp = fopen(argv[2], "r");

  if (fp == NULL){
    printf("setup: fopen() error could not create file\n");
    handleErrors();
  }
  
  /*Set up memory*/
  iv = (unsigned char *)malloc(sizeof(unsigned char) * LEN);
  tag = (unsigned char *)malloc(sizeof(unsigned char) * LEN);
  
  /**/
  key = (unsigned char *)malloc(sizeof(unsigned char) * LEN);
  struct_buffer = (char*)malloc(sizeof(Gradebook));
  
  encrypted_buffer = (char*)malloc(sizeof(Gradebook));
  decrypted_buffer = (char*)malloc(sizeof(Gradebook));
  
  /*Retrieve key*/
  sscanf(argv[4], "%02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx"
    , &key[0], &key[1], &key[2], &key[3], &key[4], &key[5], &key[6], &key[7], &key[8], &key[9], &key[10], &key[11], &key[12]
    , &key[13], &key[14], &key[15]);
  

  /*Read unencrypted data from file*/
  fread(iv, LEN, 1, fp);
  fread(tag, LEN, 1, fp);
  fread(encrypted_buffer, sizeof(Gradebook), 1, fp);

  /*Attempt decryption*/
  decryp_result = gcm_decrypt(encrypted_buffer, sizeof(Gradebook), tag, key, iv, LEN, decrypted_buffer);
  if(decryp_result == -1){
    free(iv);
    free(tag);
    free(key);
    free(struct_buffer);
    free(encrypted_buffer);
    free(decrypted_buffer);
    fclose(fp);
    handleErrors();
  }
  /*Load data into gradebook*/
  memcpy(&loaded_book,decrypted_buffer, sizeof(Gradebook));
  

  /*User curr_cmd to get Mode*/
  if(curr_cmd == ADDASSIGN){
    int ctr;
    int assign_index;
    if(loaded_book.num_assign == GBSIZE){/*Max size reached*/
      printf("Max capacity of gradebook reached. Please delete an assignmet before attempting to add\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buffer);
      free(decrypted_buffer);
      fclose(fp);
      handleErrors();
    }
    /*Perform additional checks before adding assignment*/
    if(loaded_book.num_assign != 0){
      /*Does this assignment name exist already*/
      for(ctr = 0; ctr < loaded_book.num_assign; ctr++){
        /*Must perform strncmp and strlen checks to ensure equality*/
        if(strncmp(assign_name, loaded_book.all_assign[ctr].assign_name, strlen(assign_name)) == 0 && 
        strlen(assign_name) == strlen(loaded_book.all_assign[ctr].assign_name)){
          printf("An assignment with this name already exists\n");
          free(iv);
          free(tag);
          free(key);
          free(struct_buffer);
          free(encrypted_buffer);
          free(decrypted_buffer);
          fclose(fp);
          handleErrors();
        }
      }
      /*Chech if new sum of all weights will be more than 1*/
      if(loaded_book.all_weights + weight > 1){
        printf("Total weights cannot exceed 1\n");
        free(iv);
        free(tag);
        free(key);
        free(struct_buffer);
        free(encrypted_buffer);
        free(decrypted_buffer);
        fclose(fp);
        handleErrors();
      }
    }
    /*If validation passes, add the assignment.*/
    ctr = 0;
    assign_index = loaded_book.num_assign;
    /*Create the assignment*/

    /*Add assignment to the overall gradebook*/
    strncpy(loaded_book.all_assign[assign_index].assign_name, assign_name, 99);
    loaded_book.all_assign[assign_index].assign_name[100] = '\0';
    loaded_book.all_assign[assign_index].total = points;
    loaded_book.all_assign[assign_index].student_grade = 0;
    loaded_book.all_assign[assign_index].weight = weight;


    /*Add assignment to each student*/
    for(ctr = 0; ctr < loaded_book.num_stud; ctr++){
      strncpy(loaded_book.student_arr[ctr].stud_assigns[assign_index].assign_name, assign_name, 99);
      loaded_book.student_arr[ctr].stud_assigns[assign_index].assign_name[100] = '\0';
      loaded_book.student_arr[ctr].stud_assigns[assign_index].total = points;
      loaded_book.student_arr[ctr].stud_assigns[assign_index].student_grade = 0;
      loaded_book.student_arr[ctr].stud_assigns[assign_index].weight = weight;
    }
    loaded_book.all_weights += weight;
    loaded_book.num_assign++;
  }
  else if(curr_cmd == DELASSIGN){
    int ctr;
    int index = -1;
    /*Check if assignment with this name is in the gradebook*/
    if(loaded_book.num_assign == 0){
      printf("Gradebook is already empty\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buffer);
      free(decrypted_buffer);
      fclose(fp);
      handleErrors();
    }
    /*Check for existence*/
    for(ctr = 0; ctr < loaded_book.num_assign; ctr++){
      if(strncmp(assign_name, loaded_book.all_assign[ctr].assign_name, strlen(assign_name)) == 0 && 
      strlen(assign_name) == strlen(loaded_book.all_assign[ctr].assign_name)){
        index = ctr;
        break;
      }
    }
    if(index == -1){
      printf("Assignment not found\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buffer);
      free(decrypted_buffer);
      fclose(fp);
      handleErrors();
    } else {/*Delete here*/
      int size = loaded_book.num_assign;
      int stud_ctr;

      /*Adjust total weights*/
      loaded_book.all_weights -= loaded_book.all_assign[index].weight;

      if(size == 1){/*Case only one assignment in gradebook*/
        for(stud_ctr = 0; stud_ctr < loaded_book.num_stud; stud_ctr++){/*Delete from students*/
          /*Adjust student final!*/
          loaded_book.student_arr[stud_ctr].final -= loaded_book.student_arr[stud_ctr].stud_assigns[index].weight * ((float)loaded_book.student_arr[stud_ctr].stud_assigns[index].student_grade / (float)loaded_book.student_arr[stud_ctr].stud_assigns[index].total);
        }
      } 
      else if(index == size - 1){
          for(stud_ctr = 0; stud_ctr < loaded_book.num_stud; stud_ctr++){/*Delete from students*/
            /*Adjust student final!*/
            loaded_book.student_arr[stud_ctr].final -= loaded_book.student_arr[stud_ctr].stud_assigns[index].weight * ((float)loaded_book.student_arr[stud_ctr].stud_assigns[index].student_grade / (float)loaded_book.student_arr[stud_ctr].stud_assigns[index].total);
            /*Delete from student's assignment list*/
            loaded_book.student_arr[stud_ctr].stud_assigns[index] = loaded_book.student_arr[stud_ctr].stud_assigns[index + 1];
          }
        }
      else {
        /*Perform deletion*/
        while(index < size - 1){
          loaded_book.all_assign[index] = loaded_book.all_assign[index + 1]; /*Delete from gradebook*/
          for(stud_ctr = 0; stud_ctr < loaded_book.num_stud; stud_ctr++){/*Delete from students*/
            /*Adjust student final!*/
            loaded_book.student_arr[stud_ctr].final -= loaded_book.student_arr[stud_ctr].stud_assigns[index].weight * ((float)loaded_book.student_arr[stud_ctr].stud_assigns[index].student_grade / (float)loaded_book.student_arr[stud_ctr].stud_assigns[index].total);
            /*Delete from student's assignment list*/
            loaded_book.student_arr[stud_ctr].stud_assigns[index] = loaded_book.student_arr[stud_ctr].stud_assigns[index + 1];
          }
          index++;
        }
      }
      /*Cleanup*/
       
      loaded_book.num_assign--;
    }


  }
  else if(curr_cmd == ADDSTUD){
    if(loaded_book.num_stud == GBSIZE){/*Max size reached*/
      printf("Max capacity of gradebook reached. Please delete a student before attempting to add\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buffer);
      free(decrypted_buffer);
      fclose(fp);
      handleErrors();
    }
    int ctr;
    /*Check if student with the same first and last name already exists*/
    for(ctr = 0; ctr < loaded_book.num_stud; ctr++){
      if((strncmp(f_name, loaded_book.student_arr[ctr].first_name, strlen(f_name)) == 0 && 
      strlen(f_name) == strlen(loaded_book.student_arr[ctr].first_name)) &&
      (strncmp(l_name, loaded_book.student_arr[ctr].last_name, strlen(l_name)) == 0 && 
      strlen(l_name) == strlen(loaded_book.student_arr[ctr].last_name))){
        printf("Student already exists in gradebook\n");
        free(iv);
        free(tag);
        free(key);
        free(struct_buffer);
        free(encrypted_buffer);
        free(decrypted_buffer);
        fclose(fp);
        handleErrors();
      }
    }
    /*Add the student*/
    Student stud_to_add;

    stud_to_add.final = 0;

    strncpy(stud_to_add.first_name, f_name, 99);
    stud_to_add.first_name[100] = '\0';

    strncpy(stud_to_add.last_name, l_name, 99);
    stud_to_add.last_name[100] = '\0';

    /*Give new student the assignments already in the gradebook*/
    for(ctr = 0; ctr < loaded_book.num_assign; ctr++){
      stud_to_add.stud_assigns[ctr] = loaded_book.all_assign[ctr];
    }
    loaded_book.student_arr[loaded_book.num_stud] = stud_to_add;
    loaded_book.num_stud++;
  }
  else if(curr_cmd == DELSTUD){
    /*If there are no students*/
    if(loaded_book.num_stud == 0){
      printf("There are no students in gradebook to delete\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buffer);
      free(decrypted_buffer);
      fclose(fp);
      handleErrors();
    }
    int ctr;
    int index = -1;
    for(ctr = 0; ctr < loaded_book.num_stud; ctr++){
      if((strncmp(f_name, loaded_book.student_arr[ctr].first_name, strlen(f_name)) == 0 && 
      strlen(f_name) == strlen(loaded_book.student_arr[ctr].first_name)) &&
      (strncmp(l_name, loaded_book.student_arr[ctr].last_name, strlen(l_name)) == 0 && 
      strlen(l_name) == strlen(loaded_book.student_arr[ctr].last_name))){
        /*Delete the student*/
        index = ctr;
        while(index < loaded_book.num_stud - 1){
          loaded_book.student_arr[index] = loaded_book.student_arr[index + 1];
          index++;
        }
        loaded_book.num_stud--;
        break;
      }
    } 
    if(index == -1){
      printf("Student does not exists in gradebook\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buffer);
      free(decrypted_buffer);
      fclose(fp);
      handleErrors();
    }

  }
  else if(curr_cmd == ADDGRADE){
    /*Further Vaidation*/
    int ctr;
    int assign_index = -1;
    int stud_index = -1;

    /*Is assignment in gradebook*/
    for(ctr = 0; ctr < loaded_book.num_assign; ctr++){
      if(strncmp(assign_name, loaded_book.all_assign[ctr].assign_name, strlen(assign_name)) == 0 && 
      strlen(assign_name) == strlen(loaded_book.all_assign[ctr].assign_name)){
        assign_index = ctr;
        break;
      }
    }

    /*Is student in gradebook*/
    for(ctr = 0; ctr < loaded_book.num_stud; ctr++){
      if((strncmp(f_name, loaded_book.student_arr[ctr].first_name, strlen(f_name)) == 0 && 
      strlen(f_name) == strlen(loaded_book.student_arr[ctr].first_name)) &&
      (strncmp(l_name, loaded_book.student_arr[ctr].last_name, strlen(l_name)) == 0 && 
      strlen(l_name) == strlen(loaded_book.student_arr[ctr].last_name))){
        stud_index = ctr;
        break;
      }
    } 

    if(stud_index == -1){
      printf("The entered student does not exist in the gradebook\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buffer);
      free(decrypted_buffer);
      fclose(fp);
      handleErrors();
    }
    if(assign_index == -1 || stud_index == -1){
      printf("The entered assignment does not exist in the gradebook\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buffer);
      free(decrypted_buffer);
      fclose(fp);
      handleErrors();
    }
    float curr_weight = loaded_book.student_arr[stud_index].stud_assigns[assign_index].weight;
    int old_grade = loaded_book.student_arr[stud_index].stud_assigns[assign_index].student_grade;
    int total_points = loaded_book.student_arr[stud_index].stud_assigns[assign_index].total;

    /*Add the grade*/
    /*First update student final*/
    loaded_book.student_arr[stud_index].final -= curr_weight * ((float)old_grade / (float)total_points);
    loaded_book.student_arr[stud_index].final += curr_weight * ((float)grade / (float)total_points);

    /*Then update student grade*/
    loaded_book.student_arr[stud_index].stud_assigns[assign_index].student_grade = grade;
  }

  /*Write modified Gradebook to Gradebook file*/

  fclose(fp);

  fp = fopen(argv[2], "w");

  if (fp == NULL){
    printf("setup: fopen() error could not reopen file\n");
    handleErrors();
  }

  modified_buff = (char*)malloc(sizeof(Gradebook));

  re_encryp_buffer = (char*)malloc(sizeof(Gradebook));

  memcpy(modified_buff, &loaded_book, sizeof(Gradebook));

  /*Attempt encryption*/
  new_iv = (unsigned char *)malloc(sizeof(unsigned char) * LEN);
  new_tag = (unsigned char *)malloc(sizeof(unsigned char) * LEN);
  key_gen(new_iv, LEN);
  gcm_encrypt(modified_buff, sizeof(Gradebook), key, new_iv, LEN, re_encryp_buffer, new_tag);

  fwrite(new_iv, LEN, 1, fp);
  
  fwrite(new_tag, LEN, 1, fp);
  
  fwrite(re_encryp_buffer, sizeof(Gradebook), 1, fp);
  
  
  free(iv);
  free(new_iv);
  free(tag);
  free(key);
  free(struct_buffer);

  free(modified_buff);
  free(re_encryp_buffer);

  fclose(fp);

  return (0);
}
