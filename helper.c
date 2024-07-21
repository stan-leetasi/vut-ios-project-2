// Helper functions
// Author: Stanislav Letaši, BUT FIT

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctype.h>
#include "po.h"

/**
 * Initializes a semaphore, returns pointer to the semaphore
 */
sem_t *semap_init(int value){

    sem_t *semaphore;
    semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if(semaphore == MAP_FAILED)
        return NULL;
    
    if (sem_init(semaphore, 1, value) == -1) 
    {
        munmap(semaphore, sizeof(sem_t));
        return NULL;
    }

    return semaphore;
}


void semap_remove(sem_t * semaphore){

    munmap(semaphore, sizeof(sem_t));
    sem_destroy(semaphore);
}

/**
 * Clean up all semaphores before program termination
 */
void remove_all(){
    if(po_data != NULL) munmap(po_data, sizeof(shared_data));

    if(letter_q != NULL) semap_remove(letter_q);

    if(package_q != NULL) semap_remove(package_q);

    if(money_q != NULL) semap_remove(money_q);

    if(data_access != NULL) semap_remove(data_access);

    if(file_access != NULL) semap_remove(file_access);
    
    if(proj2out != NULL) fclose(proj2out);
}

/**
 * Writes program log into proj2.out
 */
void write_out (const char * format, ...){

    sem_wait(file_access);

    va_list args;
    va_start(args, format);

    fprintf(proj2out, "%d: ", po_data->line_num);
    vfprintf (proj2out, format, args);

    po_data->line_num++;

    fflush(proj2out);
    va_end (args);
    sem_post(file_access);
}

/**
 * Operations with input arguments
 */
void arg_operations(int argc, char *argv[]){
    if(argc < 6)
    {
        fprintf(stderr, "ERR: Missing some input arguments\n");
        exit(1);
    }
    for(int i = 1; i < argc; i++)
    {
        if (!isdigit(*argv[i]))
        {
            fprintf(stderr, "ERR: Argument %s is not a valid number\n", argv[i]);
            exit(1);
        }
    }

    char *c;

    // convert arguments into integers
    po_data->NC = strtol(argv[1], &c, 10);
    po_data->NO = strtol(argv[2], &c, 10);
    po_data->TC = strtol(argv[3], &c, 10) * 1000;
    po_data->TO = strtol(argv[4], &c, 10)* 1000;
    po_data->F = strtol(argv[5], &c, 10) * 1000;

    po_data->line_num = 1;
    po_data->open = 1;      
    po_data->q1 = 0;        
    po_data->q2 = 0;
    po_data->q3 = 0;

    // check if input arguments have correct values
    if(po_data->NC < 0 ){
        fprintf(stderr, "ERR: 1. argument out of range NC>=0\n");
        munmap(po_data, sizeof(shared_data));
        exit(1);
    }
    if(po_data->NO <= 0 ){
        fprintf(stderr, "ERR: 2. argument out of range NO>0\n");
        munmap(po_data, sizeof(shared_data));
        exit(1);
    }
    if(po_data->TC < 0 || po_data->TC > 10000*1000){
        fprintf(stderr, "ERR: 3. argument out of range 0<=TC<=10000\n");
        munmap(po_data, sizeof(shared_data));
        exit(1);
    }
    if(po_data->TO < 0 || po_data->TO > 100*1000){
        fprintf(stderr, "ERR: 4. argument out of range 0<=TO<=100\n");
        munmap(po_data, sizeof(shared_data));
        exit(1);
    }
    if(po_data->F < 0 || po_data->F > 10000*1000){
        fprintf(stderr, "ERR: 5. argument out of range 0<=F<=10000\n");
        munmap(po_data, sizeof(shared_data));
        exit(1);
    }
}