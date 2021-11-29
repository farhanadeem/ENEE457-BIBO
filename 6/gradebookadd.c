#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <ctype.h>
#include <math.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

//#include "data.h"
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


/******gradebook stuff******/
typedef struct _Assignment Assignment;
typedef struct _Student Student;
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

/*IO functions*/
void store_student(FILE *fp, Student *s) {
    if(s==NULL) {
        //printf("store student s is null\n");
        return;
    }
    fseek(fp,0,SEEK_END);
    Student *holdnext = s->next_stu;
    s->next_stu = NULL;
    fwrite(s,sizeof(Student),1,fp);
    //printf("in store student:%s %s\n",s->fn,s->ln);
    store_student(fp,holdnext);
}

void store_assignment(FILE *fp, Assignment *a) {
    if(a==NULL) {
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
    fseek(fp,0,SEEK_END);
    store_assignment(fp,holdnext);
}

void store_gradebook(Gradebook *g) {

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

Student* read_student(FILE *fp,int num) {
    Student *start=NULL;
    Student *prev =NULL;
    for(int i=0; i<num; i++) {
        Student *s = malloc(sizeof(Student));
        fread(s,sizeof(Student),1,fp);
        //printf("in read %d student:%s %s\n",i,s->fn,s->ln);
        if(i==0) {
            start = s;
            prev = s;

        } else {
            prev->next_stu=s;
            prev =prev->next_stu;
        }
    }
    return start;
}
Assignment* read_assignment(FILE* fp, int num_stu) {
    Assignment *a=malloc(sizeof(Assignment));
    fread(a,sizeof(Assignment),1,fp);
    a->stu = read_student(fp, num_stu);
    //printf("assignment read done: %s\n", a->name);
    return a;
}

Gradebook* read_gradebook(char *name) {
    Gradebook *g = malloc(sizeof(Gradebook));
    FILE *fp;
    fp=fopen(name,"rb");
    if(fp==NULL) {
        printf("invalid gradebook name");
        exit(255);
    }
    fread(g,sizeof(Gradebook),1,fp);
    //printf("%s\n",g->name);
    fseek(fp,sizeof(Gradebook),SEEK_SET);
    g->stu=read_student(fp,g->num_stu);
    long int offset = sizeof(Gradebook)+sizeof(Student)*g->num_stu;
    fseek(fp,offset,SEEK_SET);
    if(g->num_as>0) {
        //printf("******Starting reading Assignment*******\n" );
        g->assignments=read_assignment(fp,g->num_stu);
        //printf("The first assignment is %s\n", g->assignments->name);
        Assignment *prev = g->assignments;
        offset +=sizeof(Assignment)+sizeof(Student)*g->num_stu;
        fseek(fp,offset,SEEK_SET);
        //printf("in read gradebook %s", prev->next_as->name);
        for(int i = 1; i<g->num_as; i++) {
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



/***gradebook add functions*************************/
int checkString(char str1[]) {
    int i, x=0, p;
    p=strlen(str1);

    for (i = 0; i < p ; i++)
        if (isalnum(str1[i]))
            continue;
        else return 0;

    return 1;
}
int add_assignment(Gradebook *g, Assignment *a) {
    if(g->num_as==0 && a->weight <= 1) {
        //printf("reach add assignment numas===0\n");
        g->assignments = a;
        g->num_as++;
    } else {
        Assignment *i=g->assignments;
        float total_w=0;
        //printf("%s\n",i->next_as->name);
        while(i->next_as!=NULL) {
            total_w+=i->weight;
            if(strcmp(i->name,a->name)==0){
              printf("Error. The assignment's name already exists\n");
              return -1;
            }
            i = i->next_as;
        }
        //printf("in add_assignment: %s\n",i->name);
        total_w+=i->weight;
        if(strcmp(i->name,a->name)==0){
              printf("Error. The assignment's name already exists\n");
              return -1;
        }
        total_w+=a->weight;
        if(total_w>1){
          printf("Error. Sum of weights is more than 1: %g \n",total_w);
          return -1;
        }
        i->next_as = a;
        g->num_as++;
    }
    //printf("%s\n",g->assignments->next_as->name);
    if(g->num_stu>0) {
        Student *i = g->stu;
        Student *start;
        Student *prev;
        for(int j = 0; j<g->num_stu; j++) {
            Student *stu = malloc(sizeof(Student));
            strcpy(stu->fn,i->fn);
            strcpy(stu->ln,i->ln);
            stu->grade = -1;
            if(j==0) {
                start = stu;
                prev = stu;
            } else {
                prev->next_stu = stu;
                prev = prev->next_stu;
            }
            i=i->next_stu;
            //printf("in add assignment :%s\n",stu->fn);
        }
        a->stu = start;
    }

    return 0;
}

int add_student(Gradebook *g, Student *s) {
    //printf("adding student, data is :%s %s\n",s->fn,s->ln);
    if(g->num_stu==0) {
        g->stu = s;
        Assignment *as = g->assignments;
        g->num_stu++;
        while(as!=NULL) {
            Student *stu = malloc(sizeof(Student));
            strcpy(stu->fn,s->fn);
            strcpy(stu->ln,s->ln);
            stu->grade = -1;
            stu->next_stu=NULL;
            as->stu=stu;
            as = as->next_as;
        }
    } else {
        // add to gradebook
        Student *i=g->stu;
        while(i->next_stu!=NULL) {
            if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0) {
                printf("student already exists: %s %s",s->fn,s->ln);
                return -1;
            }
            i = i->next_stu;
        }
        if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0) {
            printf("student already exists: %s %s",s->fn,s->ln);
            return -1;
        }
        i->next_stu = s;
        g->num_stu++;
        // add to assignment
        Assignment *as = g->assignments;
        while(as!=NULL) {
            Student *stu = malloc(sizeof(Student));
            strcpy(stu->fn,s->fn);
            strcpy(stu->ln,s->ln);
            stu->grade = -1;
            stu->next_stu=NULL;
            Student *i = as->stu;
            while(i->next_stu!=NULL) {
                if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0) {
                    printf("student already exists: %s %s",s->fn,s->ln);
                    return -1;
                }
                i=i->next_stu;
            }
            if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0) {
                printf("student already exists: %s %s",s->fn,s->ln);
                return -1;
            }
            i->next_stu = s;
            as = as->next_as;
        }
    }
    return 0;
}
int student_deleter(Student *stu,Student *s) {
    Student *i=stu;
    Student *prev =stu;
    while(i->next_stu!=NULL) {
        if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0) {
            prev->next_stu = i->next_stu;
            return 1;
        }
        prev=i;
        i = i->next_stu;
    }
    if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0) {
        prev->next_stu=i->next_stu;
        return 1;
    } else {
        printf("no such student to be deleted: %s %s\n",s->fn,s->ln);
        return 0;
    }
}
int delete_student(Gradebook *g, Student *s) {
    //printf("deleting student, data is: %s, %s\n",s->fn,s->ln);
    if(g->num_stu==0) {
        printf("no student rn\n");
        return -1;
    } else {
        if(strcmp(s->fn,g->stu->fn)==0 && strcmp(s->ln,g->stu->ln)==0) {
            Student *holdstu = g->stu;
            g->stu = g->stu->next_stu;
            g->num_stu--;
            //free(holdstu);
        } else {
            if(student_deleter(g->stu,s)){
            	  g->num_stu--;
            }
        }
        Assignment *as = g->assignments;
        while(as!=NULL) {
        	//printf("deleting student in %s.\n",as->name);
            //check if the first
            if(strcmp(s->fn,as->stu->fn)==0 && strcmp(s->ln,as->stu->ln)==0) {
                Student *holdstu = as->stu;
                as->stu = as->stu->next_stu;
                //free(holdstu);
            } else {
                if(student_deleter(g->stu,s)){
            	  g->num_stu--;
            	}
            }
            as=as->next_as;
        }

    }
    return 0;
}

