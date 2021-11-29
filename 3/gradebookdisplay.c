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

#include<limits.h>

#include "data.h"

void print_Assignment(Assignment assignment, Student student) {
  printf("(%s, %s, %d)\n", student.last_name, student.first_name, assignment.student_grade);
  return;
}

void print_Student(Student student, int num_assigns){
  int i = 0;
  for(i = 0; i < num_assigns; i++){
    printf("(%s, %d)\n", student.stud_assigns[i].assign_name, student.stud_assigns[i].student_grade);
  }
  return;
}

void print_Final(Student student){
  printf("(%s, %s, %g)\n", student.last_name, student.first_name, student.final);
  return;
}

int has_index_been_visited(int curr_set[], int index_to_find, int up_to){
  int ans = 0;
  int i = 0;
  for(i = 0; i < up_to; i++){
    if(index_to_find == curr_set[i]){
      ans = 1;
      break;
    }
  }
  return ans;
};

int compare_students(Student a, Student b){
  int result;
  result = strcmp(a.last_name, b.last_name);
  if(result == 0){
    return strcmp(a.first_name, b.first_name);
  }
  return result;
}

GBDisplayFlag find_action(char *option, int len){
  GBDisplayFlag result = NONE;

  if(strncmp(option, "-PA", len) == 0 && strlen("-PA") == strnlen(option, len))
    result = PA;
  else if(strncmp(option, "-PS", len) == 0 && strlen("-PS") == strnlen(option, len))
    result = PS;
  else if(strncmp(option, "-PF", len) == 0 && strlen("-PF") == strnlen(option, len))
    result = PF;
  return result;
}

GBDisplayFlag find_req(char *option, int len){
  GBDisplayFlag result = NONE;
  if(strncmp(option, "-AN", len) == 0 && strlen("-AN") == strnlen(option, len))
    result = DISP_AN;
  else if(strncmp(option, "-FN", len) == 0 && strlen("-FN") == strnlen(option, len))
    result = DISP_FN;
  else if(strncmp(option, "-LN", len) == 0 && strlen("-LN") == strnlen(option, len))
    result = DISP_LN;
  else if(strncmp(option, "-A", len) == 0 && strlen("-A") == strnlen(option, len))
    result = DISP_A;
  else if(strncmp(option, "-G", len) == 0 && strlen("-G") == strnlen(option, len))
    result = DISP_G;
  return result;
}

