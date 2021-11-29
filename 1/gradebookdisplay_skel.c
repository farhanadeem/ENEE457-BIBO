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
#define LEN 32 


//action holds what action we will be doing
//ActionType action = add_assignment;

typedef struct CmdLineResult {
  int good;
  char *grade_book_name;
  unsigned char *key;
  ActionType action;
  //below are values for the options
  char *AN_value;
  char *FN_value;
  char *LN_value;
  char *order;

} CmdLineResult;


//enum {ADD_ASSIGNMENT, DELETE_ASSIGNMENT, ADD_STUDENT, DROP_STUDENT, ADD_GRADE} action = NULL;




int validate_pair(char *option, char *value){
int checker = 1;

   if(strcmp(option,"-AN") == 0){
      for(int j = 0; j < strlen(value); j++){
	  if(!((value[j] >= 'A' && value[j] <= 'Z') || (value[j] >= 'a' && value[j] <= 'z') || (value[j] >= '1' && value[j]<='9'))){
             checker = 0;
          }
      }

   }else if(strcmp(option,"-FN")==0){
      for(int j = 0; j < strlen(value); j++){
	  if(!((value[j] >= 'a'&& value[j] <= 'z') || (value[j] >= 'A' && value[j] <= 'Z'))){ 
	     //printf("onh my god\n");
             checker = 0;
          }    
      }            
      
   }else if(strcmp(option,"-LN")==0){
      for(int j = 0; j < strlen(value); j++){
	  if(!((value[j] >= 'a'&& value[j] <= 'z') || (value[j] >= 'A' && value[j] <= 'Z'))){ 
	     //printf("onh my god\n");
             checker = 0;
          }    
      }            
      
   }else{
      checker = 0;
   }
   
   //check the decimal counter before returning

return checker;
}


ActionType find_action(char *argv[]){

      if(strcmp(argv[5],"-PA")==0){
         return print_assignment;
      }else if(strcmp(argv[5],"-PS")==0){
         return print_student;
      }else if(strcmp(argv[5],"-PF")==0){
         return print_final;
      }else{
         return nonce;
      }
}

