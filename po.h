// Author: Stanislav Letaši, BUT FIT

#ifndef PO_H
#define PO_H

#include <semaphore.h>

/**********************************/
// Data structures
typedef struct {
    int NC;         // number of customers
    int NO;         // number of officers
    int TC;         // max time in ms that a customer waits to enter the post office
    int TO;         // max time in ms of the officer's break
    int F;          // max time in ms that the post office will stay opened
    int q1;         // length of queue for letters
    int q2;         // length of queue for packages
    int q3;         // length of queue for financial operations
    int line_num;   // numbering of lines for logging purposes
    int open;       // indicates post office state
} shared_data;      // data shared by all threads

/**********************************/
// Global variables
extern sem_t *letter_q;     // letter queue
extern sem_t *package_q;    // package queue
extern sem_t *money_q;      // financial operations queue
extern sem_t *data_access;  // semaphore for shared_data
extern sem_t *file_access;  

extern shared_data *po_data; // post office data   
extern FILE *proj2out; 

/**********************************/
// Functions 
sem_t *semap_init(int value);
void semap_remove(sem_t * semaphore);
void remove_all();
void write_out (const char * format, ...);
void customer(int id);
void officer(int id);
void arg_operations(int argc, char *argv[]);

#endif