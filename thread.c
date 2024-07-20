// Author: Stanislav Letaši, BUT FIT

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include "po.h"

// Funkcia simulujúca zákazníka
void customer(int id){

    srand(getpid() + id * time(NULL)); // Použité na generáciu náhodného času čakania a voľby úlohy

    write_out("Z %i: started\n", id); 
    usleep((rand()%(po_data->TC+1))); // Čakanie náhodnú dĺžku času pred vstúpením

    sem_wait(data_access); 

    if(po_data->open == 1) // Ak je pošta otvorená
    {
        int service = ((rand()%3) + 1); // Výber služby
        write_out("Z %i: entering office for a service %i\n", id, service);

        if(po_data->NO == 0) // Ak na pošte nie sú úradníci, proces zákazník končí
        {
            sem_post(data_access);  // Odblokovanie prístupu k dátam o pošte
            write_out("Z %i: going home\n", id);
            exit(0);
        }

        switch (service){ // Rozdelenie do fronty
        case 1:
            po_data->q1++;          // Fronta sa predĺži
            sem_post(data_access);  // Odblokovanie prístupu k dátam o pošte
            sem_wait(letter_q);     // Zaradenie sa do fronty
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
    }
    else
    {
        sem_post(data_access); // Odblokovanie prístupu k dátam o pošte
        write_out("Z %i: going home\n", id);
        exit(0);
    }

    write_out("Z %i: called by office worker\n", id);
    usleep(rand()%10001);
    write_out("Z %i: going home\n", id);

    exit(0);
}

// Funkcia simulujúca úradníka
void officer(int id){

    srand(getpid() + id * time(NULL)); // Použité na generáciu náhodného času čakania a voľby úlohy

    write_out("U %i: started\n", id);

    int task; // Premenná určujúca ktorú frontu úradník obslúži
    while(true)
    {
        sem_wait(data_access);

        if(po_data->q1 == 0 && po_data->q2 == 0 && po_data->q3 == 0 && po_data->open == 1) // Otvorená pošta, nikto nečaká, úradník ide na pauzu
        {
            write_out("U %i: taking break\n", id);
            sem_post(data_access);
            usleep(rand()%(po_data->TO+1));
            write_out("U %i: break finished\n", id);
            continue;
        }
        if(po_data->q1 == 0 && po_data->q2 == 0 && po_data->q3 == 0 && po_data->open == 0) // Zatvorená pošta, nikto nečaká, úradník ide domov
        {
            sem_post(data_access);
            write_out("U %i: going home\n", id);
            exit(0);
        }
        if(po_data->q1 > 0 || po_data->q2 > 0 || po_data->q3 > 0) // Čaká aspoň 1 zákazník, nezáleží či je pošta otvorená
        {
            task = (rand()%3) + 1; // Náhodne zvolená fronta

            for(int i=0; i<3;) // i reprezentuje počet skontrolovaných qeues
            {                  // aspoň jedna z front bude mať vždy aspoň 1 zákazníka, lebo úradník má v tomto momente data_access 
                if(task==4)    // task je v rozmedzí <1, 3>
                {
                    task=1;
                }

                switch (task){
                case 1:
                    if(po_data->q1 > 0)         // Fronta 1 nie je prázdna
                    {
                        sem_post(letter_q);     // Obslúženie zákazníka
                        write_out("U %i: serving a service of type %i\n", id, task);
                        po_data->q1--;          // Zmenšenie rady
                        sem_post(data_access);  // Odblokovanie prístupu k dátam o pošte
                        usleep(rand()%10001);      // Vykonávanie služby
                        write_out("U %i: service finished\n", id);
                        i = 3; // Ukončí for loop
                    }
                    else // Fronta je prázdna, skúsi či niekto čaká v ďalšej
                    {
                        task++; // Posunie sa na ďalšiu frontu
                        i++;    // Pripočítanie counteru skontrolovaných qeues
                    }
                    break;      // Ukončí switch
                       
                case 2:
                    if(po_data->q2 > 0)         // Fronta 2 nie je prázdna
                    {
                        sem_post(package_q);     // Obslúženie zákazníka
                        write_out("U %i: serving a service of type %i\n", id, task);
                        po_data->q2--;          // Zmenšenie rady
                        sem_post(data_access);  // Odblokovanie prístupu k dátam o pošte 
                        usleep(rand()%10001);      // Vykonávanie služby
                        write_out("U %i: service finished\n", id);
                        i = 3; // Ukončí for loop
                    }
                    else // Fronta je prázdna, skúsi či niekto čaká v ďalšej
                    {
                        task++; // Posunie sa na ďalšiu frontu
                        i++;    // Pripočítanie counteru skontrolovaných qeues
                    }
                    break;      // Ukončí switch

                case 3:
                    if(po_data->q3 > 0)         // Fronta nie je prázdna
                    {
                        sem_post(money_q);     // Obslúženie zákazníka
                        write_out("U %i: serving a service of type %i\n", id, task);
                        po_data->q3--;          // Zmenšenie rady
                        sem_post(data_access);  // Odblokovanie prístupu k dátam o pošte 
                        usleep(rand()%10001);      // Vykonávanie služby
                        write_out("U %i: service finished\n", id);
                        i = 3; // Ukončí for loop
                    }
                    else // Fronta je prázdna, skúsi či niekto čaká v ďalšej
                    {
                        task++; // Posunie sa na ďalšiu frontu        
                        i++;    // Pripočítanie counteru skontrolovaných qeues
                    }
                    break;      // Ukončí switch
                       
                } // Switch statement
            } // For loop na switch statement
        } // If - Čaká aspoň 1 zákazník, nezáleží či je pošta otvorená
    } // While loop procesu úradníka
} // Funkcia officer
