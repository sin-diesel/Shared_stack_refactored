

#include "stack.h"


extern const char* sync_path;

int main (int argc, char** argv) {

    int stack_size = 2 << 18;

    int key = ftok(sync_path, SYNC);
    if (key == -1) {
        perror("Error in ftok(): ");
        exit(-1);
    }

    /* Deleting segment */

    fprintf(stdout, "Deleting segment\n");
    shmdel(key, stack_size);
    semdel(key);
    int data[] = {1, 4, 3, 0, 9};

    stack_t* stack = attach_stack(key, stack_size);

    void** val = NULL;

    int pid = getpid();
    if (setgid(pid) < 0) {
        perror("errro setting gid");
    }

    printf("parent pid: %d\n", pid);
    printf("parent pgrp: %d\n", getpgid(pid));


    for (int i = 0; i < 12; ++i) {
        int resop = fork();
        if (resop == -1) {
            perror("Error while forking\n");
        }
    }

    stack = attach_stack(key, stack_size);

    push(stack, &data[0]);
    push(stack, &data[0]);
    pop(stack, val);
    pop(stack, val);

    int resop = 0;

    //printf("Killing pgrp: %d\n", getpgid(getpid())); 

    if (pid == getpid()) {
        setgid(pid + 1);   
        resop = killpg(pid , SIGKILL);
        if (resop < 0) {
            perror("error in killpg: ");
        }

        sleep(2);
        push(stack, &data[0]);
        pop(stack, val);
        push(stack, &data[0]);
        pop(stack, val);
        int count = get_count(stack);
        printf("Stack count: %d\n", count);
        int max_size = get_size(stack);
        printf("Stack max_size: %d\n", max_size);

        mark_destruct(stack);
    }
    detach_stack(stack);
    mark_destruct(stack);

    return 0;
}