CmdLineResult parse_cmdline(int argc, char *argv[]) {
  CmdLineResult Result = {0, "", "",nonce, NULL, NULL, NULL, NULL}; //initialize with whatever
  int opt,r = -1;

  Result.good = 1;

//We need to loop through argc AFTER WE FIND THE ACTION because we dont know how many arguments there are after the action.
  
   if(argc==1){ 
      printf("\nNo Extra Command Line Argument Passed Other Than Program Name\n"); 
      Result.good = 0;
   }
   
   //make sure we have the minimum number of arguments so we dont segmentation fault, after we find the action we can determine argc_new
   if(argc >= 6){
      int trash = 0;
     //ignore trash above, we good in here
   }else{
      Result.good = 0;
      return Result;
   }
   
   
 
   // checking if argv[1] is -N
   if(!(strcmp(argv[1], "-N")==0)){ 
      Result.good = 0;
   }
   
   //now check that argv[2] is valid filename
   for(int e = 0; e < strlen(argv[2]); e++){
	if(!((argv[2][e] >= 'A' && argv[2][e] <= 'Z') || (argv[2][e] >= 'a' && argv[2][e] <= 'z') || (argv[2][e] >= '1' && argv[2][e] <='9')||(argv[2][e] == '.')||(argv[2][e] == '_'))){
          Result.good = 0;
	   //printf("in here");
	  return Result;
       }
   }
   Result.grade_book_name = argv[2];
   
   // checking that argv[3] is -K
   if(!(strcmp(argv[3], "-K")==0)){
      Result.good = 0;
      	  // printf("in here");
   } 
      //now check that argv[4] is a valid key
   for(int e = 0; e < strlen(argv[4]); e++){
      if( !((argv[4][e] >= '0' && argv[4][e] <= '9') || (argv[4][e] >= 'a' && argv[4][e] <= 'f'))){//check if the key is valid
         Result.good = 0;
         return Result;
      }
   }
   Result.key = (unsigned char*)argv[4];


   //Now we figure out what the action is and assign in the struct
   if (strlen(argv[5]) != 3){ Result.good = 0; return Result;}
   Result.action = find_action(argv);
   if(Result.action == nonce){
         Result.good = 0;
   }
   
   //now we need to see what the action is and find the option/value pairing associated
   //int k = 0;
   //printf("THIS IS THE ACTION: %d\n", Result.action);
   switch(Result.action){
   
      case print_assignment:

         if(argc-6 < 3){Result.good = 0; return Result;} //to prevent seg fault should be at least 6 arguments after add_assignment act.
	 //-AN (-A|-G) could come in any order and as many times(keep the last value) so just loop through and keep updating
	 for(int index = 6; index < argc; index++){
	    if(strcmp(argv[index],"-A")==0 || strcmp(argv[index],"-G")==0){
	         Result.order = argv[index];
	         //printf("%s\n",Result.order);	         
	       
	    }else if(strcmp(argv[index],"-AN")==0){
	       if(validate_pair(argv[index],argv[index+1])==1){
	         Result.AN_value = argv[index+1];
	         index++;
	         //printf("%s\n",Result.AN_value);	         
	       }else{
	          Result.good = 0;
	       }	    
	    
	    }else{
	       Result.good = 0;
	    }
	 
	 }
	 //MAKE SURE THAT ALL REQUIRED FIELDS WERE ACTUALLY GIVEN
	 if(!Result.AN_value){Result.good = 0; return Result;}
	 if(!Result.order){Result.good = 0; return Result;}	 
      break;
      
      case print_student:

         if(argc-6 < 2){Result.good = 0; return Result;} //to prevent seg fault, be at least 2 arguments after delete_assignment action
          //-FN and -LN is the only accepted options, can be given multiple times, accept the last value.      
         for(int index = 6; index < argc; index = index + 2){
           
            if(strcmp(argv[index],"-FN")==0){
               if(validate_pair(argv[index],argv[index+1])==1){
                  Result.FN_value = argv[index+1];
                  //printf("%s\n",Result.FN_value);
               }else{
                  Result.good = 0;
               }
            
            }else if(strcmp(argv[index],"-LN")==0){
                  if(validate_pair(argv[index],argv[index+1])==1){
                     Result.LN_value = argv[index+1];
                     //printf("%s\n",Result.LN_value);
                  }else{
                     Result.good = 0;
                  }            
            
            }else{
               Result.good = 0;
            }
         }
	 if(!Result.FN_value){Result.good = 0; return Result;}    
	 if(!Result.LN_value){Result.good = 0; return Result;}
      break;
	
      case print_final:
      
         if(argc-6 < 1){Result.good = 0; return Result;}//to prevent seg fault, be at least 4 arguments after add_student action
         for(int index = 6; index < argc; index++){
            if(strcmp(argv[index],"-G")==0 || strcmp(argv[index],"-A")==0){
                  Result.order = argv[index];
                  //printf("%s\n",Result.order);
            }else{
               Result.good = 0;
            }
         }
	 if(!Result.order){Result.good = 0; return Result;}	 
      break;

      
      

      case nonce:
         printf("Invalid\n");
         Result.good = 0;
      break;


      
      default:
         //This means that the action passed in is not valid, should be nonce.
         printf("Invalid\n");
         Result.good = 0;
      break;
   
   }//END OF SWITCH 
  
  return Result;
}


int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}






Student* create_student_node(char *first_name, char *last_name, int grade){
   Student *node = (Student*)malloc(sizeof(Student));
   
   node->first_name = malloc(sizeof(strlen(first_name)));
   strcpy(node->first_name,first_name);

   node->last_name = malloc(sizeof(strlen(last_name)));
   strcpy(node->last_name,last_name);
   
   node->grade = grade;
   node->next = NULL;
   return node;
}

Assignment* create_assignment_node(char *assignment_name, int points, float weight, char *token){
   Assignment *node = (Assignment*)malloc(sizeof(Assignment));
   
   node->assignment_name = malloc(sizeof(strlen(assignment_name)));
   strcpy(node->assignment_name,assignment_name);

   node->points = points;
   
   node->weight = weight;
   
   //HERE WE CREATE THE STUDENT LINKED LIST FOR THE ASSIGNMENT
   Student *head = NULL;
   char *first_name;
   char *last_name;
   int grade;
   while(token != NULL){
      if((token = strtok(NULL," ")) == NULL){break;}              
      first_name = token;
      if((token = strtok(NULL," ")) == NULL){break;}              
      last_name = token;
      if((token = strtok(NULL," ")) == NULL){break;}              
      grade = atoi(token);
       
       if(head == NULL){  //IF THE LINKED LIST IS EMPTY THEN START WITH HEAD
          head = create_student_node(first_name,last_name,grade);

	}else{
           Student *student_ptr = head;
           while(student_ptr->next != NULL){
              student_ptr = student_ptr->next;
           }
           student_ptr->next = create_student_node(first_name,last_name,grade);
	}//END OF ELSE      
   }//END OF WHILE
   
   
   
   node->students = head;

   node->next = NULL;
   return node;
}



