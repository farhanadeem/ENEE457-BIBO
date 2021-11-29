#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

//include "data.h"

void handleErrors(void)
{
    //ERR_print_errors_fp(stderr);
    printf("Error. The key is incorrect\n");
    exit(255);
    //abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext, unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
        handleErrors();
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
        handleErrors();
    if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors();     
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
        handleErrors();
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}



int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext, unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
        handleErrors();
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
        handleErrors();
    if(1 != EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors();  
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
        handleErrors();
    int ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    if(ret>0){
    	return plaintext_len;
    }else{
    	return -1;
    }
}


/* *********   gradebook stuff*/
typedef struct _Assignment Assignment;
typedef struct _Student Student;
typedef struct _Student_final Stufinal;
typedef struct _Gradebook {
  char name[100];
  Assignment *assignments;
  Student *stu;
  int num_as;
  int num_stu;
} Gradebook;
typedef struct _Assignment {
  char name[100];
  int point;
  float weight;
  Assignment *next_as;
  Student *stu;
} Assignment;

typedef struct _Student {
  char fn[100];
  char ln[100];
  int grade;
  Student *next_stu;
} Student;

typedef struct _Student_final {
  char fn[100];
  char ln[100];
  float grade;
  Stufinal *next_stu;
} Stufinal;

/*IO functions*/
void store_student(FILE *fp, Student *s){
	if(s==NULL){
		printf("store student s is null\n");
		return;
	}
	fseek(fp,0,SEEK_END);
	Student *holdnext = s->next_stu;
	s->next_stu = NULL;
	fwrite(s,sizeof(Student),1,fp);
	//printf("in store student:%s\n",s->fn);
	store_student(fp,holdnext);
}

void store_assignment(FILE *fp, Assignment *a){
	if(a==NULL){
		return;
	}
	fseek(fp,0,SEEK_END);
	Assignment *holdnext = a->next_as;
	Student *holdstu = a->stu;
	a->next_as=NULL;
	a->stu=NULL;
	fwrite(a,sizeof(Assignment),1,fp);
	//printf("in store assignment:%s\n",a->name);
	store_student(fp,holdstu);
	store_assignment(fp,holdnext);
}

void store_gradebook(Gradebook *g){

	Assignment *holdas = g->assignments;
	Student *holdstu = g->stu;
	g->assignments = NULL;
	g->stu = NULL;
	FILE *fp;
	fp=fopen(g->name,"wb");
	fwrite(g,sizeof(Gradebook),1,fp);
	//printf("in store gradebook: %s\n",holdstu->fn);
	store_student(fp, holdstu);
	fseek(fp,0,SEEK_END);
	store_assignment(fp, holdas);
	fclose(fp);
}

Student* read_student(FILE *fp,int num){
	Student *start=NULL;
	Student *prev =NULL;
	for(int i=0;i<num;i++){
		Student *s = malloc(sizeof(Student));
		fread(s,sizeof(Student),1,fp);
		//printf("in read %d student:%s \n",i,s->fn);
		if(i==0){
			start = s;
			prev = s;
			
		}else{
			prev->next_stu=s;
			prev =prev->next_stu;
		}
	}
	return start;
}
Assignment* read_assignment(FILE* fp, int num_stu){
	Assignment *a=malloc(sizeof(Assignment));
	fread(a,sizeof(Assignment),1,fp);
	a->stu = read_student(fp, num_stu);
	//printf("assignment read done: %s\n", a->name);
	return a;
}

