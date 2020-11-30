

#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


//#define ERROR fprintf(stderr, "Error %s in %d %s\n", strerror(errno), __LINE__, __func__);
#define DEBUG 0
#define DBG(stmt) if (DEBUG) {stmt;}

#define POISON 6666 

#define SYNC 777


typedef struct stack_t stack_t;

struct stack_t {
    int m_max_size;
    int m_cur_size;

    void** m_memory;
};

/* Attach (create if needed) shared memory stack to the process.
Returns stack_t* in case of success. Returns NULL on failure. */
stack_t* attach_stack(key_t key, int size);

/* Deleting semaphore set */
void semdel(int key);

/* Print stack memory */
void stack_print(stack_t* stack);

/* Detaches existing stack from process. 
Operations on detached stack are not permitted since stack pointer becomes invalid. Returns 
 0 on success and -1 if error occures */
int detach_stack(stack_t* stack);


/* Marks stack to be destroed. Destruction is done after all detaches have been completed */ 
int mark_destruct(stack_t* stack);

/* Returns stack maximum size. */
int get_size(stack_t* stack);

/* Returns current stack size. */
int get_count(stack_t* stack);

/* Push val into stack. */
int push(stack_t* stack, void* val);

/* Pop val from stack into memory */
int pop(stack_t* stack, void** val);

/* Deletes manually segment */
void shmdel(int key, int size);
//---------------------------------------------
/* Additional tasks */

/* Control timeout on push and pop operations in case stack is full or empty.
val == -1 Operations return immediatly, probably with errors.
val == 0  Operations wait infinitely.
val == 1  Operations wait timeout time.
*/
int set_wait(int val, struct timespec* timeout);