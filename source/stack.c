#include "stack.h"


const char* sync_path = "/home/stanislav/Documents/MIPT/3rd_semester/Computer_technologies";
#define N_SEMS 3


/*TODO: 1) check for empty stack, add new semafore for current size
        2) same for full stack
        3) print value being popped */

void stack_print_elem(stack_t* stack) {
    for (int i = 0; i < stack->m_cur_size; ++i) {
        fprintf(stdout, "Stack [%d] = %d\n", i, *((int*) (stack->m_memory + (2 + i) * sizeof(void*) ) ));
    }
}


void stack_print(stack_t* stack) {
    fprintf(stdout, "\n\n\nStack address: %p\n", stack);
    fprintf(stdout, "Stack max size: %d\n", stack->m_max_size);
    fprintf(stdout, "Stack cur size: %d\n", stack->m_cur_size);
    fprintf(stdout, "Stack shared memory beginning: %p\n", stack->m_memory);
    fprintf(stdout, "Stack shared memory[0]: %d\n", *((int*) stack->m_memory));
    fprintf(stdout, "Stack shared memory[1]: %d\n", *((int*) (stack->m_memory + sizeof(void*)) ));
    stack_print_elem(stack);
    fprintf(stdout, "\n\n\n");
}

int set_sem_set(int key) {

    int sem_id = semget(key, N_SEMS, IPC_CREAT | IPC_EXCL | 0666 );
    if (sem_id > 0 && errno != EEXIST) { 

    DBG(fprintf(stdout, "Creating new semaphore set\n"))
    sem_id = semget(key, N_SEMS, IPC_CREAT | 0666);

    if (sem_id == -1) {
        perror("Error in semget() in set_sem_set(): ");
    }

    } else {
        DBG(fprintf(stdout, "Attaching old semaphore set\n"))
        sem_id = semget(key, N_SEMS, 0666);
        if (sem_id == -1) {
            perror("Error in semget() in set_sem_set(): ");
        }
    }
    return sem_id;
}

void semdel(int key) {
    int sem_id = semget(key, N_SEMS, 0666);
    if (sem_id == -1) {
        perror("Erorr in semget(): ");
    }

    int resop = semctl(sem_id, 0, IPC_RMID);
    if (resop == -1) {
        perror("Error in semctl(): ");
    }
}

void shmdel(int key, int size) {
    int id = shmget(key, size * sizeof(char), 0);
    if (id == -1) {
        perror("Erorr in shmget(): ");
    }
    int resop = shmctl (id , IPC_RMID , NULL);
    if (resop == -1) {
        perror("Error in shmctl(): ");
    }
}

stack_t* attach_stack(key_t key, int size) {

    assert(size > 0);
    assert(key != -1);
    int resop = 0;
    stack_t* stack = (stack_t*) calloc(1, sizeof(struct stack_t));
    assert(stack);
    int stack_size = size;

    DBG(fprintf(stdout, "Stack size: %d elements\n", stack_size))

    int sem_id = set_sem_set(key);

    if (resop == -1) {
        perror("Error in semctl() when setting empty semaphore: ");
    }


    int id = shmget(key, stack_size * sizeof(void*), IPC_CREAT | IPC_EXCL | 0666);

    if (id == -1) {
        fprintf(stdout, "Stack exists, attaching the stack\n");

        id = shmget(key, stack_size * sizeof(void*), 0666);
        if (id == -1) {
            perror("Error in shmget: ");
        }

        stack->m_memory = (void*) shmat(id, NULL, 0);
        if (stack->m_memory == (void*) -1) {
            perror("Error in shmat(): ");
            return NULL;
        }
        stack->m_max_size = *((int*) (stack->m_memory));
        stack->m_cur_size = *((int*) (stack->m_memory + sizeof(void*)));

        return stack;
    } else {
        fprintf(stdout, "No stack exists, creating new stack\n");
        
        id = shmget(key, stack_size * sizeof(char), 0666);
        if (id == -1) {
            perror("Error in shmget(): ");
        }

        stack->m_memory = shmat(id, NULL, 0);

        if (stack->m_memory == (void*) -1) {
            perror("Error in shmat(): ");
            return NULL;
        }

        int max_size = size;
        int cur_size = 0;

        int resop  = semctl(sem_id, 0, SETVAL, 1);
        DBG(fprintf(stdout, "Setting semaphore to 1\n"))
        resop  = semctl(sem_id, 1, SETVAL, max_size );
        DBG(fprintf(stdout, "Setting empty to stack_size \n"))
        resop  = semctl(sem_id, 2, SETVAL, 0);
        DBG(fprintf(stdout, "Setting full to 0 \n"));

        //DBG(fprintf(stdout, "Setting empty to stack_size: %d \n", sval))
        if (resop == -1) {
            perror("Error in semctl() when setting empty semaphore: ");
        }

        DBG((fprintf(stdout, "Copying max size %d and cur size %d to shared memory[0] = %p and memory[1] = %p\n", max_size, cur_size,
                                stack->m_memory, stack->m_memory + sizeof(void*))))
        memcpy((void*) stack->m_memory, &max_size, sizeof(int));
        memcpy((void*) stack->m_memory + sizeof(void*), &cur_size, sizeof(int));

        stack->m_max_size = *((int*) (stack->m_memory));
        stack->m_cur_size = *((int*) (stack->m_memory + sizeof(void*)));

        DBG(fprintf(stdout, "Stack max_size %d at %p stack cur_size %d at %p\n", stack->m_max_size,
                            stack->m_memory, stack->m_cur_size, stack->m_memory + sizeof(void*)))

        return stack;
    }


}