int main(int argc, char *argv[]) {
  int decryp_result;
  FILE *fp;
  unsigned char * encrypted_buf;
  unsigned char *struct_buffer;
  unsigned char *key;
  unsigned char *iv;
  unsigned char *tag;

  Gradebook loaded_book;
  GBDisplayFlag act_flag;
  GBDisplayFlag req_flag;
  GBDisplayCMD curr_cmd = INVALID;
  int num_A = 0;
  int num_G = 0;
  int i;
  int visited_indexes[GBSIZE];

  char assign_name[101] = {'\0'};
  char f_name[101] = {'\0'};
  char l_name[101] = {'\0'};

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

  /*Determine if option is a flag and what flag it is.*/
  act_flag = find_action(argv[5], strlen(argv[5]));
  if(act_flag == NONE){
    printf("Invalid action option. Please enter: -PA, -PS, -PF\n");
    handleErrors();
  }

  /*Change behavior based on action chosen*/
  switch(act_flag){
    case PA:
      for(i = 6; i < argc; i++){
        if(i == argc){
          printf("Missing arguement\n");
          handleErrors();
        }
        req_flag = find_req(argv[i], strlen(argv[i]));
        if(req_flag == DISP_FN || req_flag == DISP_LN){
          printf("Not a valid option for -PA\n");
          handleErrors();
        }
        /*If the option is -G*/
        if(req_flag == DISP_G){
          if(num_A >= 1 || num_G >= 1){/*Already processed a -A or -G*/
            printf("-PA can oly have one of -G or -A\n");
            handleErrors();
          } else {/*This is the first -G we encountered*/
            num_G++;
          }
        }
        /*If the option is -A*/
        else if(req_flag == DISP_A){
          if(num_A >= 1 || num_G >= 1){/*Already processed a -A or -G*/
            printf("-PA can only have one of -G or -A\n");
            handleErrors();
          } else {/*This is the first -G we encountered*/
            num_A++;
          }
        }
        else if(req_flag == DISP_AN){/*We must check if a valid name is in i+1*/
          if(i+1 >= argc){/*Assignemnt name not entered*/
            printf("The option -AN requires a valid assignment name\n");
            handleErrors();
          }else{/*There is SOMETHING in i+1*/
            if(is_assign_name_valid(argv[i+1], strlen(argv[i+1]))){
              strncpy(assign_name, argv[i+1], 99);
              assign_name[100] = '\0';
              i++;
            }else{
              printf("Invalid Assignment Name\n");
              handleErrors();
            }
          }
        }
      }
      curr_cmd = PRINT_ASSIGN;
      break;
    case PS:
      for(i = 6; i < argc; i+=2){
        int stop = i + 1;
        if(stop == argc){
          printf("Missing arguement\n");
          handleErrors();
        }
        req_flag = find_req(argv[i], strlen(argv[i]));

        if(req_flag == DISP_AN || req_flag == DISP_G || req_flag == DISP_A){
          printf("Not a valid option for -PS");
          handleErrors();
        }
        /*If option was valid*/
        if(req_flag == DISP_FN){
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
      curr_cmd = PRINT_STUD;
      break;
    case PF:
      for(i = 6; i < argc; i++){
        if(i == argc){
          printf("Missing arguement\n");
          handleErrors();
        }
        req_flag = find_req(argv[i], strlen(argv[i]));
        if(req_flag == DISP_FN || req_flag == DISP_LN || req_flag == DISP_AN){
          printf("Not a valid option for -PF\n");
          handleErrors();
        }
        /*If the option is -G*/
        if(req_flag == DISP_G){
          if(num_A >= 1 || num_G >= 1){/*Already processed a -A or -G*/
            printf("-PA can oly have one of -G or -A\n");
            handleErrors();
          } else {/*This is the first -G we encountered*/
            num_G++;
          }
        }
        /*If the option is -A*/
        else if(req_flag == DISP_A){
          if(num_A >= 1 || num_G >= 1){/*Already processed a -A or -G*/
            printf("-PA can only have one of -G or -A\n");
            handleErrors();
          } else {/*This is the first -G we encountered*/
            num_A++;
          }
        }
      }
      curr_cmd = PRINT_FINAL;
      break;
  }

  /*Open up the gradebook*/
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
  encrypted_buf = (char*)malloc(sizeof(Gradebook));

  sscanf(argv[4], "%02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx"
    , &key[0], &key[1], &key[2], &key[3], &key[4], &key[5], &key[6], &key[7], &key[8], &key[9], &key[10], &key[11], &key[12]
    , &key[13], &key[14], &key[15]);
  

  /*Read unencrypted data from file*/
  fread(iv, LEN, 1, fp);
  fread(tag, LEN, 1, fp);
  fread(encrypted_buf, sizeof(Gradebook), 1, fp);

  /*Attempt decryption*/
  decryp_result = gcm_decrypt(encrypted_buf, sizeof(Gradebook), tag, key, iv, LEN, struct_buffer);
  if(decryp_result == -1){
    free(iv);
    free(tag);
    free(key);
    free(struct_buffer);
    free(encrypted_buf);
    fclose(fp);
    handleErrors();
  }
  

  /*Load data into gradebook*/
  memcpy(&loaded_book,struct_buffer, sizeof(Gradebook));

  /*Perform command*/
  if(curr_cmd == PRINT_ASSIGN){
    int ctr;
    int index = -1;
    /*Check if assignment name exists in the gradebook*/
    if(loaded_book.num_assign == 0){
      printf("Gradebook has no assignments\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buf);
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
      free(encrypted_buf);
      fclose(fp);
      handleErrors();
    }

    /*Check if there is an -A or -G*/
    if(num_G == 0 && num_A == 0){
      printf("The -PA command requires either a -A or -G option flag\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buf);
      fclose(fp);
      handleErrors();
    }
    /*Start printing the Assignment*/
    if(loaded_book.num_stud == 0){/*No students*/
      printf("No students in this gradebook\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buf);
      fclose(fp);
      handleErrors();
    }
    if(num_G == 1){/*Print in grade order highest to lowest*/
      int printed_assigns;
      int in_set = 0;
      


      while(printed_assigns < loaded_book.num_stud){
        Student stud_with_highest;
        int highest_studs_index;
        int curr_highest = -1;

        for(ctr = 0; ctr < loaded_book.num_stud; ctr++){/*Determine students to print first*/
          Student curr_stud = loaded_book.student_arr[ctr];
          if(curr_stud.stud_assigns[index].student_grade > curr_highest){
            /*Has this student been visited before? If not...*/
            if(has_index_been_visited(visited_indexes, ctr, in_set) == 0){
              highest_studs_index = ctr;
              stud_with_highest = curr_stud;
              curr_highest = curr_stud.stud_assigns[index].student_grade;
              
            }

          }
          
        }
        visited_indexes[in_set] = highest_studs_index;
        in_set++;
        print_Assignment(stud_with_highest.stud_assigns[index], stud_with_highest);
        
        /*Print here*/
        printed_assigns++;
      }

    }
    else{/*Print in alphabetical order*/
      int printed_assigns = 0;
      int in_set = 0;
      int i = 0;
      int print_index;
      Student to_print = loaded_book.student_arr[0];
      while(printed_assigns < loaded_book.num_stud){
        i = 0;
        int j = i+1;
        while(j < loaded_book.num_stud){
          if(compare_students(loaded_book.student_arr[i], loaded_book.student_arr[j]) <= 0){
            if(has_index_been_visited(visited_indexes, i, in_set) == 0){
              to_print = loaded_book.student_arr[i];
              print_index = i;
            } else{
              to_print = loaded_book.student_arr[j];
              print_index = j;
            }
            
          } else {
            if(has_index_been_visited(visited_indexes, j, in_set) == 0){
              to_print = loaded_book.student_arr[j];
              print_index = j;
            } 
            else if((has_index_been_visited(visited_indexes, i, in_set) == 0)){
              to_print = loaded_book.student_arr[i];
              print_index = i;
            }

          }
          
          j++;

        }
        
        /**/
        print_Assignment(to_print.stud_assigns[index], to_print);
        visited_indexes[in_set] = print_index;
        in_set++;
        printed_assigns++;
      }
    }
  }
  else if(curr_cmd == PRINT_STUD){
    if(loaded_book.num_assign == 0){
      printf("Gradebook has no assignments\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buf);
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
        /*Print this student*/
        index = ctr;
        print_Student(loaded_book.student_arr[index], loaded_book.num_assign);
        break;
      }
    }
    if(index == -1){
      printf("No such student exists");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buf);
      fclose(fp);
      handleErrors();
    }

  }
  else if(curr_cmd == PRINT_FINAL){/*Print the finals of all the students*/
    /*Check if there is an -A or -G*/
    if(num_G == 0 && num_A == 0){
      printf("The -PA command requires either a -A or -G option flag\n");
      free(iv);
      free(tag);
      free(key);
      free(struct_buffer);
      free(encrypted_buf);
      fclose(fp);
      handleErrors();
    }
    int ctr;

    if(num_G == 1){/*Print finals in order highest to lowest*/
      int printed_finals;
      int in_set = 0;

      while(printed_finals < loaded_book.num_stud){
        Student stud_with_highest;
        int highest_studs_index;
        float curr_highest = -1;

        for(ctr = 0; ctr < loaded_book.num_stud; ctr++){/*Determine students to print first*/
          Student curr_stud = loaded_book.student_arr[ctr];
          if(curr_stud.final > curr_highest){
            /*Has this student been visited before? If not...*/
            if(has_index_been_visited(visited_indexes, ctr, in_set) == 0){
              highest_studs_index = ctr;
              stud_with_highest = curr_stud;
              curr_highest = curr_stud.final;
              
            }

          }
          
        }
        visited_indexes[in_set] = highest_studs_index;
        in_set++;
        print_Final(stud_with_highest);
        
        /*Print here*/
        printed_finals++;
      }

    }
    else{/*Print in alphabetical order*/
      int printed_assigns = 0;
      int in_set = 0;
      int i = 0;
      int print_index;
      Student to_print = loaded_book.student_arr[0];
      while(printed_assigns < loaded_book.num_stud){
        i = 0;
        int j = i+1;
        while(j < loaded_book.num_stud){
          if(compare_students(loaded_book.student_arr[i], loaded_book.student_arr[j]) <= 0){
            if(has_index_been_visited(visited_indexes, i, in_set) == 0){
              to_print = loaded_book.student_arr[i];
              print_index = i;
            } else{
              to_print = loaded_book.student_arr[j];
              print_index = j;
            }
            
          } else {
            if(has_index_been_visited(visited_indexes, j, in_set) == 0){
              to_print = loaded_book.student_arr[j];
              print_index = j;
            } 
            else if((has_index_been_visited(visited_indexes, i, in_set) == 0)){
              to_print = loaded_book.student_arr[i];
              print_index = i;
            }

          }
          
          j++;

        }
        
        /**/
        print_Final(to_print);
        visited_indexes[in_set] = print_index;
        in_set++;
        printed_assigns++;
      }
    }
  }
  free(iv);
  free(tag);
  free(key);
  free(struct_buffer);
  free(encrypted_buf);
  fclose(fp);
  return 0;
}