//THIS FUNCTION HELPER BELOW WILL LOAD THE DATA AFTER IT WAS VRFY AND DECRYPTED AND LOAD IN THE DATA TO THE Gradebook struct
Gradebook load_data(char *theFILENAME, FILE *fp){
   Gradebook the_Gradebook = {0, 0, 0.0, NULL, NULL, 0, 0};//to do with gradebook pointer do (Gradebook*)malloc(sizeof(Gradebook));
   char *line = "";
   size_t len = 0;
   char *token; 
   char *first_name = "";
   char *last_name = "";

   char *ass_name = "";
   int points;
   float weight;
   Assignment *ass_head = NULL;
   //BELOW IS THE STUDENT LINKED LIST FOR EXISTENCE
   Student *head = NULL;
   
   
   
   while((getline(&line,&len,fp)) != EOF){
      token = strtok(line," ");//GET FIRST PART OF THE LINE
   
      //NOW we loop throught to get other parts of the line
      while(token != NULL){
         //THIS IS THE FIRST LINE WHICH SHOULD BE "num_of_assignments:"
         if(strcmp(token,"num_of_assignments:")==0){
            token = strtok(NULL," ");
            the_Gradebook.num_of_assignments = atoi(token);
            //printf("%d\n",the_Gradebook.num_of_assignments);
            break;
         }else if(strcmp(token,"num_of_students:")==0){
            token = strtok(NULL," ");
            the_Gradebook.num_of_students = atoi(token);
            //printf("%d\n",the_Gradebook.num_of_students);
            break;
         }else if(strcmp(token,"sum_of_weights:")==0){
            token = strtok(NULL," ");
            the_Gradebook.sum_of_weights = atof(token);
            //printf("%f\n",the_Gradebook.sum_of_weights);
            break;
         }else if(strcmp(token,"\n")==0){
            break;
         }

         
         //NOW WE CHECK TO SEE IF THERE ARE ANY STUDENTS THAT EXIST AND LOAD THEM IN
         if(the_Gradebook.num_of_students > 0 && strcmp(token,"Student_Names:")==0){
            while(token != NULL){
               if((token = strtok(NULL," ")) == NULL){break;}              
	       first_name = token;
               if((token = strtok(NULL," ")) == NULL){break;}              
	       last_name = token;

	       if(head == NULL){  //IF THE LINKED LIST IS EMPTY THEN START WITH HEAD
                  head = create_student_node(first_name,last_name,0);

		}else{
                   Student *student_ptr = head;
                   while(student_ptr->next != NULL){
                      student_ptr = student_ptr->next;
                   }
                   student_ptr->next = create_student_node(first_name,last_name,0);
		}//END OF ELSE
		
            }//END OF WHILE
           
            break;//go get the next line
         }//END OF IF



         //NOW WE CHECK TO SEE IF THERE ARE ANY ASSIGNMENTS THAT EXIST AND LOAD THEM IN
         if(the_Gradebook.num_of_assignments > 0 && strcmp(token,"assignment_name:")==0){
               //create new student linked list for the new assignment               
               if((token = strtok(NULL," ")) == NULL){break;}              
               ass_name = token;
               //printf("%s\n",ass_name);
               if((token = strtok(NULL," ")) == NULL){break;}                            
               if((token = strtok(NULL," ")) == NULL){break;}                            
	       points = atoi(token);
               if((token = strtok(NULL," ")) == NULL){break;}                            	       
               if((token = strtok(NULL," ")) == NULL){break;}                          
               weight = atof(token);
               
               
               if(ass_head == NULL){
                  ass_head = create_assignment_node(ass_name,points,weight,token);
               }else{
                  Assignment *ass_ptr = ass_head;
                  while(ass_ptr->next != NULL){
                     ass_ptr = ass_ptr->next;
                  }
                  ass_ptr->next = create_assignment_node(ass_name,points,weight,token);
               }
               
            break;
         }//END OF IF         
      
         //DEFAULT GO GET NEXT LINE       
         break;
      }// END OF INNER WHILE LOOP
   }// END OF PARENT WHILE LOOP, MEANING WE ARE EOF


   the_Gradebook.students = head;
   //printf("%s\n",the_Gradebook.students->next->first_name);
   the_Gradebook.assignments = ass_head;
   //printf("%s\n",the_Gradebook.assignments->assignment_name);
   return the_Gradebook;
}