Gradebook* read_gradebook(char *name){
	Gradebook *g = malloc(sizeof(Gradebook));
	FILE *fp;
	fp=fopen(name,"rb");
	if(fp==NULL){
		printf("invalid gradebook name");
		exit(255);
	}
	fread(g,sizeof(Gradebook),1,fp);
	fseek(fp,sizeof(Gradebook),SEEK_SET);
	g->stu=read_student(fp,g->num_stu);
	long int offset = sizeof(Gradebook)+sizeof(Student)*g->num_stu;
	fseek(fp,offset,SEEK_SET);
	if(g->num_as>0){
		//printf("******Starting reading Assignment*******\n" );
		g->assignments=read_assignment(fp,g->num_stu);
		//printf("The first assignment is %s\n", g->assignments->name);
		Assignment *prev = g->assignments;
		offset +=sizeof(Assignment)+sizeof(Student)*g->num_stu;
		fseek(fp,offset,SEEK_SET);
		//printf("in read gradebook %s", prev->next_as->name);
		for(int i = 1;i<g->num_as;i++){
			prev->next_as = read_assignment(fp,g->num_stu);
			offset +=sizeof(Assignment)+sizeof(Student)*g->num_stu;
			fseek(fp,offset,SEEK_SET);
			//printf("The %d assignment is %s\n", i+1,prev->next_as->name);
			prev=prev->next_as;
		}
	}
	fclose(fp);	
	return g;
}

/********************************/
int checkString(char str1[]) {
    int i, x=0, p;
    p=strlen(str1);

    for (i = 0; i < p ; i++)
        if (isalnum(str1[i]))
            continue;
        else return 0;

    return 1;
}
void print_Gradebook(Gradebook *g) {
  printf("***%s***\n",g->name);
  Assignment *i = g->assignments;
  while(i!=NULL){
    printf("######%s#######\n",i->name);
    Student *j = i->stu;
    while(j!=NULL){
      printf("(%s, %s, %d)\n",j->fn,j->ln,j->grade);
      j=j->next_stu;
    }
    i=i->next_as;
  }
  printf("#############\n");

  return;
}


void print_Assignment_G(Assignment *assignment) {
	Student *s= assignment->stu;
	Student *start = NULL;
	
	while(s!=NULL){
		Student *i=malloc(sizeof(Student));
		strcpy(i->fn,s->fn);
		strcpy(i->ln,s->ln);
		i->grade=s->grade;
		i->next_stu =NULL;
		//check if no student
		if(start==NULL){
			start = i;
			s=s->next_stu;
			continue;
		}
		
		Student *j=start;
		//check if insert first
		if(start->grade<=i->grade){
			i->next_stu=start;
			start=i;
			s=s->next_stu;
			continue;
		}else{
			
			Student *prev = j;
			while(j!=NULL && j->grade>i->grade){
				prev = j;
				j=j->next_stu;
			}
			i->next_stu=j;
			prev->next_stu=i;
			
		}
		
		s=s->next_stu;
	}
	Student *j = start;
    while(j!=NULL){
    	if(j->grade != -1){
      		printf("(%s, %s, %d)\n",j->ln,j->fn,j->grade);
      	}
      j=j->next_stu;
    }
}

void print_Assignment_A(Assignment *assignment) {
	if(assignment == NULL){
		printf("invalid assignment NULL.\n");
		return;
	}
	Student *s= assignment->stu;
	Student *start = NULL;
	//printf("reach print assignment\n");
	while(s!=NULL){
		Student *i=malloc(sizeof(Student));
		strcpy(i->fn,s->fn);
		strcpy(i->ln,s->ln);
		i->grade=s->grade;
		i->next_stu =NULL;
		//check if no student
		if(start==NULL){
			start = i;
			s=s->next_stu;
			continue;
		}
		Student *j=start;
		//check if insert first
		
		if(strcasecmp(start->ln,i->ln)>0 ||(strcasecmp(start->ln,i->ln)==0 && strcasecmp(start->fn,i->fn)>0 )){
			i->next_stu=start;
			start=i;
			s=s->next_stu;
			continue;
		}else if(strcasecmp(start->fn,i->fn)==0 && strcasecmp(start->ln,i->ln)==0){
			i->next_stu=start;
			start=i;
			s=s->next_stu;
			continue;	
		}else{
			Student *prev = j;
			while(j!=NULL && (strcasecmp(j->ln,i->ln)<0 ||(strcasecmp(j->ln,i->ln)==0 && strcasecmp(j->fn,i->fn)<0 ))){
				prev = j;
				j=j->next_stu;
			}
			i->next_stu=j;
			prev->next_stu=i;
			
		}
		
		s=s->next_stu;
	}
	Student *j = start;
    while(j!=NULL){
    	if(j->grade != -1){
      		printf("(%s, %s, %d)\n",j->ln,j->fn,j->grade);
      	}
      j=j->next_stu;
    }
    

  
	
}

