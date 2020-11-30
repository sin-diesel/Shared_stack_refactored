#include "stack.h"


#define ERROR fprintf(stdout, "Error in line %d, func %s\n", __LINE__, __func__);
const char* sync_path = "/home/stanislav/Documents/MIPT/3rd_semester/Computer_technologies";

#define N_SEMS 1


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

int set_sem(int key) {

    int sem_id = semget(key, N_SEMS, 0666);
    if (sem_id < 0 && errno == ENOENT) { 
        DBG(fprintf(stdout, "Creating new semaphore set\n"))
        
        sem_id = semget(key, N_SEMS, IPC_CREAT | 0666);

        if (sem_id == -1) {
            ERROR
            perror("Error in semget() in creating new sem set: ");
            exit(-1);
        }

        int resop = semctl(sem_id, 0, SETVAL, 1);
        if (resop < 0) {
            ERROR
            perror("Error in setting mutex: ");
            exit(-1);
        }

        return sem_id;

    } else if (sem_id > 0) {
        DBG(fprintf(stdout, "Attaching old semaphore set\n"))
        return sem_id;  
    } else {
        ERROR
        perror("Error in semget() in attaching old sem_set");
        exit(-1);
    }
    return -1;
}

void semdel(int key) {
    int sem_id = semget(key, N_SEMS, 0666);
    if (sem_id < 0) {
        ERROR
        perror("Error in semget() in deleting sem set: ");
    }

    int resop = semctl(sem_id, 0, IPC_RMID);
    if (resop == -1) {
        ERROR
         perror("Error in semctl() in deleting sem set: ");
    }
}

void shmdel(int key, int size) {
    int id = shmget(key, size * sizeof(void*), 0);
    if (id == -1) {
        ERROR
        perror("Erorr in shmget() in deleting shared memory: ");
    }

    int resop = shmctl (id , IPC_RMID , NULL);
    if (resop == -1) {
        ERROR
        perror("Error in shmctl() in deleting shared memoory: ");
    }
}

stack_t* attach_stack(key_t key, int size) {

    assert(size > 0);
    assert(key != -1);
    stack_t* stack = (stack_t*) calloc(1, sizeof(struct stack_t));
    assert(stack);
    int stack_size = size;

    DBG(fprintf(stdout, "Stack size: %d elements\n", stack_size))

    int sem_id = set_sem(key);
    assert(sem_id > 0);

    int id = shmget(key, stack_size * sizeof(void*), 0666);

    if (id > 0) { // existing stack
        //fprintf(stdout, "Stack exists, attaching the stack\n");

        stack->m_memory = (void*) shmat(id, NULL, 0);
        if (stack->m_memory == (void*) -1) {
            ERROR
            perror("Error in shmat() in attaching existing stack: ");
            exit(-1);
            return NULL;
        }
        stack->m_max_size = *((int*) (stack->m_memory));
        stack->m_cur_size = *((int*) (stack->m_memory + sizeof(void*)));

        return stack;
    } else if (errno == ENOENT) { // creating new stack
        //fprintf(stdout, "No stack exists, creating new stack\n");
        
        id = shmget(key, stack_size * sizeof(void*), IPC_CREAT | 0666);
        if (id == -1) {
            ERROR
            perror("Error in shmget() in creating new stack: ");
        }

        stack->m_memory = shmat(id, NULL, 0);

        if (stack->m_memory == (void*) -1) {
            perror("Error in shmat() in creating new stack: ");
            ERROR
            exit(-1);
            return NULL;
        }

        int max_size = size;
        int cur_size = 0;

        DBG((fprintf(stdout, "Copying max size %d and cur size %d to shared memory[0] = %p and memory[1] = %p\n", max_size, cur_size,
                                stack->m_memory, stack->m_memory + sizeof(void*))))
        memcpy((void*) stack->m_memory, &max_size, sizeof(int));
        memcpy((void*) stack->m_memory + sizeof(void*), &cur_size, sizeof(int));

        stack->m_max_size = *((int*) (stack->m_memory));
        stack->m_cur_size = *((int*) (stack->m_memory + sizeof(void*)));

        DBG(fprintf(stdout, "Stack max_size %d at %p stack cur_size %d at %p\n", stack->m_max_size,
                            stack->m_memory, stack->m_cur_size, stack->m_memory + sizeof(void*)))

        return stack;
    } else {
        ERROR
        perror( "Error in creating new stack: ");
        exit(-1);
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
        ERROR
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
        ERROR
        perror("Error in ftok() in marking for destruct: ");
    }

    struct sembuf mutex;
    mutex.sem_flg = SEM_UNDO;
    mutex.sem_num = 0;
    mutex.sem_op = -1;

    int sem_id = set_sem(key);

    semop(sem_id, &mutex, 1);

    stack->m_max_size = get_size(stack);

    int id = shmget(key, stack->m_max_size * sizeof(void*), 0666);
    if (id == -1) {
        ERROR
        perror("Error in shmget i marking for destruction: ");
    }

    struct shmid_ds buf;
    shmctl(id, IPC_STAT, &buf);

    //printf("Current attaches: %ld\n", buf.shm_nattch);

    if (buf.shm_nattch == 1) {
        int resop = shmctl(id, IPC_RMID, NULL);

        if (resop == -1) {
            ERROR
            perror("Error in removing shmem in markdsctrk: ");
        }
    }


    mutex.sem_flg = SEM_UNDO;
    mutex.sem_num = 0;
    mutex.sem_op = 1;

    semop(sem_id, &mutex, 1);

    return 0;

}

