#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>   
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SEM_COUNT 5

/**
 * Enum type for handling error codes in programm.
 */
enum{
	SUCCESS              =  1, /**< Successful exit of a function. */
	E_ARGUMENT_MISSING   = -2, /**< Argument of a programm is missing or there are too many. */
	E_ARGUMENT_NOT_NUM   = -3, /**< Argument is not an integer. */
	E_ARGUMENT_WRONG_VAL = -4  /**< Argument is out of a scope of a legit values. */
};

/**
 * Enum type for handling array of semaphores.
 */
enum{
	FILE_MUTEX  = 0, /**< Inedx of semaphore handling writing/reading to/from a file. */
	VAR_MUTEX   = 1, /**< Index of semaphore handling writing/reading to/from a shared variables. */
	CHILD_QUEUE = 2, /**< Index of semaphore handling childs entering and leaving centre. */
	ADULT_QUEUE = 3, /**< Index of semaphore handling adults entering and leaving centre. */
	BARRIER     = 4  /**< Index of semaphore handling finishing the processes all at once */
};
/**
 * Structure used to pass the variables into the shared memory.
 */
struct memory{
	unsigned int instruction;      /**< Number of instruction printed at the beginning of a line. */
	unsigned int finished_count;   /**< Number of processes waiting at the finish barrier. */
	unsigned int all_processes;    /**< Sum of all adult and child processes. */
	unsigned int adult_count;      /**< Number of adults in the centre. */
	unsigned int adult_leaving;    /**< Number of adults leaving the centre. */
	unsigned int adult_sum;        /**< Total number of adults who already entered the adult_work function. */
	unsigned int child_sum;        /**< Total number of children who already entered the children_work function. */
	unsigned int children_count;   /**< Number of childs in the centre. */
	unsigned int children_waiting; /**< Number of children waiting to enter the centre. */
	sem_t semaphores[SEM_COUNT];   /**< Array of semaphores used to control the processes. */
};

/**
 * Macro will chceck whether the given arguments are in a scope of valid values or not.
 * @return 1 If everything is in a scope or 0 otherwise.
 */
#define TIME_CHECK(AGT, CGT, AWT, CWT)\
		(*AGT >= 0 && *AGT < 5001 && *CGT >= 0 && *CGT < 5001 && *AWT >= 0 && *AWT < 5001 && *CWT >= 0 && *CWT < 5001 ? 1 : 0)

/**
 * Function will chceck all the arguments given when the programm was run and store them.
 * @params A-CWT Arguments from the cmd when the programm was run.
 * @return Error code according to the problem with parsing. Error code is described in enum.
 */
int get_args(int argc, char** argv, int* A, int* C, int* AGT, int* CGT, int* AWT, int* CWT);

/**
 * Function will check whether the string is an integer and store it.
 * @param arg String to be checked.
 * @param converted Where the converted string will be stored.
 */
bool check_arg_int(const char* arg, int *converted);

/**
 * Function will initialize shared memory and semaphores at the beginning of a programm.
 * @return Pointer to the shared memory structure.
 */
struct memory* init();

/**
 * Function will deallocate all of the allocated shared memory.
 * @param mem Shared memory to be deallocated.
 */
void deinit(struct memory* mem);

/**
 * Function simulates children waiting for the entry to the centre, being in centre and then leave
 * the centre. Detailed describtion in the code of the function.
 * @param CWT Time during which child simulates being in the centre. 
 * @param A Number of aduts to be generated.
 * @param mem Shared memory.
 * @param file File for the output operation.
 */
void children_work(int CWT, int A, struct memory* mem, FILE* file);

/**
 * Function simulates adult entering the centre, being in centre, trying to leave
 * the centre and then leave the centre.  Detailed describtion in the code of the function.
 * @param AWT Time during which adult simulates being in the centre. 
 * @param A Number of aduts to be generated.
 * @param C Number of children to be generated.
 * @param mem Shared memory.
 * @param file File for the output operation.
 */
void adult_work(int AWT, int A, int C, struct memory* mem, FILE* file);

/**
 * Function includes barrier to wait for all the processes to get to the barrier and
 * then end all of them at once.
 * @param name Either "A" or "C", depends if the process is an adult or a child.
 * @param num Number of a child or an adult.
 * @param mem Shared memory.
 * @param file File for the output operation.  
 */
void finish(const char* name, int num, struct memory* mem, FILE* file);

#endif