void print_Student(Gradebook *g, Student *s) {
	Assignment *a=g->assignments;
	while(a!=NULL){
		Student *i = a->stu;
		while(i!=NULL){
			if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0){
				if(i->grade != -1){
					printf("(%s, %d)\n",a->name, i->grade);
				}
				break;
			}
			i=i->next_stu;
		}
		if(i==NULL){
			printf("no such student in print student\n");
			return;
		}
		a=a->next_as;		
	}
	Student *i = g->stu;
		while(i!=NULL){
			if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0){
				break;
			}
			i=i->next_stu;
		}
		if(i==NULL){
			printf("no such student\n");
			return;
		}
	
}

void print_Final_A(Gradebook *g){
	Student *s = g->stu;
	Stufinal *start = NULL;
	while(s!=NULL){
		float final = 0;
		Assignment *a=g->assignments;
		//find a student's final grade
		while(a!=NULL){
			Student *i = a->stu;
			while(i!=NULL){
				if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0){
					if(i->grade==-1){
						final = final;
					}else{
						final = final + (a->weight)*(float)(i->grade)/(float)(a->point);
					}
					//printf("%s: %d %f\n",a->name,i->grade,final);
					break;
				}
				i=i->next_stu;
			}
			if(i==NULL){
				printf("Student lists inconsistant. Data may have been compromised.\n");
				return;
			}
			a=a->next_as;		
		}
		//create new intance
		Stufinal *s_final=malloc(sizeof(Stufinal));
		strcpy(s_final->fn,s->fn);
		strcpy(s_final->ln,s->ln);
		s_final->grade=final;
		s_final->next_stu=NULL;
		//insert into final list in name order
			//check if no student
		if(start==NULL){
			start = s_final;
			s=s->next_stu;
			continue;
		}
		
		Stufinal *j=start;
			//check if insert first
		if(strcasecmp(start->ln,s_final->ln)>0 ||(strcasecmp(start->ln,s_final->ln)==0 && strcasecmp(start->fn,s_final->fn)>0 )){
			//printf("insert as the first:%s",s_final->ln);
			s_final->next_stu=start;
			start=s_final;
			s=s->next_stu;
			continue;
		}else if(strcasecmp(start->fn,s_final->fn)==0 && strcasecmp(start->ln,s_final->ln)==0){
			s_final->next_stu=start;
			start=s_final;
			s=s->next_stu;
			continue;
		}else{			
			Stufinal *prev = start;
			while(j!=NULL && (strcasecmp(j->ln,s_final->ln)<0 ||(strcasecmp(j->ln,s_final->ln)==0 && strcasecmp(j->fn,s_final->fn)<0 ))){
				prev = j;
				j=j->next_stu;
			}
			s_final->next_stu=j;
			prev->next_stu=s_final;			
		}
		s=s->next_stu;
	}
	Stufinal *j = start;
    while(j!=NULL){
      printf("(%s, %s, %g)\n",j->ln,j->fn,j->grade);
      j=j->next_stu;
    }

}

void print_Final_G(Gradebook *g){
	Student *s = g->stu;
	Stufinal *start = NULL;
	while(s!=NULL){
		float final = 0;
		Assignment *a=g->assignments;
		//find a student's final grade
		while(a!=NULL){
			Student *i = a->stu;
			while(i!=NULL){
				if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0){
					if(i->grade==-1){
						final = final;
					}else{
						final = final + (a->weight)*(float)(i->grade)/(float)(a->point);
					}
					//printf("%s: %f %f\n",a->name,a->weight,final);
					break;
				}
				i=i->next_stu;
			}
			if(i==NULL){
				printf("Student lists inconsistant. Data may have been compromised. Can't find %s %s\n",s->fn,s->ln);
				return;
			}
			a=a->next_as;		
		}
		//create new intance
		Stufinal *s_final=malloc(sizeof(Stufinal));
		strcpy(s_final->fn,s->fn);
		strcpy(s_final->ln,s->ln);
		s_final->grade=final;	
		s_final->next_stu = NULL;
		//insert into final list in name order
			//check if no student
		if(start==NULL){
			start = s_final;
			s=s->next_stu;
			continue;
		}
		
		Stufinal *j=start;
			//check if insert first
			//printf("%d, %f, %f",start->grade<=s->grade,start->grade,s->grade);
		if(start->grade<=s_final->grade){
			s_final->next_stu=start;
			start=s_final;
			s=s->next_stu;
			continue;
		}else{			
			Stufinal *prev = j;
			while(j!=NULL && j->grade>s_final->grade){
				prev = j;
				j=j->next_stu;
			}
			s_final->next_stu=j;
			prev->next_stu=s_final;			
		}
		s=s->next_stu;
	}
	Stufinal *j = start;
    while(j!=NULL){
      printf("(%s, %s, %g)\n",j->ln,j->fn,j->grade);
      j=j->next_stu;
    }

}