int get_size(stack_t* stack) {
    assert(stack != NULL);
    //update cur size
    stack->m_max_size = *(( int*) (stack->m_memory ));

    return stack->m_max_size;
}

int get_count(stack_t* stack) {
    assert(stack != NULL);
    //update cur size
    stack->m_cur_size = *(( int*) (stack->m_memory +  sizeof(void*)) );
    
    return stack->m_cur_size;
}


int detach_stack(stack_t* stack) {
    int resop = 0;
    resop = shmdt(stack->m_memory);
    if (resop == -1) {
        perror("Error in detaching stack with shmdt(): ");
    }
    free(stack);
    return resop;
}

int mark_destruct(stack_t* stack) {
    assert(stack != NULL);
    DBG(fprintf(stdout, "Marking stack for destruction\n"))

    int key = ftok(sync_path, SYNC);
    if (key == -1) {
        perror("Error in ftok(): ");
    }

    int id = shmget(key, 0, 0);
    if (id == -1) {
        perror("Error in shmget: ");
    }

    int resop = shmctl(id, IPC_RMID, 0);
    if (resop == -1) {
        perror("Error in marking stack for destruction: ");
    }

    return resop;

}

int push(stack_t* stack, void* val) {

    int key = ftok(sync_path, SYNC);
    if (key == -1) {
        perror("Error in ftok(): ");
    }

    // creating or attaching semaphore set
    int sem_id = set_sem_set(key);

    struct sembuf* sops = (struct sembuf*) calloc(3, sizeof(struct sembuf));
    assert(sops);
    
    // set semaphore to SEM_UNDO so stack does not crash when any processes acquring this semaphore get terminated
    sops[0].sem_flg = 0;
    sops[1].sem_flg = SEM_UNDO;

    int sval = 0;
    int resop = 0;

    // semaphore numbers: semaphore - 0, empty - 1, full - 2
    // CHANGE: sops[0] - empty --, sops[1] = semaphore --

    sops[0].sem_num = 1;
    sops[0].sem_op = -1;
    sops[1].sem_num = 0;
    sops[1].sem_op = -1;

    DBG(fprintf(stdout, "Empty --, semafore --\n"))

    struct timespec timeout;
    set_wait(1, &timeout);

    semtimedop(sem_id, sops, 2, &timeout);
     if (errno == EAGAIN) {
        fprintf(stdout, "Timeout interval exceeded, operation aborted\n");
        errno = 0;
        return -1;
    }

    sval = semctl(sem_id, 1, GETVAL);
    DBG(fprintf(stdout, "empty value upon entering critical area: %d\n", sval))
    sval = semctl(sem_id, 0, GETVAL);
    DBG(fprintf(stdout, "Semaphore value on entering critical area :%d\n", sval))

    //DBG(fprintf(stdout, "Semaphore value after taking:%d\n", sval))
    if (resop == -1) {
        perror("Error in semop(): ");
    }

    
    // update cur size

    stack->m_cur_size = *(( int*) (stack->m_memory +  sizeof(void*)) );

    memcpy((void*) (stack->m_memory + (stack->m_cur_size + 2) * sizeof(void*)), val, sizeof(void*));

    int new_cur_size = stack->m_cur_size + 1;
    stack->m_cur_size = new_cur_size;


    if (resop == -1) {
        perror("Error in semop(): ");
    }

    memcpy((void*) (stack->m_memory + sizeof(void*)), &new_cur_size, sizeof(int));


   // semaphore numbers: semaphore - 0, empty - 1, full - 2
    // CHANGE: sops[0] - semaphore ++, sops[1] - full ++

    sops[0].sem_num = 0;
    sops[0].sem_op = 1;
    sops[1].sem_num = 2;
    sops[1].sem_op = 1;

    sops[1].sem_flg = 0;
    sops[0].sem_flg = SEM_UNDO;

    DBG(fprintf(stdout, "semaphore ++, full ++\n"))

    sval = semctl(sem_id, 0, GETVAL);
    DBG(fprintf(stdout, "Semaphore value on exiting critical area:%d\n", sval))
    sval = semctl(sem_id, 2, GETVAL);
    DBG(fprintf(stdout, "Full value on exiting critical area :%d\n", sval))


    semtimedop(sem_id, sops, 2, &timeout);

    if (resop == -1) {
        perror("Error in semop(): ");
    }

    free(sops);
    return 0;
}


