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
#include <ctype.h>
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

    if(argc < 6) // Overenie či boli zadané všetky vstupné argumenty
    {
        fprintf(stderr, "Neboli zadane vsetky vstupne argumenty\n");
        exit(1);
    }
    for(int i = 1; i < argc; i++) // Overenie či sú vstupné argumenty valídne čísla
    {
        if (!isdigit(*argv[i]))
        {
            fprintf(stderr, "Argument %s nie je validne cislo\n", argv[i]);
            exit(1);
        }
    }

    // Alokácia zdieľanej pamäte pre štruktúru so všetkými potrebnými dátami ohľadom pošty
    po_data = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (po_data == MAP_FAILED) {
        fprintf(stderr, "Zlyhala alokacia zdielanej pamate pre data\n");
        exit(1);
    }

    long conv;          // Dočasná premenná použitá na uloženi výstupu pri prekonvertovaní vstupných argumentov na int
    char *c;            // Dočasná premenná použitá na prekonvertovanie vstupných argumentov na int
    srand(time(NULL));  // Generácia náhodných čísel

    // Uloženie vstupných argumentov do zdieľanej štruktúry (bezpečný spôsob konverzie oproti funkcii atoi)
    conv = strtol(argv[1], &c, 10); // Pretypovanie programového argumentu na int
    po_data->NC = conv;
    conv = strtol(argv[2], &c, 10);
    po_data->NO = conv;
    conv = strtol(argv[3], &c, 10);
    po_data->TC = conv * 1000;     // Prevedenie na mikrosekundy
    conv = strtol(argv[4], &c, 10);
    po_data->TO = conv * 1000;     // Prevedenie na mikrosekundy
    conv = strtol(argv[5], &c, 10);
    po_data->F = conv * 1000;      // Prevedenie na mikrosekundy

    po_data->line_num = 1;  // Číslovanie riadkov začína od 1
    po_data->open = 1;      // Pošta je otvorená
    po_data->q1 = 0;        // Prázdne fronty
    po_data->q2 = 0;
    po_data->q3 = 0;

    // Overenie správnosti rozsahu vstupných argumentov
    if(po_data->NC < 0 ){
        fprintf(stderr, "1. argument presiahol limity NC>=0\n");
        remove_all();
        exit(1);
    }
    if(po_data->NO <= 0 ){
        fprintf(stderr, "2. argument presiahol limity NO>0\n");
        remove_all();
        exit(1);
    }
    if(po_data->TC < 0 || po_data->TC > 10000*1000){
        fprintf(stderr, "3. argument presiahol limity 0<=TC<=10000\n");
        remove_all();
        exit(1);
    }
    if(po_data->TO < 0 || po_data->TO > 100*1000){
        fprintf(stderr, "4. argument presiahol limity 0<=TO<=100\n");
        remove_all();
        exit(1);
    }
    if(po_data->F < 0 || po_data->F > 10000*1000){
        fprintf(stderr, "5. argument presiahol limity 0<=F<=10000\n");
        remove_all();
        exit(1);
    }

    proj2out = fopen("proj2.out", "w");

    if (proj2out == NULL) // Ak zlyhalo otvorenie výstupného súboru
    {
        fprintf(stderr, "Otvorenie vystupneho suboru zlyhalo\n");
        exit(1);
    }


    // Inicializácia semaforov
    letter_q = semap_init(0);
    package_q = semap_init(0);
    money_q = semap_init(0);
    data_access = semap_init(1);
    file_access = semap_init(1);

    if (letter_q == NULL || package_q == NULL || money_q == NULL || data_access == NULL) // Overenie úspešnej inicializácie semaforov
    {
        remove_all();
        fprintf(stderr, "Nastala chyba pri inicializacii niektoreho zo semaforov\n");
        exit(1);
    }

    pid_t child_pids[po_data->NO + po_data->NC]; // Array s pid všetkých child procesov

   
    pid_t pid;
    // Vytvorenie procesov zákazníkov
    for (int i = 1; i <= po_data->NC; i++)
    {
        pid = fork();
        if (pid < 0) 
        {
            for (int j = 0; j < (i - 1); j++) // Zabitie všetkých bežiacich child procesov
            {
                kill(child_pids[j], SIGTERM);
            }

            remove_all(); // Vyčistenie zdieľanej a alokovanej pamäte
            fprintf(stderr, "Nastala chyba pri vytvoreni procesov zakaznikov\n");
            exit(1);
        }

        else if (pid == 0) // Child process zákazníka
        {
            customer(i);
        }
        else
        {
            child_pids[i-1] = pid; // Uloženie pid child procesu do arrayu
        }
    }

    // Vytvorenie procesov úradníkov
    for (int i = 1; i <= po_data->NO; i++)
    {
        pid = fork();
        if (pid < 0) // Ak fork zlyhá
        {
            for (int j = 0; j < ((po_data->NC) - 1 + i ); j++) // Zabitie všetkých bežiacich child procesov
            {
                kill(child_pids[j], SIGTERM);
            }

            remove_all(); // Vyčistenie zdieľanej a alokovanej pamäte
            fprintf(stderr, "Nastala chyba pri vytvoreni procesov uradnikov\n");
            exit(1);
        } 

        else if (pid == 0) // Child process úradníka
        {
            officer(i);
        }
        else
        {
            child_pids[po_data->NC + i - 1] = pid; // Uloženie pid child procesu do arrayu
        }
    }

    usleep(((rand() % (1 + po_data->F/2)) + po_data->F/2)); //Čeká pomocí volání usleep náhodný čas v intervalu <F/2 , F>

    sem_wait(data_access);
    write_out("closing\n");
    po_data->open = 0;  // Zatvorenie pošty
    sem_post(data_access);

    // Čaká sa na ukončenie všetkých procesov
    for (int i = 0; i < (po_data->NC + po_data->NO); i++)
    {
        waitpid(child_pids[i], NULL, 0);
    }

    remove_all(); // "Upratanie" semaforov a alokovanej pamäte

    exit(0);
}