int delete_assignment(Gradebook *g, Assignment *a) {
    if(g->num_as==0) {
        printf("no assignment stored.\n");
        return -1;
    } else {
        //printf("deleteing assignment, data is: %s\n",a->name);
        //if it's the first
        if(strcmp(g->assignments->name,a->name)==0) {
            Assignment *holdas=g->assignments;
            g->assignments=g->assignments->next_as;
            //free(holdas->stu);
            //free(holdas);
            g->num_as--;
            return 0;
        }
        Assignment *i=g->assignments;
        Assignment *prev = g->assignments;
        while(i->next_as!=NULL) {
                   // printf("asg_name: %s\n",i->name);
            if(strcmp(a->name,i->name)==0) {
                prev->next_as = i->next_as;
                g->num_as--;
                //free(i->stu);
                //free(i);
                return 0;
            }
            prev=i;
            i = i->next_as;
        }
        if(strcmp(a->name,i->name)==0) {
            prev->next_as=i->next_as;
            //free(i->stu);
            //free(i);
            g->num_as--;
            return 0;
        } else {
            printf("no such assignment\n");
            return -1;
        }
    }
    return 0;
}

int add_grade(Gradebook *g, char *as_name, Student* s, int grade) {
    Assignment *as=g->assignments;
    while(as!=NULL&&strcmp(as_name,as->name)!=0) {
        as=as->next_as;
    }
    if(as==NULL) {
        printf("no such assignment");
        return -1;
    }
    Student *i=as->stu;
    while(i!=NULL) {
        if(strcmp(s->fn,i->fn)==0 && strcmp(s->ln,i->ln)==0) {
            break;
        }
        i=i->next_stu;
    }
    //printf("Adding grade, data is: %s , %s , %d\n",as->name,i->fn,grade);
    if(i==NULL) {
        printf("no such student: %s %s\n",s->fn,s->ln);
        return -1;
    } else {
    	if(grade > as->point){
    		printf("Grade is greater than max possible points.\n");
    	}else{
        	i->grade=grade;
        }
    }
    return 0;


}

