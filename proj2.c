#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "functions.h"

int main(int argc, char * argv[]){
	int A, C, AGT, CGT, AWT, CWT;
	int err_num = get_args(argc, argv, &A, &C, &AGT, &CGT, &AWT, &CWT);

	srand(time(NULL));

	switch(err_num){
		case E_ARGUMENT_MISSING:	
				fprintf(stderr, "Invalid number of arguments.\n");
				return EXIT_FAILURE;
		case E_ARGUMENT_NOT_NUM: 
				fprintf(stderr, "Invalid argument - argument is not a number.\n");
				return EXIT_FAILURE;
		case E_ARGUMENT_WRONG_VAL:
				fprintf(stderr, "Invalid argument - argument is not in a scope of valid values.\n");
				return EXIT_FAILURE;
	}

	//Opening a file for the output.
	FILE* file;
	file = fopen("proj2.out", "w");
	if(file == NULL){
		fprintf(stderr, "File cannot be opened.\n");
		return 2;
	}

	struct memory* mem = init();
	mem->all_processes = A+C+1;
	//int status = 0;

	pid_t id = fork();
	pid_t child_id;

	/**
	 * Generating processes. If the first fork is an adult it will fork again and then the child
	 * process will generate children processes. If the first fork is a child it will generate 
	 * adult processes. It is implemeted like this so we could wait only for adult process.
	 */
	if(id > 0){
		child_id = fork();
		if(child_id == 0){
			int i = 0;
			setbuf(file, NULL);
			for(i = 0; i < C; i++){
				int time = rand() % (CGT+1);
				usleep(time*1000);
				pid_t id = fork();
				if(id == 0){
					children_work(CWT, A, mem, file);
					exit(0);
				}
			}
			exit(0);
		}	
	} else {
		int i = 0;
		setbuf(file, NULL);
		for(i = 0; i < A; i++){
			int time = rand() % (AGT+1);
			usleep(time*1000);
			pid_t id = fork();
			if(id == 0){
				adult_work(AWT, A, C, mem, file);
				exit(0);
			}
		}
		exit(0);
	}

	// Waiting for all the processes to end to deinit memory and close a file.
	finish(NULL, 0, mem, file);
	
	fclose(file);
	deinit(mem);
	return 0;
}