void check_flag(char** args,int len){
	for(int i = 0; i <len; i++){
		if(args[i][0]=='-' && strcmp(args[i],"-N")!=0 && strcmp(args[i],"-K")!=0&& strcmp(args[i],"-PA")!=0&& strcmp(args[i],"-PS")!=0&& strcmp(args[i],"-AN")!=0&& strcmp(args[i],"-FN")!=0&& strcmp(args[i],"-LN")!=0&& strcmp(args[i],"-A")!=0&& strcmp(args[i],"-G")!=0&& strcmp(args[i],"-PF")!=0){
				printf("Unidentified flag.\n");
			       exit(255);
		}
	}
}

char* find_flag(char** args,int len, char* flag) {
    int value = -1;
    for(int i = 0; i <len; i++) {
        if(strcmp(args[i],flag)==0) {
            if(i+1 >= len || args[i+1][0]=='-'){
               printf("Missing arguments.\n");
               return NULL;
            }
            value = i+1;
        }
    }
    if(value == -1){
    	printf("Can't find the flag %s.\n",flag);
        return NULL;
    }
    if(args[value+1]!=NULL && args[value+1][0]!='-'){
        	printf("Unidentified arguments: %s.\n",args[value+1]);
        return NULL;
    }
    return args[value];
}
int exist_flag(char** args,int len, char* flag){
	for(int i = 0; i <len; i++){
		if(strcmp(args[i],flag)==0){
			if(args[i+1]!=NULL && args[i+1][0]!='-'){
				printf("Unidentified arguments.\n");
			       return -1;
			}
			return 1;
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
   check_flag(argv,argc);
  //gcc -o gradebookdisplay gradebookdisplay.c
  if(argc==1) 
      printf("\nNo Extra Command Line Argument Passed Other Than Program Name"); 
  if(argc>=2) 
  { 
      //read in key
	unsigned char key[16];
	FILE *fp;
    if(argc>=2) 
    { 
        if(strcmp(argv[1],"-N")!=0 ||strcmp(argv[3],"-K")!=0){
        	printf("missing name or key\n");
        	exit(255);
        }
    }
    int j = 0;
    if(strlen(argv[4])!=32){
    	printf("invalid key length: %ld\n",strlen(argv[4]));
    	exit(255);
    }
    for(int i=0;i<strlen(argv[4]);i=i+2){
       char byte[2];
       byte[0]=argv[4][i];
       byte[1]=argv[4][i+1];
       key[j] = (int)strtol(byte,NULL,16);
       //printf("%x ",key[j]);
       j++;
    }
    
//find cipherfile size
  fp = fopen(argv[2], "rb");
  if (fp == NULL){
    printf("setup: fopen() error could not create file\n");
    printf("invalid\n");
    return(255);
  }
  fseek(fp,0,SEEK_END);
  long fileSize =ftell(fp);
  rewind(fp);
  long int ciphertext_len = fileSize-32;
//read in ciphertext
  unsigned char cipher[fileSize];
  unsigned char iv[16];
  unsigned char tag[16];
  fp = fopen(argv[2], "rb");
  fread(iv, 16,1,fp);
  fread(tag, 16,1,fp);
  fread(cipher, ciphertext_len,1,fp);
  //printf("%ld %ld",fileSize,ciphertext_len);
  //BIO_dump_fp (stdout, (const char *)iv, 16);
  //BIO_dump_fp (stdout, (const char *)new_iv, 16);
  unsigned char plaintext[fileSize];
  long int len = decrypt(cipher, ciphertext_len, key, iv, plaintext,tag);
  if(len==-1){
  	printf("The gradebook has been compromised or the key is incorrect.\n");
  	fclose(fp);
  	exit(255);
  }
  //printf("%ld",len);
  fclose(fp);
//write plain gradebook
  fp = fopen(argv[2], "wb");
  fwrite(plaintext, fileSize,1,fp);
  fclose(fp);
	
   
   
   
   
   
/*********************display operation*******************/
	Gradebook *g = read_gradebook(argv[2]);
    if(strcmp(argv[2],g->name)==0){
 
    
      //print assignment
      if (strcmp(argv[5],"-PA")==0 && argv[6][0]=='-'){
		  
			char* asg_name = find_flag(argv,argc, "-AN");
			int A_flag = exist_flag(argv,argc, "-A");
			int G_flag = exist_flag(argv,argc, "-G");	
			Assignment *as = g->assignments;			
			while(as!=NULL && asg_name!=NULL &&checkString(asg_name)){
				if(strcmp(as->name,asg_name)==0){
					//printf("check %s,%s, %d\n",as->name,asg_name,strcmp(as->name,asg_name)==0);
					break;
				}
				as=as->next_as;
			}
			
			
			if(as==NULL){
				printf("no such assignment in print assignment\n");
			}else{	
				if(A_flag==1 && (G_flag == 0)){
					print_Assignment_A(as);
				}else if(G_flag==1 && (A_flag == 0)){
					print_Assignment_G(as);
				}else{
					printf("invalid print assignment\n");
				}
			}
		//print student
		}else if (strcmp(argv[5],"-PS")==0 && argv[6][0]=='-'){
			char* fn = find_flag(argv,argc, "-FN");
			char* ln = find_flag(argv, argc,"-LN");
        	if (fn==NULL || ln ==NULL || !checkString(fn) || !checkString(ln)){
				printf("invalid print student");
			}else{
		    	Student s={"whatever","whatever",0,NULL};
		    	if(strlen(fn)>100 || strlen(ln)>100){
					printf("Buffer overflow warning! Exiting.....\n");
				}else{
					strcpy(s.fn,fn);
					strcpy(s.ln,ln);
					print_Student(g,&s);
		    	}
        	}
		//print final
		}else if (strcmp(argv[5],"-PF")==0 && argv[6][0]=='-'){
			int A_flag = exist_flag(argv,argc, "-A");
			int G_flag = exist_flag(argv,argc, "-G");	
			if(A_flag==1 && (G_flag == 0)){
				print_Final_A(g);
			}else if(G_flag==1 && (A_flag == 0)){
				print_Final_G(g);
			}else{
				printf("invalid print final\n");
			}		
		}else{
		printf("Command line format error.\n");
    	}
}else{
   	printf("Gradebook name doesn't match.\n");
}
      
   
//encryption operation
	//find new iv
	unsigned char *new_iv = (unsigned char*) malloc(sizeof(unsigned char)*16);
	unsigned char *new_tag = (unsigned char*) malloc(sizeof(unsigned char)*16);
  FILE *random = fopen("/dev/urandom", "r");
  fread(new_iv, sizeof(unsigned char)*16, 1, random);
  fclose(random);
  // find file sie
  fp = fopen(argv[2], "rb");
  if (fp == NULL){
    printf("setup: fopen() error could not create file\n");
    printf("invalid\n");
    return(255);
  }
  fseek(fp,0,SEEK_END);
  fileSize =ftell(fp);
  rewind(fp);
  //encrpytion
  unsigned char new_plain[fileSize];
  unsigned char new_ciphertext[fileSize];
  //read plaintext
  fread(new_plain, fileSize,1,fp);
  fclose(fp);
  long int new_ciphertext_len = encrypt (new_plain, fileSize, key, new_iv,new_ciphertext,new_tag);
 // write cipherfile
  fp = fopen(argv[2], "wb");
   fwrite(new_iv, 16,1,fp);
   fwrite(new_tag, 16,1,fp);
  fwrite(new_ciphertext, new_ciphertext_len,1,fp);
  
  } 
  

}
