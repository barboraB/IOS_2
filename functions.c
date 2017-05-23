#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "functions.h"
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>   
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static int mem_id; // ID for shared memory.

int get_args(int argc, char** argv, int* A, int* C, int* AGT, int* CGT, int* AWT, int* CWT){
	if(argc < 7 || argc > 7){
		return E_ARGUMENT_MISSING; //Argument missing or too many arguments.
	}

	if(!check_arg_int(argv[1], A  )) return E_ARGUMENT_NOT_NUM; //Argument is not a number
	if(!check_arg_int(argv[2], C  )) return E_ARGUMENT_NOT_NUM;
	if(!check_arg_int(argv[3], AGT)) return E_ARGUMENT_NOT_NUM;
	if(!check_arg_int(argv[4], CGT)) return E_ARGUMENT_NOT_NUM;
	if(!check_arg_int(argv[5], AWT)) return E_ARGUMENT_NOT_NUM;
	if(!check_arg_int(argv[6], CWT)) return E_ARGUMENT_NOT_NUM;

	if(*A <= 0 || *C <= 0) return E_ARGUMENT_WRONG_VAL; //Condition for arguments not met.
	if(!TIME_CHECK(AGT, CGT, AWT, CWT)) return E_ARGUMENT_WRONG_VAL;

	return 1;
}

bool check_arg_int(const char* arg, int *converted){
	char* ptr = NULL;
	*converted = strtol(arg, &ptr, 10);
	if(*ptr != '\0'){
		return false;
	}
	return true;
}

struct memory* init(){

	struct memory* mem;
	mem_id = shmget(IPC_PRIVATE, sizeof(struct memory), IPC_CREAT | 0666);
	if (mem_id == -1){
		fprintf(stderr, "Shared memory allocation failed.\n");
		exit(2);
	}
	mem = (struct memory *) shmat(mem_id, NULL, 0);
	if (mem == NULL){
		fprintf(stderr, "Cannot attach shared memory.\n");
		goto shm;
	}

	if(sem_init(&mem->semaphores[FILE_MUTEX], 1, 1) == -1){
		fprintf(stderr, "Semaphore allocation failed.\n");
	}
	if(sem_init(&mem->semaphores[VAR_MUTEX], 1, 1) == -1){
		fprintf(stderr, "Semaphore allocation failed.\n");
		goto file_mutex;
	}
	if(sem_init(&mem->semaphores[CHILD_QUEUE], 1, 0) == -1){
		fprintf(stderr, "Semaphore allocation failed.\n");
		goto var_mutex;
	}
	if(sem_init(&mem->semaphores[ADULT_QUEUE], 1, 0) == -1){
		fprintf(stderr, "Semaphore allocation failed.\n");
		goto child_queue;
	}
	if(sem_init(&mem->semaphores[BARRIER], 1, 0) == -1){
		fprintf(stderr, "Semaphore allocation failed.\n");
		goto adult_queue;
	}

	return mem;

	adult_queue: sem_destroy(&mem->semaphores[ADULT_QUEUE]);
	child_queue: sem_destroy(&mem->semaphores[CHILD_QUEUE]);
	var_mutex:   sem_destroy(&mem->semaphores[VAR_MUTEX]);
	file_mutex:  sem_destroy(&mem->semaphores[FILE_MUTEX]);
	shm:         shmctl(mem_id, IPC_RMID, NULL);

	exit(2);
}

void deinit(struct memory* mem){
	for (int i = 0; i < SEM_COUNT; i++){
		sem_destroy(&mem->semaphores[i]);
	}

	shmctl(mem_id, IPC_RMID, NULL);
	shmdt(mem);	
	//free(mem);
}