void check_flag(char** args,int len){
	for(int i = 0; i <len; i++){
		if(args[i][0]=='-' && strcmp(args[i],"-N")!=0 && strcmp(args[i],"-K")!=0&& strcmp(args[i],"-AA")!=0&& strcmp(args[i],"-DA")!=0&& strcmp(args[i],"-AS")!=0&& strcmp(args[i],"-DS")!=0&& strcmp(args[i],"-AG")!=0&& strcmp(args[i],"-AN")!=0&& strcmp(args[i],"-FN")!=0&& strcmp(args[i],"-LN")!=0&& strcmp(args[i],"-P")!=0&& strcmp(args[i],"-W")!=0&& strcmp(args[i],"-G")!=0){
				printf("Unidentified flag or negative numbers.\n");
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

void parse_cmdline(int argc, char *argv[]) {

    if(argc==1)
        printf("\nNo Extra Command Line Argument Passed Other Than Program Name");

    Gradebook *test = read_gradebook(argv[2]);
    if(strcmp(argv[2],test->name)!=0){
    	printf("Gradebook name doesn't match.\n");
    	return;
    }
    //add assignment
    if (strcmp(argv[5],"-AA")==0 && argv[6][0]=='-') {

        char* asg_name = find_flag(argv,argc, "-AN");
        char* point_s = find_flag(argv, argc,"-P");
        char* weight_s = find_flag(argv,argc, "-W");
		if(asg_name!=NULL&&point_s!=NULL && weight_s !=NULL && strlen(point_s)<6 && strlen(weight_s) < 6 && checkString(asg_name)){
			if(roundf(atof(point_s))==atof(point_s)){
				int points = atoi(point_s);
				float weight = atof(weight_s);
				if(points>=0 && weight >=0 && weight <=1) {
				    Assignment as= {"whatever",points,weight, NULL,NULL};
				    strcpy(as.name,asg_name);
				    as.point=points;
				    //printf("in add assignment %s\n",as.name);
				    add_assignment(test,&as);
				    //printf("after adding assignment:%s\n",test->assignments->name);
				} else {
				    printf("invalid format\n");
				}
			}else{
		    	printf("Point has to be a integer.\n");
		    }
		}else{
			printf("Invalid arguments.\n");
		}

        //delete assignment
    } else if (strcmp(argv[5],"-DA")==0 && argv[6][0]=='-') {
        char* asg_name = find_flag(argv, argc,"-AN");
        if (asg_name == NULL || !checkString(asg_name)) {
            printf("invalid delete assignment\n");
        }else{
		    Assignment a;
		    strcpy(a.name,asg_name);
		    delete_assignment(test,&a);
        }
        //add student
    } else if (strcmp(argv[5],"-AS")==0 && argv[6][0]=='-') {
        char* fn = find_flag(argv,argc, "-FN");
        char* ln = find_flag(argv, argc,"-LN");
        if (fn==NULL || ln ==NULL || !checkString(fn) || !checkString(ln)) {
            printf("invalid student name\n");
        }else{
		    Student s= {"","",0,NULL};
		    if(strlen(fn)>100 || strlen(ln)>100){
		        printf("Buffer overflow warning! Exiting.....\n");
		    }else{
				strcpy(s.fn,fn);
				strcpy(s.ln,ln);
				add_student(test,&s);
		    }
        }
        //delete student
    } else if (strcmp(argv[5],"-DS")==0 && argv[6][0]=='-') {
        char* fn = find_flag(argv, argc,"-FN");
        char* ln = find_flag(argv, argc,"-LN");
        if (fn==NULL || ln ==NULL || !checkString(fn) || !checkString(ln)) {
            printf("invalid student name\n");
        }else{
		    Student s= {"","",0,NULL};
		    if(strlen(fn)>100 || strlen(ln)>100){
		        printf("Buffer overflow warning! Exiting.....\n");
		    }else{
				strcpy(s.fn,fn);
				strcpy(s.ln,ln);
				delete_student(test,&s);
			}
        }
        //add grade
    } else if(strcmp(argv[5],"-AG")==0 && argv[6][0]=='-') {
        char *fn = find_flag(argv, argc,"-FN");
        char *ln = find_flag(argv, argc,"-LN");
        char* asg_name = find_flag(argv, argc,"-AN");
        char* grade_s = find_flag(argv,argc, "-G");
        if (grade_s==NULL || asg_name == NULL || fn==NULL || ln ==NULL || !checkString(fn) || !checkString(ln) || strlen(grade_s)>6) {
            printf("invalid add grade");
        }else{
        	if(roundf(atof(grade_s))==atof(grade_s)){
		    	int grade = atoi(grade_s);
				if(grade >=0) {
				    Student s= {"","",0,NULL};
				    if(strlen(fn)>100 || strlen(ln)>100){
				    printf("Buffer overflow warning! Exiting.....\n");
				    }else{
						strcpy(s.fn,fn);
						strcpy(s.ln,ln);
						add_grade(test,asg_name,&s,grade);
				    }
				} else {
				    printf("invalid grade");
				}
		    }else{
		    	printf("Grade has to be a integer.\n");
		    }
		}
    } else {
        printf("Command line format error.\n");
    }
/*
	//print result
    Gradebook *g1 = test;
    //printf("inresult :%s %d\n",g1->assignments->name,g1->num_stu);
    printf("\n***%s***\n",g1->name);
        Student *j = g1->stu;
        while(j!=NULL) {
            printf("(%s, %s, %d)\n",j->fn,j->ln,j->grade);
            j=j->next_stu;
        }
    for(int i = 0; i<argc;i++)
    	printf("%s ",argv[i]);
    Assignment *i = g1->assignments;
    while(i!=NULL) {
        printf("######%s#######\n",i->name);
        Student *j = i->stu;
        while(j!=NULL) {
            printf("(%s, %s, %d)\n",j->fn,j->ln,j->grade);
            j=j->next_stu;
        }
        i=i->next_as;
    }
    */

    store_gradebook(test);

}

int main(int argc, char *argv[]) {
	check_flag(argv,argc);
//read in key
    unsigned char key[16];
    FILE *fp;
    if(argc>=2)
    {
        if(strcmp(argv[1],"-N")!=0 ||strcmp(argv[3],"-K")!=0) {
            printf("missing name or key\n");
            exit(255);
        }
    }
    int j = 0;
    if(strlen(argv[4])!=32) {
        printf("invalid key length: %ld\n",strlen(argv[4]));
        exit(255);
    }
    for(int i=0; i<strlen(argv[4]); i=i+2) {
        char byte[2];
        byte[0]=argv[4][i];
        byte[1]=argv[4][i+1];
        key[j] = (int)strtol(byte,NULL,16);
        //printf("%x ",key[j]);
        j++;
    }

//find cipherfile size
    fp = fopen(argv[2], "rb");
    if (fp == NULL) {
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
  	printf("The gradebook has been compromised.\n");
  	fclose(fp);
  	exit(255);
  }
    //printf("%ld",len);
    fclose(fp);
//write plain gradebook
    fp = fopen(argv[2], "wb");
    fwrite(plaintext, fileSize,1,fp);
    fclose(fp);


//gradebook operation
    parse_cmdline(argc, argv);




//encryption operation
    //find new iv
    unsigned char *new_iv = (unsigned char*) malloc(sizeof(unsigned char)*16);
    unsigned char *new_tag = (unsigned char*) malloc(sizeof(unsigned char)*16);
    FILE *random = fopen("/dev/urandom", "r");
    fread(new_iv, sizeof(unsigned char)*16, 1, random);
    fclose(random);
    // find file sie
    fp = fopen(argv[2], "rb");
    if (fp == NULL) {
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





    /*
    Gradebook g = {"mygradebook",NULL,NULL,0,0};
    store_gradebook(&g);

    Assignment a = {"Final",100,0.25,NULL,NULL};
    add_assignment(&g,&a);
    //printf("inresult :%s %d\n",g.assignments->next_as->name,g.assignments->stu==NULL);

    Assignment b = {"Project",50,0.55,NULL,NULL};
    add_assignment(&g,&b);

    Student s={"John","Smith",100,NULL};
    Student s1={"Russell","Tyler",50,NULL};
    Student s3={"Ted","Mason",750,NULL};
    add_student(&g,&s);
    add_student(&g,&s1);
    add_student(&g,&s3);

    //printf("in main; %s\n",g.assignments->next_as->next_as->name);

    store_gradebook(&g);

    Gradebook *grad;
    grad = read_gradebook("mygradebook");

    printf("inresult :%s %d\n",grad->assignments->next_as->name,grad->assignments->stu==NULL);
    */



    return 0;
}
//gcc -o gradebookadd gradebookadd.c
