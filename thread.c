// Author: Stanislav Letaši, BUT FIT

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include "po.h"

/**
 * Simulates behavior of a customer
 */
void customer(int id){

    srand(getpid() + id * time(NULL));
    write_out("Z %i: started\n", id); 
    usleep((rand()%(po_data->TC+1))); // wait before entering post office

    sem_wait(data_access);

    if(po_data->open == 1)
    {
        int service = ((rand()%3) + 1);
        write_out("Z %i: entering office for service no. %i\n", id, service);

        if(po_data->NO == 0) // no officers in post office
        {
            sem_post(data_access);
            write_out("Z %i: going home\n", id);
            exit(0);
        }

        switch (service){ // pick appropriate queue
        case 1:
            po_data->q1++;
            sem_post(data_access);  
            sem_wait(letter_q);     // enter queue
            break;
        
        case 2:
            po_data->q2++;
            sem_post(data_access);
            sem_wait(package_q);
            break;

        case 3:
            po_data->q3++;
            sem_post(data_access);
            sem_wait(money_q);
            break;
        }
    } // If - post office is open
    else // post office is closed
    {
        sem_post(data_access);
        write_out("Z %i: going home\n", id);
        exit(0);
    }

    write_out("Z %i: called by office worker\n", id);
    usleep(rand()%10001); // time required for the operation to finish
    write_out("Z %i: going home\n", id);

    exit(0);
}

/**
 * Simulates behaviour of an officer
 */
void officer(int id){

    srand(getpid() + id * time(NULL)); 
    write_out("U %i: started\n", id);

    int task; // represents which queue the officer will handle
    while(true)
    {
        sem_wait(data_access);

        // post office opened, no one is waiting, officer goes for a break
        if(po_data->q1 == 0 && po_data->q2 == 0 && po_data->q3 == 0 && po_data->open == 1) 
        {
            write_out("U %i: taking break\n", id);
            sem_post(data_access);
            usleep(rand()%(po_data->TO+1));
            write_out("U %i: break finished\n", id);
            continue;
        }
        // post office closed, no one is waiting, officer goes home
        if(po_data->q1 == 0 && po_data->q2 == 0 && po_data->q3 == 0 && po_data->open == 0)
        {
            sem_post(data_access);
            write_out("U %i: going home\n", id);
            exit(0);
        }
        // at least one customer is waiting
        if(po_data->q1 > 0 || po_data->q2 > 0 || po_data->q3 > 0)
        {
            task = (rand()%3) + 1; // choose queue

            for(int i=0; i<3; i++) // i represents the number of checked queues
            {
                task = task % 3 + 1;

                switch (task){
                case 1:
                    if(po_data->q1 > 0) // queue not empty
                    {
                        sem_post(letter_q);
                        write_out("U %i: serving a service of type %i\n", id, task);
                        po_data->q1--;          
                        break;
                    }
                    continue;
                       
                case 2:
                    if(po_data->q2 > 0)
                    {
                        sem_post(package_q);
                        write_out("U %i: serving a service of type %i\n", id, task);
                        po_data->q2--;
                        break;
                    }
                    continue;

                case 3:
                    if(po_data->q3 > 0)
                    {
                        sem_post(money_q);
                        write_out("U %i: serving a service of type %i\n", id, task);
                        po_data->q3--;
                        break;
                    }
                    continue;
                } // switch statement

                sem_post(data_access);  
                usleep(rand()%10001); // process task
                write_out("U %i: service finished\n", id);
                break;
            } // for loop
        } // If - at least one customer is waiting in queue
    } // while loop of the officer process
}
