#include "stack.h"

extern const char* sync_path;

#define SLEEP_T 1

#define TEST_PUSH(a)  push(stack, &data[a]);   \

#define TEST_POP(val) pop(stack, val);   \

int main (int argc, char** argv) {

    int stack_size = 2 << 18;

    int key = ftok(sync_path, SYNC);
    if (key == -1) {
        //perror("Error in ftok(): ");
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

    //stack_t* stack = attach_stack(key, stack_size);
    int data[] = {1, 4, 3, 0, 9};
    
    //int size = get_size(stack);
    //DBG(fprintf(stdout, "Stack maximum size: %d\n", size))

    //int cur_size = get_count(stack);
    //DBG(fprintf(stdout, "Stack current size: %d\n", cur_size))


    void** val;

    int pid = getpid();
    setgid(pid);


    for (int i = 0; i < 12; ++i) {
        int resop = fork();
        if (resop == -1) {
            //perror("Error while forking\n");
        }
    }
    stack_t* stack = attach_stack(key, stack_size);

    TEST_PUSH(0);
    TEST_PUSH(0);
    TEST_PUSH(0);
    TEST_PUSH(0);
    TEST_PUSH(0);
    TEST_PUSH(0);
    TEST_POP(val)
    TEST_POP(val)
    TEST_POP(val)
    TEST_POP(val)
    TEST_POP(val)
    //TEST_POP(val)

    if (getpid() == pid) {
        setgid(pid + 1);
        killpg(pid, SIGKILL);

        sleep(3);
        TEST_PUSH(0);
        TEST_POP(val);
        TEST_PUSH(0);
        TEST_POP(val);
        int count = get_count(stack);
        printf("Stack count: %d\n", count);
    }

    


    DBG(fprintf(stdout, "Stack pointer before detaching: %p\n", stack))
    detach_stack(stack);

    DBG(fprintf(stdout, "Stack pointer after detaching: %p\n", stack))

    mark_destruct(stack);

    #endif

}