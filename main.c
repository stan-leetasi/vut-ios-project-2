// IOS project 2 solution, 27.4.2023
// Author: Stanislav Letaši, BUT FIT
// Simulation of a post office using threads

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "po.h"

// Global variables
shared_data *po_data = NULL;
sem_t *letter_q = NULL;
sem_t *package_q = NULL;
sem_t *money_q = NULL;
sem_t *data_access = NULL;
sem_t *file_access = NULL;
FILE *proj2out = NULL;


int main(int argc, char *argv[]){

    po_data = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (po_data == MAP_FAILED) {
        fprintf(stderr, "ERR: Allocation of shared memory failed\n");
        exit(1);
    }

    arg_operations(argc, argv);

    srand(time(NULL));    

    proj2out = fopen("proj2.out", "w");

    if (proj2out == NULL)
    {
        fprintf(stderr, "ERR: Failed to open output file\n");
        exit(1);
    }

    letter_q = semap_init(0);
    package_q = semap_init(0);
    money_q = semap_init(0);
    data_access = semap_init(1);
    file_access = semap_init(1);

    if (letter_q == NULL || package_q == NULL || money_q == NULL || data_access == NULL)
    {
        remove_all();
        fprintf(stderr, "ERR: Failed to initialize a semaphore\n");
        exit(1);
    }

    pid_t child_pids[po_data->NO + po_data->NC];
    pid_t pid;

    // create customer processes
    for (int i = 1; i <= po_data->NC; i++)
    {
        pid = fork();
        if (pid < 0) 
        {
            for (int j = 0; j < (i - 1); j++) // kill all child processes
                kill(child_pids[j], SIGTERM);
            

            remove_all();
            fprintf(stderr, "ERR: An error occured during customer processes creation\n");
            exit(1);
        }

        else if (pid == 0)
            customer(i);
        
        else
            child_pids[i-1] = pid; // save pid into array
        
    }

    // create officer processes
    for (int i = 1; i <= po_data->NO; i++)
    {
        pid = fork();
        if (pid < 0)
        {
            for (int j = 0; j < ((po_data->NC) - 1 + i ); j++)
                kill(child_pids[j], SIGTERM);
            
            remove_all();
            fprintf(stderr, "ERR: An error occured during officer processes creation\n");
            exit(1);
        } 

        else if (pid == 0)
            officer(i);
        
        else
            child_pids[po_data->NC + i - 1] = pid;
    }

    usleep(((rand() % (1 + po_data->F/2)) + po_data->F/2)); // wait for a random amount of time in interval <F/2 , F>

    sem_wait(data_access);
    write_out("closing\n");
    po_data->open = 0;
    sem_post(data_access);

    // wait for all processes to end
    for (int i = 0; i < (po_data->NC + po_data->NO); i++)
        waitpid(child_pids[i], NULL, 0);
    
    remove_all();

    exit(0);
}
