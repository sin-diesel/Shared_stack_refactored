#include "stack.h"


const char* sync_path = "/home/stanislav/Documents/MIPT/3rd_semester/Computer_technologies";


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

    int sem_id = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666 );
    if (sem_id > 0 && errno != EEXIST) { // set is new, this setting the initial value of semaphore to 1

        DBG(fprintf(stdout, "Creating new semaphore set\n"))

        sem_id = semget(key, 1, IPC_CREAT | 0666);
        if (sem_id == -1) {
            perror("Error in semget(): ");
        }

        int resop  = semctl(sem_id, 0, SETVAL, 1);
        DBG(fprintf(stdout, "Setting semaphore to 1\n"))
        if (resop == -1) {
            perror("Error in set_sem_set(): ");
        }

    } else {
        DBG(fprintf(stdout, "Attaching old semaphore set\n"))
        sem_id = semget(key, 1, 0666);
        if (sem_id == -1) {
            perror("Error in semget(): ");
        }
    }
    return sem_id;
}

void semdel(int key) {
    int sem_id = semget(key, 1, 0666);
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
    int size = stack->m_max_size;

    return size;
}

int get_count(stack_t* stack) {
    assert(stack != NULL);
    int count = stack->m_cur_size;
    
    return count;
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

    struct sembuf semafor = {0, -1, 0};
    semafor.sem_flg = SEM_UNDO;

    // taking semaphore
    int sval = semctl(sem_id, 0, GETVAL);
    DBG(fprintf(stdout, "Semaphore value before taking:%d\n", sval))
    int resop = semop(sem_id, &semafor, 1);
    sval = semctl(sem_id, 0, GETVAL);
    DBG(fprintf(stdout, "Semaphore value after taking:%d\n", sval))
    if (resop == -1) {
        perror("Error in semop(): ");
    }

    //DBG(stack_print(stack));

    //stack->m_memory[stack->m_cur_size + 2] = val;
    
    //DBG(fprintf(stdout, "Val: %d\n", * ((int*) val)))
    
    // update cur size

    stack->m_cur_size = *(( int*) (stack->m_memory +  sizeof(void*)) );

    memcpy((void*) (stack->m_memory + (stack->m_cur_size + 2) * sizeof(void*)), val, sizeof(void*));
    //DBG(fprintf(stdout, "Stack [%d]: %d\n", stack->m_cur_size, *( (int*) (stack->m_memory + (stack->m_cur_size + 2) * sizeof(void*)) )))

    int new_cur_size = stack->m_cur_size + 1;
    stack->m_cur_size = new_cur_size;
    memcpy((void*) (stack->m_memory + sizeof(void*)), &new_cur_size, sizeof(int));


    // releasing semaphore
    semafor.sem_op = 1;
    resop = semop(sem_id, &semafor, 1);
    if (resop == -1) {
        perror("Error in semop(): ");
    }

    sval = semctl(sem_id, 0, GETVAL);
    //DBG(fprintf(stdout, "Semaphore value:%d\n", sval))

    return 0;
}