Gradebook remove_student_from_exsistence(Gradebook My_Gradebook, char *FN_VALUE, char *LN_VALUE){
   Student *stu_ptr = My_Gradebook.students;
   Student *stu_ptr_1 = NULL;
   //Student *tail_stu_ptr = My_Gradebook.students;
   Student *prev_stu_ptr = NULL;
   
   //FIRST CHECK IF HEAD NEEDS TO BE REMOVED
   if(strcmp(stu_ptr->first_name, FN_VALUE)==0 && strcmp(stu_ptr->last_name, LN_VALUE)==0){
      My_Gradebook.num_of_students -= 1;
      My_Gradebook.students = My_Gradebook.students->next;
      free(stu_ptr);
      stu_ptr = NULL;
      //printf("%s", My_Gradebook.students->first_name);
      return My_Gradebook;
   }
   
   //NOW CHECK IF THE TAIL NEEDS TO BE REMOVED
   while(stu_ptr->next != NULL){
      prev_stu_ptr = stu_ptr;
      stu_ptr = stu_ptr->next;
   }
   if(strcmp(stu_ptr->first_name, FN_VALUE)==0 && strcmp(stu_ptr->last_name, LN_VALUE)==0){
      prev_stu_ptr->next = NULL;
      free(stu_ptr);
      stu_ptr = NULL;
      My_Gradebook.num_of_students -= 1;
      return My_Gradebook;
   }
   
   stu_ptr = My_Gradebook.students;
   //NOW IF THE NODE IS IN THE MIDDLE OF THE EXSISTANCE LINKED LIST
   while(stu_ptr->next != NULL){
      if(strcmp(stu_ptr->first_name, FN_VALUE)==0 && strcmp(stu_ptr->last_name, LN_VALUE)==0){
      stu_ptr_1->next = stu_ptr->next;
      free(stu_ptr);
      stu_ptr = NULL;
      My_Gradebook.num_of_students -= 1;
      break;
      }
      stu_ptr_1 = stu_ptr;
      stu_ptr = stu_ptr->next;
   }

   return My_Gradebook;
}




Student* remove_student_from_list(Student *students, char *FN_VALUE, char *LN_VALUE){
   Student *stu_ptr = students;
   Student *stu_ptr_1 = NULL;
   Student *prev_stu_ptr = NULL;
           if(students == NULL){
              return students;
           }
	   //FIRST CHECK IF HEAD NEEDS TO BE REMOVE
	   if(strcmp(stu_ptr->first_name, FN_VALUE)==0 && strcmp(stu_ptr->last_name, LN_VALUE)==0){
	      students = students->next;
	      free(stu_ptr);
	      stu_ptr = NULL;
	      return students;
	   }
	   
	   //NOW CHECK IF THE TAIL NEEDS TO BE REMOVED
	   while(stu_ptr->next != NULL){
	      prev_stu_ptr = stu_ptr;
	      stu_ptr = stu_ptr->next;
	   }
	   if(strcmp(stu_ptr->first_name, FN_VALUE)==0 && strcmp(stu_ptr->last_name, LN_VALUE)==0){
	      prev_stu_ptr->next = NULL;
	      free(stu_ptr);
	      stu_ptr = NULL;
              return students;
	   }
	   
	   stu_ptr = students;
	   //NOW IF THE NODE IS IN THE MIDDLE OF THE EXSISTANCE LINKED LIST
	   while(stu_ptr->next != NULL){
	      if(strcmp(stu_ptr->first_name, FN_VALUE)==0 && strcmp(stu_ptr->last_name, LN_VALUE)==0){
	         stu_ptr_1->next = stu_ptr->next;
	         free(stu_ptr);
	         stu_ptr = NULL;
	         break;
	      }
	      stu_ptr_1 = stu_ptr;
	      stu_ptr = stu_ptr->next;
	   }
	 
	 return students;
}