void children_work(int CWT, int A, struct memory* mem, FILE* file){
	setbuf(file, NULL);
	sem_wait(&mem->semaphores[VAR_MUTEX]);
	mem->child_sum += 1;
	int child = mem->child_sum;
	mem->instruction += 1;
	fprintf(file, "%d\t: C %d\t: started\n", mem->instruction, child);
	if(mem->children_count < ((mem->adult_count)*3)){
		mem->children_count += 1;
		mem->instruction += 1;
		fprintf(file, "%d\t: C %d\t: enter\n", mem->instruction, child);
		sem_post(&mem->semaphores[VAR_MUTEX]);
	} else {
		mem->children_waiting += 1;
		if(mem->adult_sum != (unsigned int)A){
			mem->instruction += 1;
			fprintf(file, "%d\t: C %d\t: waiting : %d : %d\n", mem->instruction, child, mem->adult_count, mem->children_count);
		}
		sem_post(&mem->semaphores[VAR_MUTEX]);
		sem_wait(&mem->semaphores[CHILD_QUEUE]);
		sem_wait(&mem->semaphores[VAR_MUTEX]);
		mem->instruction += 1;
		fprintf(file, "%d\t: C %d\t: enter\n", mem->instruction, child);
		sem_post(&mem->semaphores[VAR_MUTEX]);
	}

	int time = rand() % (CWT+1);
	usleep(time*1000);

	sem_wait(&mem->semaphores[VAR_MUTEX]);
	mem->instruction += 1;
	fprintf(file, "%d\t: C %d\t: trying to leave\n", mem->instruction, child);
	mem->children_count -= 1;
	mem->instruction += 1;
	fprintf(file, "%d\t: C %d\t: leave\n", mem->instruction, child);
	if(mem->adult_leaving && mem->children_count <= (3*(mem->adult_count -1))){
		mem->adult_leaving -= 1;
		mem->adult_count -= 1;
		sem_post(&mem->semaphores[ADULT_QUEUE]);
	}
	sem_post(&mem->semaphores[VAR_MUTEX]);
	finish("C", child, mem, file);
}


void adult_work(int AWT, int A, int C, struct memory* mem, FILE* file){
	setbuf(file, NULL);
	sem_wait(&mem->semaphores[VAR_MUTEX]);
	mem->adult_sum += 1;
	int adult = mem->adult_sum;
	mem->instruction += 1;
	fprintf(file, "%d\t: A %d\t: started\n", mem->instruction, adult);
	mem->instruction += 1;
	fprintf(file, "%d\t: A %d\t: enter\n", mem->instruction, adult);
	mem->adult_count += 1;
	if(mem->children_waiting){
		int min = mem->children_waiting < 3 ? mem->children_waiting : 3;
		for(int i = 0; i < min; i++){
			sem_post(&mem->semaphores[CHILD_QUEUE]);
		}
		mem->children_waiting -= min;
		mem->children_count += min;
	}
	sem_post(&mem->semaphores[VAR_MUTEX]);

	int time = rand() % (AWT+1);
	usleep(time*1000);

	sem_wait(&mem->semaphores[VAR_MUTEX]);
	mem->instruction += 1;
	fprintf(file, "%d\t: A %d\t: trying to leave\n", mem->instruction, adult);
	if(mem->children_count <= 3*(mem->adult_count-1)){
		mem->adult_count -= 1;
		mem->instruction += 1;
		fprintf(file, "%d\t: A %d\t: leave\n", mem->instruction, adult);
		sem_post(&mem->semaphores[VAR_MUTEX]);
	} else {
		mem->adult_leaving += 1;
		mem->instruction += 1;
		fprintf(file, "%d\t: A %d\t: waiting : %d : %d\n", mem->instruction, adult, mem->adult_count, mem->children_count);
		sem_post(&mem->semaphores[VAR_MUTEX]);
		sem_wait(&mem->semaphores[ADULT_QUEUE]);
		sem_wait(&mem->semaphores[VAR_MUTEX]);
		mem->instruction += 1;
		fprintf(file, "%d\t: A %d\t: leave\n", mem->instruction, adult);
		sem_post(&mem->semaphores[VAR_MUTEX]);
	}

	if(mem->adult_sum == (unsigned int)A && (mem->adult_count == 0)){
		for(int i = 0; i < C; i++){
			sem_post(&mem->semaphores[CHILD_QUEUE]);
		}
	}
	finish("A", adult, mem, file);
}

void finish(const char* name, int num, struct memory * mem, FILE* file){
	setbuf(file, NULL);
	sem_wait(&mem->semaphores[FILE_MUTEX]);
	mem->finished_count += 1;
	sem_post(&mem->semaphores[FILE_MUTEX]);

	if(mem->finished_count == mem->all_processes){
		sem_post(&mem->semaphores[BARRIER]);
	}
	sem_wait(&mem->semaphores[BARRIER]);
	sem_post(&mem->semaphores[BARRIER]);
	if(name != NULL){
		sem_wait(&mem->semaphores[FILE_MUTEX]);
		mem->instruction += 1;
		fprintf(file, "%d\t: %s %d\t: finished\n", mem->instruction, name, num);
		sem_post(&mem->semaphores[FILE_MUTEX]);
	}
	fclose(file);
	exit(0);
}