int push(stack_t* stack, void* val) {

    int key = ftok(sync_path, SYNC);
    if (key == -1) {
        //perror("Error in ftok(): ");
    }

    // creating or attaching semaphore set
    int sem_id = set_sem(key);

    struct sembuf* sops = (struct sembuf*) calloc(3, sizeof(struct sembuf));
    assert(sops);
    
    // set semaphore to SEM_UNDO so stack does not crash when any processes acquring this semaphore get terminated
    sops[0].sem_flg = SEM_UNDO;
    sops[1].sem_flg = SEM_UNDO;

    int sval = 0;
    int resop = 0;

    // semaphore numbers: semaphore - 0, empty - 1, full - 2
    // CHANGE: sops[0] - empty --, sops[1] = semaphore --

    sops[0].sem_num = 0;
    sops[0].sem_op = -1;
    sops[1].sem_num = 1;
    sops[1].sem_op = -1;

    DBG(fprintf(stdout, "Empty --, semafore --\n"))

    struct timespec timeout;
    set_wait(2, &timeout);

    semtimedop(sem_id, sops, 1, NULL);
     if (errno == EAGAIN) {
        //fprintf(stdout, "Timeout interval exceeded, operation aborted\n");
        errno = 0;
        return -1;
    }

    sval = semctl(sem_id, 1, GETVAL);
    DBG(fprintf(stdout, "empty value upon entering critical area: %d\n", sval))
    sval = semctl(sem_id, 0, GETVAL);
    DBG(fprintf(stdout, "Semaphore value on entering critical area :%d\n", sval))

    //DBG(fprintf(stdout, "Semaphore value after taking:%d\n", sval))
    if (resop == -1) {
        //perror("Error in semop(): ");
    }

    
    // update cur size

    stack->m_cur_size = *(( int*) (stack->m_memory +  sizeof(void*)) );

    if (stack->m_cur_size == stack->m_max_size) {
        sops[0].sem_num = 0;
        sops[0].sem_op = 1;
        sops[0].sem_flg = SEM_UNDO;
         semtimedop(sem_id, sops, 1, NULL);
        return -1;
    }

    memcpy((void*) (stack->m_memory + (stack->m_cur_size + 2) * sizeof(void*)), val, sizeof(void*));

    int new_cur_size = stack->m_cur_size + 1;
    stack->m_cur_size = new_cur_size;


    if (resop == -1) {
        //perror("Error in semop(): ");
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


    semtimedop(sem_id, sops, 1, NULL);

    if (resop == -1) {
        //perror("Error in semop(): ");
    }

    free(sops);
    return 0;
}


int pop(stack_t* stack, void** val) {

    int key = ftok(sync_path, SYNC);
    if (key == -1) {
        //perror("Error in ftok(): ");
    }

    // creating or attaching semaphore set
    int sem_id = set_sem(key);


    struct sembuf* sops = (struct sembuf*) calloc(3, sizeof(struct sembuf));
    assert(sops);


    // semaphore numbers: semaphore - 0, empty - 1, full - 2
    // CHANGE: sops[0] - full --, sops[1] = semaphore --

    sops[0].sem_num = 0;
    sops[0].sem_op = -1;
    sops[1].sem_num = 0;
    sops[1].sem_op = -1;

    sops[1].sem_flg = SEM_UNDO;
    sops[0].sem_flg = SEM_UNDO;

    int sval = 0;
    int resop = 0;


    struct timespec timeout;
    set_wait(1, &timeout);

    semtimedop(sem_id, sops, 1, NULL);
    if (errno == EAGAIN) {
        //fprintf(stdout, "Timeout interval exceeded, operation aborted\n");
        errno = 0;
        return -1;
    }

    sval = semctl(sem_id, 2, GETVAL);
    DBG(fprintf(stdout, "Full value after taking:%d\n", sval))
    sval = semctl(sem_id, 0, GETVAL);
    DBG(fprintf(stdout, "Semaphore value after taking :%d\n", sval))

    if (resop == -1) {
        //perror("Error in semop(): ");
    }

    //update cur size
    stack->m_cur_size = *(( int*) (stack->m_memory +  sizeof(void*)) );

    if (stack->m_cur_size == 0) {
        sops[0].sem_num = 0;
        sops[0].sem_op = 1;
        sops[0].sem_flg = SEM_UNDO;
        semtimedop(sem_id, sops, 1, NULL);
        return -1;
    }


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


    semtimedop(sem_id, sops, 1, NULL);

    sval = semctl(sem_id, 0, GETVAL);
    DBG(fprintf(stdout, "Semaphore value after taking (finished critical area):%d\n", sval))
    sval = semctl(sem_id, 1, GETVAL);
    DBG(fprintf(stdout, "Empty value after taking:%d\n", sval))

    if (resop == -1) {
        //perror("Error in semop(): ");
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
    return 0;
}