Gradebook remove_student_from_assignments(Gradebook My_Gradebook, char *FN_VALUE, char *LN_VALUE){
   Assignment *ass_ptr = My_Gradebook.assignments;

   while(ass_ptr != NULL){

      ass_ptr->students = remove_student_from_list(ass_ptr->students, FN_VALUE,LN_VALUE);
      ass_ptr = ass_ptr->next;

   }
   
   return My_Gradebook;
}

Student* print_grade_alphabetically_and_remove(Student *students){

   //START WITH FIRST STUDENT AS HIGHEST PRIORITY
   Student *highest_priority_student = students;
   Student *stu_ptr = students->next;
   
    //IF THERE IS ONLY ONE STUDENT THEN JUST PRINT REMOVE AND RETURN
    if(students->next == NULL){
       printf("(%s, %s, %d)\n", students->last_name, students->first_name, students->grade);	      
       return students = remove_student_from_list(students, students->first_name, students->last_name);
    }


      while(stu_ptr != NULL){
         //IF THERE IS A DIFFERENCE ASSIGN
         if(strcasecmp(stu_ptr->last_name, highest_priority_student->last_name) < 0 ){
            highest_priority_student = stu_ptr;
         
         //IF THEY ARE THE SAME THEN COMPARE THE FIRST NAMES
         }else if(strcasecmp(stu_ptr->last_name, highest_priority_student->last_name) == 0){
            if(strcasecmp(stu_ptr->first_name, highest_priority_student->first_name) < 0 ){
               highest_priority_student = stu_ptr;
            }                                                                                
         }
         stu_ptr = stu_ptr->next; 
      }
   
   printf("(%s, %s, %d)\n",highest_priority_student->last_name, highest_priority_student->first_name, highest_priority_student->grade);
   return students = remove_student_from_list(students, highest_priority_student->first_name, highest_priority_student->last_name);

}

Student* print_grade_hightolow_and_remove(Student *students){

   //START WITH FIRST STUDENT AS HIGHEST PRIORITY
   Student *highest_priority_student = students;
   Student *stu_ptr = students->next;
   
    //IF THERE IS ONLY ONE STUDENT THEN JUST PRINT REMOVE AND RETURN
    if(students->next == NULL){
       printf("(%s, %s, %d)\n", students->last_name, students->first_name, students->grade);	      
       return students = remove_student_from_list(students, students->first_name, students->last_name);
    }


      while(stu_ptr != NULL){
         //IF THERE IS A DIFFERENCE ASSIGN
         if(stu_ptr->grade > highest_priority_student->grade){
            highest_priority_student = stu_ptr;
         
         }
      stu_ptr = stu_ptr->next; 
      }
   
   printf("(%s, %s, %d)\n",highest_priority_student->last_name, highest_priority_student->first_name, highest_priority_student->grade);
   return students = remove_student_from_list(students, highest_priority_student->first_name, highest_priority_student->last_name);
}





Student* print_grade_alphabetically_and_remove_final(Student *students){

   //START WITH FIRST STUDENT AS HIGHEST PRIORITY
   Student *highest_priority_student = students;
   Student *stu_ptr = students->next;
   
    //IF THERE IS ONLY ONE STUDENT THEN JUST PRINT REMOVE AND RETURN
    if(students->next == NULL){
       printf("(%s, %s, %.4g)\n", students->last_name, students->first_name, students->final_grade);	      
       return students = remove_student_from_list(students, students->first_name, students->last_name);
    }


      while(stu_ptr != NULL){
         //IF THERE IS A DIFFERENCE ASSIGN
         if(strcasecmp(stu_ptr->last_name, highest_priority_student->last_name) < 0 ){
            highest_priority_student = stu_ptr;
         
         //IF THEY ARE THE SAME THEN COMPARE THE FIRST NAMES
         }else if(strcasecmp(stu_ptr->last_name, highest_priority_student->last_name) == 0){
            if(strcasecmp(stu_ptr->first_name, highest_priority_student->first_name) < 0 ){
               highest_priority_student = stu_ptr;
            }                                                                                
         }
         stu_ptr = stu_ptr->next; 
      }
   
   printf("(%s, %s, %.4g)\n",highest_priority_student->last_name, highest_priority_student->first_name, highest_priority_student->final_grade);
   return students = remove_student_from_list(students, highest_priority_student->first_name, highest_priority_student->last_name);

}