int pop(stack_t* stack, void** val) {

    int key = ftok(sync_path, SYNC);
    if (key == -1) {
        perror("Error in ftok(): ");
    }

    // creating or attaching semaphore set
    int sem_id = set_sem_set(key);


    struct sembuf* sops = (struct sembuf*) calloc(3, sizeof(struct sembuf));
    assert(sops);


    // semaphore numbers: semaphore - 0, empty - 1, full - 2
    // CHANGE: sops[0] - full --, sops[1] = semaphore --

    sops[0].sem_num = 2;
    sops[0].sem_op = -1;
    sops[1].sem_num = 0;
    sops[1].sem_op = -1;

    sops[1].sem_flg = SEM_UNDO;
    sops[0].sem_flg = 0;

    int sval = 0;
    int resop = 0;


    struct timespec timeout;
    set_wait(1, &timeout);

    semtimedop(sem_id, sops, 2, &timeout);
    if (errno == EAGAIN) {
        fprintf(stdout, "Timeout interval exceeded, operation aborted\n");
        errno = 0;
        return -1;
    }

    sval = semctl(sem_id, 2, GETVAL);
    DBG(fprintf(stdout, "Full value after taking:%d\n", sval))
    sval = semctl(sem_id, 0, GETVAL);
    DBG(fprintf(stdout, "Semaphore value after taking :%d\n", sval))

    if (resop == -1) {
        perror("Error in semop(): ");
    }

    //update cur size
    stack->m_cur_size = *(( int*) (stack->m_memory +  sizeof(void*)) );


    void** pop_value = stack->m_memory + (2 + stack->m_cur_size) * sizeof(void*);
    //DBG(fprintf(stdout, "Value being popped: %p\n", pop_value ));
    val = pop_value;

    //set new cur size
    int new_cur_size = stack->m_cur_size - 1;
    stack->m_cur_size = new_cur_size;
    memcpy((void*) (stack->m_memory + sizeof(void*)), &new_cur_size, sizeof(int));


    // releasing semaphore

   // semaphore numbers: semaphore - 0, empty - 1, full - 2
    // CHANGE: sops[0] - semaphore ++, sops[1] - empty ++

    sops[0].sem_num = 0;
    sops[0].sem_op  = 1;
    sops[1].sem_num = 1;
    sops[1].sem_op  = 1;

    sops[0].sem_flg = SEM_UNDO;
    sops[1].sem_flg = 0;


    semtimedop(sem_id, sops, 2, &timeout);

    sval = semctl(sem_id, 0, GETVAL);
    DBG(fprintf(stdout, "Semaphore value after taking (finished critical area):%d\n", sval))
    sval = semctl(sem_id, 1, GETVAL);
    DBG(fprintf(stdout, "Empty value after taking:%d\n", sval))

    if (resop == -1) {
        perror("Error in semop(): ");
    }

    return 0;; 
}

int set_wait(int val, struct timespec* timeout) {
    if (val == -1) {
        return val;
    } else if (val == 1) {
        timeout->tv_sec = 1;
        timeout->tv_nsec = 0;
        return val;
    } else if (val == 0) {
        return val;
    }
}
