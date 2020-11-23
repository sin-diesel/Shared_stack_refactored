#include "stack.h"

extern const char* sync_path;

#define SLEEP_T 1

#define TEST_PUSH(a)   sleep(SLEEP_T);          \
                       push(stack, &data[a]);   \
                       DBG(stack_print(stack))  \

#define TEST_POP(val)  sleep(SLEEP_T);          \
                       pop(stack, val);   \
                       DBG(stack_print(stack))  \

//stack_t* stack;
//#define CLEAR

int main (int argc, char** argv) {

    int stack_size = 20;

    int key = ftok(sync_path, SYNC);
    if (key == -1) {
        perror("Error in ftok(): ");
    }

    if (argc == 2) {
        if (strcmp(argv[1], "-c") == 0) {
            fprintf(stdout, "Deleting segment\n");
            shmdel(key, stack_size);
            semdel(key);
            exit(0);
        }
    }

    /* Deleting segment */

    #ifdef CLEAR
    fprintf(stdout, "Deleting segment\n");
    shmdel(key, stack_size);
    semdel(key);
    exit(0);
    #else

    stack_t* stack = attach_stack(key, stack_size);
    int data[] = {1, 4, 3, 0, 9};
    
    int size = get_size(stack);
    DBG(fprintf(stdout, "Stack maximum size: %d\n", size))

    int cur_size = get_count(stack);
    DBG(fprintf(stdout, "Stack current size: %d\n", cur_size))


    void** val;

    for (int i = 0; i < 3; ++i) {
        int resop = fork();
        if (resop == -1) {
            perror("Error while forking\n");
        }
    }

    // TEST_PUSH(0);
    // TEST_PUSH(0);
    // TEST_PUSH(0);
    // TEST_PUSH(0);
    // TEST_PUSH(0);
    // TEST_PUSH(0);
    // TEST_POP(val)
    // TEST_POP(val)
    // TEST_POP(val)
    // TEST_POP(val)
    // TEST_POP(val)
    // TEST_POP(val)
    

    // 1 - push, 0 = pop
    // while (1) {
    //     int op = -1;
    //     fprintf(stdout, "Enter cmd: \n");
    //     int res = fscanf(stdin, "%d", &op);
    //     assert(res == 1);
    //     if (op == 0) {
    //         TEST_POP(val);
    //     } else if (op == 1) {
    //         TEST_PUSH(0);
    //     }
    // }
    // pop(stack, val);
    // DBG(stack_print(stack))
    // sleep(SLEEP_T);
    // pop(stack, val);
    // DBG(stack_print(stack))


    DBG(fprintf(stdout, "Stack pointer before detaching: %p\n", stack))
    detach_stack(stack);

    DBG(fprintf(stdout, "Stack pointer after detaching: %p\n", stack))

    mark_destruct(stack);

    //sleep(5);

    #endif

}