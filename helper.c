// Helper functions
// Author: Stanislav Letaši, BUT FIT

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "po.h"

// Inicializuje semafor, vracia ukazateľ na inicializovaný semafor
sem_t *semap_init(int value){

    // Alokácia zdieľanej pamäti pre semafor
    sem_t *semaphore;
    semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if(semaphore == MAP_FAILED) // Zlyhanie alokácia zdieľanej pamäte
    {
        return NULL;
    }
    // Inicializácia semaforu na zadanú hodnotu
    if (sem_init(semaphore, 1, value) == -1) 
    {
        munmap(semaphore, sizeof(sem_t)); // Zlyhanie alokácia zdieľanej pamäte pre semafor
        return NULL;
    }

    return semaphore; // Ukazateľ na semafor
}

// "Upratanie" semaforu pred ukončením programu
void semap_remove(sem_t * semaphore){

    munmap(semaphore, sizeof(sem_t)); // Odstránenie alokovanej pamäte
    
    sem_destroy(semaphore); // Odstránenie semaforu
}

// "Upratanie" všetkých semaforov a alokovanej pamäte pred ukončením programu
void remove_all(){
    if(letter_q != NULL) semap_remove(letter_q);

    if(package_q != NULL) semap_remove(package_q);

    if(money_q != NULL) semap_remove(money_q);

    if(data_access != NULL) semap_remove(data_access);

    if(file_access != NULL) semap_remove(file_access);

    if(po_data != NULL) munmap(po_data, sizeof(shared_data));
    
    if(proj2out != NULL) fclose(proj2out);
}

// Funkcia na zapisovanie do proj2.out, pri každom zápise prevedie fflush aby bol výstup v správnom poradí (buffer)
void write_out (const char * format, ...){

    sem_wait(file_access); // "Zarezervovanie" činnosti zápisu do súboru

    va_list args;
    va_start(args, format);

    fprintf(proj2out, "%d: ", po_data->line_num);
    vfprintf (proj2out, format, args);

    po_data->line_num++;

    fflush(proj2out);
    va_end (args);
    sem_post(file_access); // Uvoľnenie činnosti zápisu do súboru
}