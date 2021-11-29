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
#include <stdbool.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

#define DEBUG


typedef struct {
	ActionType action;
	char assignment_name[MAX_BUFFER_LEN];
	int assignment_points;
	float assignment_weight;
	char firstname[MAX_BUFFER_LEN];
	char lastname[MAX_BUFFER_LEN];
	int grade;
} GradebookData;


typedef struct {
	char filename[MAX_BUFFER_LEN];
	ActionType action;
	char assignment_name[MAX_BUFFER_LEN];
	char assignment_points[MAX_BUFFER_LEN];
	char assignment_weight[MAX_BUFFER_LEN];
	char firstname[MAX_BUFFER_LEN];
	char lastname[MAX_BUFFER_LEN];
	char grade[MAX_BUFFER_LEN];
} CmdLineData;


CmdLineData parse_cmdline(int argc, char *argv[]) {
	CmdLineData C = {0};
	int i = 0;
	int j = 0;
	int k = 0;
	int bool_assign_name = 0;
	int bool_firstname = 0;
	int bool_lastname = 0;
	int bool_assign_points = 0;
	int bool_assign_weight = 0;
	int bool_grade = 0;

	if (argc > MAX_CMD_LINE_ARGS) {
		#ifdef DEBUG
		printf("More than allowed Arguments.\n");
		#endif
		printf("Invalid\n");
		exit(INVALID);
	}

  if (argc < 8) {
		#ifdef DEBUG
    printf("Not enough arguments.\n");
		#endif
		printf("Invalid\n");
		exit(INVALID);
	}


  if ((strnlen(argv[0], MAX_USER_INPUT_LEN + 1) >= MAX_USER_INPUT_LEN + 1)){
             #ifdef DEBUG
		printf("Invalid argument lengths for inputs\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
  }

    if( (strnlen(argv[1], 3) != 2) || (strnlen(argv[2], MAX_USER_INPUT_LEN + 1) >= MAX_USER_INPUT_LEN + 1) ){
                #ifdef DEBUG
		printf("Invalid argument lengths for inputs\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
    }

   if((strnlen(argv[3], 3) != 2) || (strnlen(argv[4], KEY_SIZE * 2 + 1) != KEY_SIZE * 2) || (strnlen(argv[5], 4) != 3)) {
		#ifdef DEBUG
		printf("Invalid argument lengths for inputs\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
  
   for (i=6; i<=(argc-1); i+=2) {
		if (strnlen(argv[i], 4) >= 4 || strnlen(argv[i], 4) <= 1) {
			#ifdef DEBUG
			printf("Invalid flag lengths of inputs exceeded\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
	}
   for (i=7; i<=(argc-1); i+=2) {
		if (strnlen(argv[i], MAX_USER_INPUT_LEN + 1) >= MAX_USER_INPUT_LEN + 1) {
			#ifdef DEBUG
			printf("Invalid argument lengths for inputs exceeded\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
	}


  if (strncmp(argv[1], "-N", 3)) {
  	#ifdef DEBUG
		printf("Second argument is not -N\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
  }
  
  for (i=0; i <= (strnlen(argv[2], MAX_USER_INPUT_LEN)-1); i++) {
  	if (!(isalnum(argv[2][i]) || argv[2][i] == '.' || argv[2][i] == '_')) {
  		#ifdef DEBUG
			printf("Filename in not allowed\n");
			#endif
			printf("Invalid\n");
			exit(INVALID);
  	}
  
	if (access(argv[2], F_OK) == -1) { //F_OK test for existing file GNU library
			#ifdef DEBUG
			printf("File does not exist already\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
  }

      if (strncmp(argv[3], "-K", 3)) {
		#ifdef DEBUG
		printf("Fourth argument is not -K\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

  for (i=0; i < strnlen(argv[4], KEY_SIZE * 2); i++) {
  	if(!isxdigit(argv[4][i])) {
  		#ifdef DEBUG
			printf("Invalid key\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
  	}
  }

  // copying command line arguments
  strncpy(C.filename, argv[2], MAX_USER_INPUT_LEN);

  
  if (!strncmp(argv[5], "-AG", 4) || !strncmp(argv[5], "-DS", 4) ||
  		!strncmp(argv[5], "-AS", 4) || !strncmp(argv[5], "-DA", 4) ||
  		!strncmp(argv[5], "-AA", 4)) {

  	if (!strncmp(argv[5], "-AA", 4)) { 
  		for (i=6; i<argc; i+=2) {
  		
				if (!strncmp(argv[i], "-AN", 4)) {
					
				  	for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalnum(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid assignment name\n");
							#endif
							printf("Invalid\n");
							exit(INVALID);
						}
						}
					bool_assign_name++;
					strncpy(C.assignment_name, argv[i+1], MAX_USER_INPUT_LEN);
				}
				
				else if (!strncmp(argv[i], "-P", 3)) {
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isdigit(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid Assignment points\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					bool_assign_points++;
					strncpy(C.assignment_points, argv[i+1], MAX_USER_INPUT_LEN);
				}
				
				else if (!strncmp(argv[i], "-W", 3)) {
					k=0;
					
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isdigit(argv[i+1][j]) || argv[i+1][j] == '.')) {
							#ifdef DEBUG
							printf("Invalid assignment weight argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
						if (argv[i+1][j] == '.') {
							k++;
						}
					}
					if (k > 1) { 
						#ifdef DEBUG
						printf("Invalid assignment weight argument\n");
						#endif
						printf("invalid\n");
						exit(INVALID);
					}
					bool_assign_weight++;
					strncpy(C.assignment_weight, argv[i+1], MAX_USER_INPUT_LEN);
				}
				else { 
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
  		}
  		if (bool_assign_name < 1 &&
  				bool_assign_points < 1 &&
  				bool_assign_weight < 1) {
				#ifdef DEBUG
				printf("Not enough options specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
  		C.action = add_assignment; 
  	}

  	else if (!strncmp(argv[5], "-DA", 4)) { 
  		for (i=6; i<argc; i+=2) {
  			
				if (!strncmp(argv[i], "-AN", 4)) {
					
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalnum(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid assignment name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					bool_assign_name++;
					strncpy(C.assignment_name, argv[i+1], MAX_USER_INPUT_LEN);
				}
				else { 
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
			}
			if (bool_assign_name < 1) {
				#ifdef DEBUG
				printf("Assignment name not specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
  		C.action = delete_assignment; 
  	}

  	else if(!strncmp(argv[5], "-AS", 4)) { 
  		for (i=6; i<argc; i+=2) {
  			
				if (!strncmp(argv[i], "-FN", 4)) {
					
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalpha(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid first name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					bool_firstname++;
					strncpy(C.firstname, argv[i+1], MAX_USER_INPUT_LEN);
				}
				
				else if (!strncmp(argv[i], "-LN", 4)) {
					
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalpha(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid last name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					bool_lastname++;
					strncpy(C.lastname, argv[i+1], MAX_USER_INPUT_LEN);
				}
				else { 
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
			}
			if (bool_firstname < 1 && bool_lastname < 1) {
				#ifdef DEBUG
				printf("Not enough options specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
  		C.action = add_student; 
  	}

  	else if(!strncmp(argv[5], "-DS", 4)) { 
  		for (i=6; i<argc; i+=2) {
  			
				if (!strncmp(argv[i], "-FN", 4)) {
					
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalpha(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid first name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					bool_firstname++;
					strncpy(C.firstname, argv[i+1], MAX_USER_INPUT_LEN);
				}
				
				else if (!strncmp(argv[i], "-LN", 4)) {
					
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalpha(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid last name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					bool_lastname++;
					strncpy(C.lastname, argv[i+1], MAX_USER_INPUT_LEN);
				}
				else { 
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
			}
			if (bool_firstname < 1 && bool_lastname < 1) {
				#ifdef DEBUG
				printf("Not enough options specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
  		C.action = delete_student; 
  	}

		else { 
			for (i=6; i<argc; i+=2) {
  			
				if (!strncmp(argv[i], "-FN", 4)) {
					
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalpha(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid first name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					bool_firstname++;
					strncpy(C.firstname, argv[i+1], MAX_USER_INPUT_LEN);
				}
				
				else if (!strncmp(argv[i], "-LN", 4)) {
					
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalpha(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid last name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					bool_lastname++;
					strncpy(C.lastname, argv[i+1], MAX_USER_INPUT_LEN);
				}
				
				else if (!strncmp(argv[i], "-AN", 4)) {
					
				  	for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalnum(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid assignment name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
						}
				  
					bool_assign_name++;
					strncpy(C.assignment_name, argv[i+1], MAX_USER_INPUT_LEN);
				}
				
				else if (!strncmp(argv[i], "-G", 3)) {
					
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isdigit(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid assignment points argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					bool_grade++;
					strncpy(C.grade, argv[i+1], MAX_USER_INPUT_LEN);
				}
				else { 
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("Invalid\n");
					exit(INVALID);
				}
			}
			if (bool_firstname < 1 && bool_lastname < 1 &&
					bool_assign_name < 1 && bool_grade < 1) {
				#ifdef DEBUG
				printf("Not enough options specified\n");
				#endif
				printf("Invalid\n");
				exit(INVALID);
			}
  		C.action = add_grade; 
		}
  } else { 
  	#ifdef DEBUG
		printf("Invalid flag\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
  }
  return C;
}

  

GradebookData convert_cmdlineargs(const CmdLineData C) {
	GradebookData G = {0};

	G.action = C.action;

	strncpy(G.assignment_name, C.assignment_name, MAX_USER_INPUT_LEN);

	if (strnlen(C.assignment_points, MAX_INT_LEN + 1) >= MAX_INT_LEN + 1) {
		#ifdef DEBUG
		printf("Assignment points exceeded total\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	G.assignment_points = atoi(C.assignment_points);
	if (G.action == add_assignment && G.assignment_points == 0) {
		#ifdef DEBUG
		printf("Assignment points must be bigger than 0\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	G.assignment_weight = atof(C.assignment_weight);
	if (G.action == add_assignment && G.assignment_weight > 1.00) {
		#ifdef DEBUG
		printf("Assignment weight must be in [0,1]\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	strncpy(G.firstname, C.firstname, MAX_USER_INPUT_LEN);

	strncpy(G.lastname, C.lastname, MAX_USER_INPUT_LEN);

	if (strnlen(C.grade, MAX_INT_LEN + 1) >= MAX_INT_LEN + 1) {
		#ifdef DEBUG
		printf("Grade goes beyond total\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	G.grade = atoi(C.grade);

	return G;
}

void modify(GradebookData G, DecryptedGradebook *gradebook) {
  int i = 0;
  int j = 0;
  int k = 0;
  int count = 0;
  float weight = 0.0;

	if (G.action == add_assignment) { 
		if (gradebook->num_assignments >= MAX_ASSIGNMENTS) {
			#ifdef DEBUG
			printf("Gradebook full\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		weight = G.assignment_weight;
		int index = -1;
		for (i = 0; i < MAX_ASSIGNMENTS; i++) {
			
			if (!strncmp(G.assignment_name, gradebook->assignments[i].name, MAX_USER_INPUT_LEN)) {
				#ifdef DEBUG
				printf("Assignment exists\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
			
			if (index == -1 && gradebook->assignment_slot_filled[i] == false) {
				index = i;
			}
			
			weight += gradebook->assignments[i].weight;
			if (weight > 1.00) {
				#ifdef DEBUG
				printf("Total weight exceeded\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
		}
		
		gradebook->num_assignments++;
		gradebook->assignment_slot_filled[index] = true;
		strncpy(gradebook->assignments[index].name, G.assignment_name, MAX_USER_INPUT_LEN);
		gradebook->assignments[index].points = G.assignment_points;
		gradebook->assignments[index].weight = G.assignment_weight;
	} 

	else if (G.action == delete_assignment) { 
		if (gradebook->num_assignments <= 0) {
			#ifdef DEBUG
			printf("Gradebook doesn't have any assignments\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		int index = -1;
		for (i = 0; i < MAX_ASSIGNMENTS; i++) {
			
			if (!strncmp(G.assignment_name, gradebook->assignments[i].name, MAX_USER_INPUT_LEN)) {
				index = i;
			}
		}
		if (index == -1) {
			#ifdef DEBUG
			printf("Assignment doesn't exist\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		
		gradebook->num_assignments--;
		gradebook->assignment_slot_filled[index] = false;
		memset(gradebook->assignments[index].name, '\0', MAX_USER_INPUT_LEN);
		gradebook->assignments[index].points = 0;
		gradebook->assignments[index].weight = 0.0;
		
		for (i = 0; i < MAX_STUDENTS; i++) {
			gradebook->students[i].grades[index] = 0;
		}
	} 

	else if (G.action == add_student) { 
		if (gradebook->num_students >= MAX_STUDENTS) {
			#ifdef DEBUG
			printf("Gradebook full\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}

		int index = -1;
		for (i = 0; i < MAX_STUDENTS; i++) {
		
			if (!(strncmp(G.firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN)) &&
					!(strncmp(G.lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN))) {
				#ifdef DEBUG
				printf("Invalid\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
		
			if (index == -1 && gradebook->student_slot_filled[i] == false) {
				index = i;
			}
		}
		
		gradebook->num_students++;
		gradebook->student_slot_filled[index] = true;
		strncpy(gradebook->students[index].firstname, G.firstname, MAX_USER_INPUT_LEN);
		strncpy(gradebook->students[index].lastname, G.lastname, MAX_USER_INPUT_LEN);
		
		for (i = 0; i < MAX_ASSIGNMENTS; i++) {
			gradebook->students[index].grades[i] = 0;
		}
	} 

	else if (G.action == delete_student) { 
		if (gradebook->num_students <= 0) {
			#ifdef DEBUG
			printf("Gradebook has no students\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		int index = -1;
		for (i = 0; i < MAX_STUDENTS; i++) {
			
			if (!(strncmp(G.firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN)) &&
					!(strncmp(G.lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN))) {
				index = i;
			}
		}
		if (index == -1) {
			#ifdef DEBUG
			printf("Invalid\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}

		
		gradebook->num_students--;
		gradebook->student_slot_filled[index] = false;
		memset(gradebook->students[index].firstname, '\0', MAX_USER_INPUT_LEN);
		memset(gradebook->students[index].lastname, '\0', MAX_USER_INPUT_LEN);
		
		for (i = 0; i < MAX_STUDENTS; i++) {
			gradebook->students[index].grades[i] = 0;
		}
	} 

	else if (G.action == add_grade) { 
		
		if (gradebook->num_students <= 0) {
			#ifdef DEBUG
			printf("Gradebook doesn't have any students\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		int index = -1;
		for (i = 0; i < MAX_STUDENTS; i++) {
			
			if (!(strncmp(G.firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN)) &&
					!(strncmp(G.lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN))) {
				index = i;
			}
		}
		if (index == -1) {
			#ifdef DEBUG
			printf("Invalid\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		
		if (gradebook->num_assignments <= 0) {
			#ifdef DEBUG
			printf("Gradebook doesn't have any assignments\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		int index2 = -1;
		for (i = 0; i < MAX_ASSIGNMENTS; i++) {
			
			if (!strncmp(G.assignment_name, gradebook->assignments[i].name, MAX_USER_INPUT_LEN)) {
				index2 = i;
			}
		}
		
		gradebook->students[index].grades[index2] = G.grade;
	} 
	else { 
		#ifdef DEBUG
		printf("Failure: Action Not Found");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
}

int main(int argc, char *argv[]) {
	CmdLineData C = {0};
	GradebookData G = {0};
	DecryptedGradebook dec_gradebook = {0};
	EncryptedGradebook enc_gradebook = {0};
	FILE *infile;
	FILE *outfile;
	unsigned char key[KEY_SIZE];
	unsigned char iv[IV_SIZE];
      	unsigned char tag[MAC_SIZE];
	unsigned char mac_tag[MAC_SIZE];
	unsigned char input_key[33] = {0};
	int ret;
	int i;
	int j;
	int k;


	C = parse_cmdline(argc, argv);


	if (strnlen(argv[4], KEY_SIZE * 2 + 1) == KEY_SIZE * 2) {
		memcpy(input_key, argv[4], KEY_SIZE * 2);
	}
	else {
		#ifdef DEBUG
		printf("Invalid key\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}


  G = convert_cmdlineargs(C);


	infile = fopen(C.filename, "r");
	if (infile == NULL) {
		#ifdef DEBUG
		printf("Could not open gradebook\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	} else {
		ret = fread(enc_gradebook.iv, 1, sizeof(EncryptedGradebook), infile);
		fflush(infile);
		fclose(infile);

		if (ret != sizeof(EncryptedGradebook)) {
			#ifdef DEBUG
			printf("Invalid file length\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}

		
		unsigned char *hex_key = input_key;
		for (int count = 0; count < KEY_SIZE; count++) {
			sscanf(hex_key, "%2hhx", &key[count]);
			hex_key += 2;
		}

	   
		memcpy(tag, enc_gradebook.tag, sizeof(tag));

	
		unsigned char* mac_res;
		mac_res = HMAC(EVP_md5(), key, sizeof(key), (unsigned char *)&enc_gradebook.iv, sizeof(EncryptedGradebookSize), mac_tag, NULL);

	
		if (!strncmp(tag, mac_tag, MAC_SIZE)) {
		
			memcpy(iv, enc_gradebook.iv, sizeof(iv));
			ret = decrypt((unsigned char *)&enc_gradebook.encrypted_data, sizeof(DecryptedGradebook), key, iv, (unsigned char *)&dec_gradebook.num_assignments);
			if (ret == -1) {
				#ifdef DEBUG
				printf("Authentication failed\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
		} else {
			#ifdef DEBUG
			printf("Authentication failed\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
	}


	modify(G, &dec_gradebook);

	
	ret = RAND_bytes(iv, sizeof(iv));
	if (ret == -1) {
		#ifdef DEBUG
		printf("Failure: Random IV generation\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}


	memcpy(enc_gradebook.iv, iv, sizeof(iv));
	ret = encrypt((unsigned char *)&dec_gradebook.num_assignments, sizeof(DecryptedGradebook), key, iv, (unsigned char *)&enc_gradebook.encrypted_data);
	if (ret == -1) {
		#ifdef DEBUG
		printf("Encryption failed\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}


	unsigned char* mac_res;
	mac_res = HMAC(EVP_md5(), key, sizeof(key), (unsigned char *)&enc_gradebook.iv, sizeof(EncryptedGradebookSize), tag, NULL);

	
	memcpy(enc_gradebook.tag, tag, sizeof(tag));


	outfile = fopen(C.filename, "w");
	if (outfile == NULL) {
		#ifdef DEBUG
		printf("Could not create file\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}


	fwrite(enc_gradebook.iv, 1, sizeof(EncryptedGradebook), outfile);
	fflush(outfile);
	fclose(outfile);
  	return 0;
}
