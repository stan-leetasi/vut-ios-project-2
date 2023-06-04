// proj2.c 
// Řešení IOS projekt2, 27.4.2023
// Autor: Stanislav Letaši, FIT

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/sem.h>

#include <pthread.h>

/**********************************
* Dátové štruktúry
**********************************/

// Dáta ktoré zdieľajú všetky procesy
typedef struct {
    int NZ;         // Počet zákazníkov
    int NU;         // Počet úradníkov
    int TZ;         // Maximální čas v milisekundách, po který zákazník po svém vytvoření čeká, než vejde na poštu
    int TU;         // Maximální délka přestávky úředníka v milisekundách
    int F;          // Maximální čas v milisekundách, po kterém je uzavřena pošta pro nově příchozí
    int q1;         // Dĺžka rady na listy
    int q2;         // Dĺžka rady na zásielky
    int q3;         // Dĺžka rady na peňažné operácie
    int line_num;   // Číslovanie riadkov pri výpise do proj2.out
    int open;       // Indikuje či je pošta otvorená alebo zatvorená
} shared_data;


/**********************************
* Globálne premenné
**********************************/

// Semafory
sem_t *letter_q = NULL;     // Rada s listami
sem_t *package_q = NULL;    // Rada s balíkmi
sem_t *money_q = NULL;      // Rada s penažnými operáciami
sem_t *data_access = NULL;  // Semafor ochraňujúci dátovú štruktúru shared_data, na vstupné argumenty a premennú line_num sa nepoužíva 
sem_t *file_access = NULL;  // Semafor ochraňujúci súbor proj2.out


shared_data *po_data = NULL; // Post-office data   
FILE *proj2out = NULL;       // Ukazateľ na súbor proj2.out 

/**********************************
* Definície funkcií
**********************************/

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
    if(letter_q != NULL)
    semap_remove(letter_q);

    if(package_q != NULL)
    semap_remove(package_q);

    if(money_q != NULL)
    semap_remove(money_q);

    if(data_access != NULL)
    semap_remove(data_access);

    if(file_access != NULL)
    semap_remove(file_access);

    if(po_data != NULL)
    munmap(po_data, sizeof(shared_data));
    
    if(proj2out != NULL)
    fclose(proj2out);

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

// Funkcia simulujúca zákazníka
void customer(int id){

    srand(getpid() + id * time(NULL)); // Použité na generáciu náhodného času čakania a voľby úlohy

    write_out("Z %i: started\n", id); 
    usleep((rand()%(po_data->TZ+1))); // Čakanie náhodnú dĺžku času pred vstúpením

    sem_wait(data_access); 

    if(po_data->open == 1) // Ak je pošta otvorená
    {
        int service = ((rand()%3) + 1); // Výber služby
        write_out("Z %i: entering office for a service %i\n", id, service);

        if(po_data->NU == 0) // Ak na pošte nie sú úradníci, proces zákazník končí
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
            usleep(rand()%(po_data->TU+1));
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
    po_data->NZ = conv;
    conv = strtol(argv[2], &c, 10);
    po_data->NU = conv;
    conv = strtol(argv[3], &c, 10);
    po_data->TZ = conv * 1000;     // Prevedenie na mikrosekundy
    conv = strtol(argv[4], &c, 10);
    po_data->TU = conv * 1000;     // Prevedenie na mikrosekundy
    conv = strtol(argv[5], &c, 10);
    po_data->F = conv * 1000;      // Prevedenie na mikrosekundy

    po_data->line_num = 1;  // Číslovanie riadkov začína od 1
    po_data->open = 1;      // Pošta je otvorená
    po_data->q1 = 0;        // Prázdne fronty
    po_data->q2 = 0;
    po_data->q3 = 0;

    // Overenie správnosti rozsahu vstupných argumentov
    if(po_data->NZ < 0 ){
        fprintf(stderr, "1. argument presiahol limity NZ>=0\n");
        remove_all();
        exit(1);
    }
    if(po_data->NU <= 0 ){
        fprintf(stderr, "2. argument presiahol limity NU>0\n");
        remove_all();
        exit(1);
    }
    if(po_data->TZ < 0 || po_data->TZ > 10000*1000){
        fprintf(stderr, "3. argument presiahol limity 0<=TZ<=10000\n");
        remove_all();
        exit(1);
    }
    if(po_data->TU < 0 || po_data->TU > 100*1000){
        fprintf(stderr, "4. argument presiahol limity 0<=TU<=100\n");
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

    pid_t child_pids[po_data->NU + po_data->NZ]; // Array s pid všetkých child procesov

   
    pid_t pid;
    // Vytvorenie procesov zákazníkov
    for (int i = 1; i <= po_data->NZ; i++)
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
    for (int i = 1; i <= po_data->NU; i++)
    {
        pid = fork();
        if (pid < 0) // Ak fork zlyhá
        {
            for (int j = 0; j < ((po_data->NZ) - 1 + i ); j++) // Zabitie všetkých bežiacich child procesov
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
            child_pids[po_data->NZ + i - 1] = pid; // Uloženie pid child procesu do arrayu
        }
    }

    usleep(((rand() % (1 + po_data->F/2)) + po_data->F/2)); //Čeká pomocí volání usleep náhodný čas v intervalu <F/2 , F>

    sem_wait(data_access);
    write_out("closing\n");
    po_data->open = 0;  // Zatvorenie pošty
    sem_post(data_access);

    // Čaká sa na ukončenie všetkých procesov
    for (int i = 0; i < (po_data->NZ + po_data->NU); i++)
    {
        waitpid(child_pids[i], NULL, 0);
    }

    remove_all(); // "Upratanie" semaforov a alokovanej pamäte

    exit(0);
}