Student* print_grade_hightolow_and_remove_final(Student *students){

   //START WITH FIRST STUDENT AS HIGHEST PRIORITY
   Student *highest_priority_student = students;
   Student *stu_ptr = students->next;
   
    //IF THERE IS ONLY ONE STUDENT THEN JUST PRINT REMOVE AND RETURN
    if(students->next == NULL){
       printf("(%s, %s, %.4g)\n", students->last_name, students->first_name, students->final_grade);	      
       return students = remove_student_from_list(students, students->first_name, students->last_name);
    }


      while(stu_ptr != NULL){
         //IF THERE IS A DIFFERENCE ASSIGN
         if(stu_ptr->final_grade > highest_priority_student->final_grade){
            highest_priority_student = stu_ptr;
         
         }
      stu_ptr = stu_ptr->next; 
      }
   
   printf("(%s, %s, %.4g)\n",highest_priority_student->last_name, highest_priority_student->first_name,highest_priority_student->final_grade);
   return students = remove_student_from_list(students, highest_priority_student->first_name, highest_priority_student->last_name);
}


















int main(int argc, char *argv[]) {
  int r = 0;
  CmdLineResult R;
  //Gradebook this_gradebook = {};//FILL THIS IN
  
  //FIRST WE PARSE THE INPUT AND VALIDATE, AND STORE THE ARGUMENTS INTO THE "CmdLineResult R" struct.
  R = parse_cmdline(argc, argv);

  if(R.good == 0){
     //printf("R.good from parse_cmdline function call return 0\n");
     printf("Invalid\n");
     return(255);
  }else{
     r=1;
     //printf("WE MADE IT!\n");
     //printf("------------------in main-------------------------\n");
  }
  
  
  //NOW WE MAKE SURE THAT THE GRADEBOOK FILE EXISTS.
  char *FILENAME = R.grade_book_name;
  if(file_test(FILENAME)==0){
     printf("This file does not exist:error 255\n");
     return(255);
  }
  //FILE *fp = fopen(FILENAME, "r");
  
FILE *fp;
  
  
  //NOW I SHOULD RUN THE VRFY algorithim and then DECRYPT upon success, AND THEN LOAD IN THE DATA
  unsigned char *KEY = R.key;

  unsigned char *IV = (unsigned char*)malloc(16*sizeof(unsigned char)+1);
  unsigned char *parity = (unsigned char*)malloc(16*sizeof(unsigned char)+1);
  //for(int i = 0; i < LEN; i++){
  //    unsigned int curr = *(KEY + i);
  //    printf("%.2x", curr);
  //}
  fp = fopen(FILENAME,"rb");
  fseek(fp, 0, SEEK_END);
  long file_len = ftell(fp);
  fclose(fp);
  fp = fopen(FILENAME,"rb");
  unsigned char *plaintext = (unsigned char*)malloc(file_len+1);  
  unsigned char *buffer = (unsigned char *)malloc(file_len+1);
  fread(buffer, sizeof(unsigned char), file_len, fp);  
  
  
  //GET THE PARITY AND IV 
  FILE *fp1 = fopen("temp.txt", "r");
  char *temp_line = "";
  size_t temp_len;
  unsigned char *token;
  while((getline(&temp_line,&temp_len,fp1)) != EOF){
     token = (unsigned char*)strtok(temp_line," ");//GET FIRST PART OF THE LINE
     if(strcmp(token,FILENAME)==0){
        token = strtok(NULL," ");
        parity = token;
        token = strtok(NULL," ");
        IV  = token;
     }  
  }  
  fclose(fp1);

  gcm_decrypt(buffer, file_len, parity, KEY, IV, 1, plaintext);

  fp=freopen(NULL,"w",fp);
  fwrite(plaintext, file_len, 1, fp); 
  
  fclose(fp);
  
  
  
  //NOW LOAD IN DATA IF VRFY WAS GOOD AND DECRYPTION SUCCESFULL
  fp = fopen(FILENAME, "r");
  Gradebook My_gradebook = load_data(FILENAME, fp);
  //printf("%s\n", My_gradebook.assignments->next->assignment_name);  
  
  
  
  //I SHOULD SWITCH THE R.ACTION DO THE OTHER CHECKS I HAVE BEEN KEEPING TRACK OF AND THEN DO THE ACTION -> R.action
  char *AN_VALUE;
  char *FN_VALUE;
  char *LN_VALUE;
  char *ORDER;
  Assignment *ass_ptr = My_gradebook.assignments;
  Assignment *prev_ass_ptr_1 = NULL;
  Assignment *prev_ass_ptr_2 = NULL;
  Student *stu_ptr_4ass = NULL;
 
//FOR THE EXISTENCE LINKED LIST
  Student *stu_ptr = My_gradebook.students;

  
  //NOW SWITCH THE ACTION AND ACTUALLY PRINT THE DATA
  float final_grade_for_student = 0;
  int ass_points;
  float ass_weight;
  int counter = 0;
  int checker_ = 0;  
  switch(R.action){
    
     case print_assignment:
        ORDER = R.order;
        AN_VALUE = R.AN_value;
        
        //FIRST MAKE SURE THE ASSIGNMENTS EXSISTS 
	while(ass_ptr != NULL){
	   if(strcmp(ass_ptr->assignment_name,AN_VALUE)==0){
	      checker_= 1;
	      break;
	   }
	   ass_ptr = ass_ptr->next;
	}
	if(checker_ != 1){
	   printf("Invalid\n");
	   return(255);
	}       
	//NOW MAKE SURE THE STUDENTS LINKED LIST EXISTS FOR THIS ASSIGNMENT
	if(ass_ptr->students == NULL){printf("This assignment has no students\n"); return(255);}
	
	//NOW SEE WHAT ORDER I NEED TO PRINT THE STUDENT GRADES FOR THE ASSIGNMENT
	if(strcmp(ORDER,"-A")==0){
	   
	   //IDEA IS TO FIND THE HIGHEST PRIORITY AND PRINT AND THEN REMOVE AND THEN REPEAT UNTIL STUDENT LIST IS EMPTY
	   while(ass_ptr->students != NULL){
              ass_ptr->students = print_grade_alphabetically_and_remove(ass_ptr->students);
 	   }
 	   
	}else if(strcmp(ORDER,"-G")==0){
	   
	   //IDEA IS TO FIND THE HIGHEST PRIORITY AND PRINT AND THEN REMOVE AND THEN REPEAT UNTIL STUDENT LIST IS EMPTY
	   while(ass_ptr->students != NULL){
              ass_ptr->students = print_grade_hightolow_and_remove(ass_ptr->students);
 	   }
	
	}else{
	   printf("Invalid\n");
	   return(255);
	}

     
     break;
     
     
     case print_student:
        FN_VALUE = R.FN_value;
        LN_VALUE = R.LN_value;
        
        //IF THE STUDENT EXSISTENCE LINKED LIST IS EMPTY THEN ERROR
        if(stu_ptr == NULL){printf("Invalid\n"); return(255);}//WE ACTUALLY DONT NEED THIS LINE, THE NEXT LOOP AND CHECK WILL TAKE CARE
        
        //MAKE SURE THE STUDENT IS IN THE EXSISTENCE LIST
        while(stu_ptr != NULL){
           if(strcmp(stu_ptr->first_name, FN_VALUE)==0 && strcmp(stu_ptr->last_name, LN_VALUE)==0){
              checker_ = 1;
           }
           stu_ptr = stu_ptr->next;
        }
        if(checker_ != 1){
           printf("student does not exsist in gradebook\n");
           printf("Invalid\n");
           return(255);
        }
       
	//MAKE SURE THAT THERE ARE ASSIGNMENTS IN THE GRADEBOOK
        if(ass_ptr == NULL){
           printf("Student is in gradebook but has no grade for any assignment\n");
           break;
        }
        //NOW WE LOOP THROUGH THE ASSIGNMENTS AND THEN THE STUDENTS,IF STUDENT BELONGS TO THAT ASSIGNMENT THEN PRINT.        
        while(ass_ptr != NULL){
           stu_ptr = ass_ptr->students;
           while(stu_ptr != NULL){
              if(strcmp(stu_ptr->first_name,FN_VALUE)==0 && strcmp(stu_ptr->last_name,LN_VALUE)==0){
                 printf("(%s, %d)\n", ass_ptr->assignment_name, stu_ptr->grade);                 
                 counter++;
              }
              stu_ptr = stu_ptr->next;
           }
           ass_ptr = ass_ptr->next;
        }
       
        if(counter == 0){
           printf("Student is in gradebook but has no grade for any assignment\n");            
        }
        
     break;
     
     
     case print_final: 
        //int counter = My_gradebook.num_of_students;
        ORDER = R.order;
        //IF THERE ARE NO STUDENTS THEN RETURN
        if(stu_ptr == NULL){printf("No students in the gradebook\n"); return(255);}
        //if(ass_ptr == NULL){printf("No assignments in the gradebook\n"); return(255);}        
        //I SHOULD FIND THE FINAL GRADE FOR EACH STUDENT AND UTILIZE THE EXISISTENCE LINKED LIST TO STORE AND THEN USE THE HELPER PRINT
        
        
        while(stu_ptr != NULL){
           final_grade_for_student = 0;
           while(ass_ptr != NULL){
              ass_points = ass_ptr->points;//this is an int
              ass_weight = ass_ptr->weight;//this is a float
              stu_ptr_4ass = ass_ptr->students;
              while(stu_ptr_4ass != NULL){//WHILE THERE ARE STUDENT FOR AN ASSIGNMENT 
                 if(strcmp(stu_ptr->first_name, stu_ptr_4ass->first_name)==0 && strcmp(stu_ptr->last_name,stu_ptr_4ass->last_name)==0){
                    final_grade_for_student += ((float)(stu_ptr_4ass->grade)/(float)(ass_points)) * ass_weight;
                      break;//if found then go to next assignment
                 }
                 
                 stu_ptr_4ass = stu_ptr_4ass->next;
              }
              
              ass_ptr = ass_ptr->next;
           }
           stu_ptr->final_grade = final_grade_for_student; //Set the grade for a student, add 1-sum_of_weights to account for 
           						    //sum_of_weights not being 1
           ass_ptr = My_gradebook.assignments;//reset the assignment pointer
           stu_ptr = stu_ptr->next;
        }
        
        
	//NOW SEE WHAT ORDER I NEED TO PRINT THE STUDENT FINAL GRADES FOR THE ASSIGNMENT
	if(strcmp(ORDER,"-A")==0){
	   
	   //IDEA IS TO FIND THE HIGHEST PRIORITY AND PRINT AND THEN REMOVE AND THEN REPEAT UNTIL STUDENT LIST IS EMPTY
	   while(My_gradebook.students != NULL){
              My_gradebook.students = print_grade_alphabetically_and_remove_final(My_gradebook.students);
 	   }
 	   
	}else if(strcmp(ORDER,"-G")==0){
	   
	   //IDEA IS TO FIND THE HIGHEST PRIORITY AND PRINT AND THEN REMOVE AND THEN REPEAT UNTIL STUDENT LIST IS EMPTY
	   while(My_gradebook.students != NULL){
              My_gradebook.students = print_grade_hightolow_and_remove_final(My_gradebook.students);
 	   }
	
	}else{
	   printf("Invalid\n");
	   return(255);
	}

     break;
     
     
     case nonce:
        printf("Invalid\n");
        return(255);
     break;      
     
     default:
        printf("Invalid\n");
        return(255);
     break;
  }//END OF SWITCH

  fclose(fp);

//NOW ENCRYPT THE FILE
  
  fp = fopen(FILENAME,"rb");
  fseek(fp, 0, SEEK_END);
  file_len = ftell(fp);
  fclose(fp);
  fp = fopen(FILENAME,"rb");
  unsigned char *new_buffer = (unsigned char *)malloc(file_len+1);
  fread(new_buffer, sizeof(unsigned char), file_len, fp);
  unsigned char *cipher_text = (unsigned char*)malloc(file_len+1);
    
  gcm_encrypt(new_buffer, file_len, KEY, IV, 1, cipher_text, parity);
  
  fp=freopen(NULL,"w",fp);
  fwrite(cipher_text, file_len, 1, fp);



  fclose(fp);







  return r;
}	   
