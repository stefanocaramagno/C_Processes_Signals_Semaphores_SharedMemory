/*
Scrivere un programma in C su Linux in cui un processo principale (P0) genera un processo figlio P1.
P0 e P1 condividono tramite shared memory due variabili intere (base e altezza) che rappresentano, rispettivamente, 
la lunghezza della base e dell'altezza di un triangolo.

P0, ciclicamente e per 4 volte, 
	- chiede all’utente di inserire due interi da tastiera (base e altezza)
	- attende che P1 abbia calcolato l'area del triangolo
Dopo le 4 iterazioni, P0 
	- attende la terminazione del processo figlio
	- elimina shared memory e semafori
	- termina esso stesso
	
Il processo P1, ciclicamente, per 4 volte
	- attende che P0 abbia aggiornato le variabili condivise (l’attesa deve essere implementata utilizzando i semafori)
	- calcola l’area del triangolo e la stampa a video
Dopo 4 iterazioni P1 termina.

Utilizzare il meccanismo dei semafori sia per sincronizzare i processi e sia per regolare l’accesso in sezione critica.
*/

// LIBRERIE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <string.h>
#include <time.h>
#include <signal.h>

// PROTOTIPI
int sem_down(int sem_id); // Si Occupa di ottonere una Risorsa controllata dal Semaforo
int sem_up(int sem_id); // Si Occupa di rilasciare una Risorsa controllata dal Semaforo
int sem_set(int semid, int val); // Si Occupa di Inizializzare il Semaforo
int sem_del(int semid); // Si Occupa di Eliminare il Semaforo

// STRUTTURA SEMAFORO
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
};

// STRUTTURA DATI
struct dati{
	int base;
	int altezza;
};

// COSTANTI
#define KEY_SEM (key_t) 9876 // Chiave per la Gestione del Semaforo
#define KEY_SHM (key_t) 9875 // Chiave per la Gestione della Shared Memory
#define NUM_CYCLES 4

// MAIN
int main(void){
	int sem_id; // Identificativo del Semaforo che andremo a Creare
	int shm_id; // Identificativo della Shared Memory che andremo a Creare
	
	int result; // Variabile per il Controllo di Possibili Errori	
	
	struct dati *data;
	pid_t p1;
	int area_triangolo;
	
	// Creazione del Semaforo
	sem_id = semget(KEY_SEM, 1, 0666 | IPC_CREAT); 
	if(sem_id == -1){
    	fprintf(stderr, "Errore di Creazione del Semaforo\n");
		exit(EXIT_FAILURE);	
	}
	
    // Inizializzazione del Semaforo
    result = sem_set(sem_id, 1);
    if(result == -1){
    	fprintf(stderr, "Errore nell'Inizializzazione del Semaforo\n");
    	exit(EXIT_FAILURE);
	}
	
    // Creazione della Shared Memory
    shm_id = shmget(KEY_SHM, sizeof(int) * 100, 0666 | IPC_CREAT);
	if(shm_id == -1){
    	fprintf(stderr, "Errore di Creazione del Shared Memory\n");
		exit(EXIT_FAILURE);			
	}	
	
	// Si aggancia la Shared Memory con lo spazio degli Indirizzi di Memoria del Processo Padre
	data = (struct dati *)shmat(shm_id, NULL, 0);
	
	
	// Fulcro del Programma
	int contatore = 0;
	p1 = fork();
	switch(p1){
		case -1:
			break;
		case 0:
			while(1){
				result = sem_down(sem_id);
				if(result == -1){
					fprintf(stderr, "Errore nell'ottenere la risorsa controllata dal Semaforo");
					exit(EXIT_FAILURE);
				}			
				if(contatore == 4){
					exit(EXIT_SUCCESS);
				}
				
				area_triangolo = (data->base * data->altezza) /2;
				printf("\n[PROCESSO FIGLIO]: L'Area del Triangolo è': %d",area_triangolo);
				contatore++;
				
				result = sem_up(sem_id);
				if(result == -1){
					fprintf(stderr, "Errore nel rilasciare la risorsa controllata dal Semaforo");
					exit(EXIT_FAILURE);
				}	
				sleep(1);
				}			
		default:
			break;
	}
	
	for(int i=0; i<NUM_CYCLES; i++){
		result = sem_down(sem_id);
		if(result == -1){
			fprintf(stderr, "Errore nell'ottenere la risorsa controllata dal Semaforo");
			exit(EXIT_FAILURE);
		}
		
		printf("\n\n[PROCESSO PADRE]: Inserisci un Valore da dare alla Base: ");
		scanf("%d/n",&data->base);
		printf("[PROCESSO PADRE]: Inserisci un Valore da dare alla Altezza: ");
		scanf("%d/n",&data->altezza);				
		
		result = sem_up(sem_id);
		if(result == -1){
			fprintf(stderr, "Errore nel rilasciare la risorsa controllata dal Semaforo");
			exit(EXIT_FAILURE);
		}		
		sleep(1);
	}
	
	wait(NULL);
	
	// Sganciamento della Shared Memory
    result = shmdt(data);
    if(result == -1){
     	fprintf(stderr, "Errore nello Sganciamento della Shared Memory'\n");
    	exit(EXIT_FAILURE);   	
	}	
	
    // Eliminazione (Deallocazione) della Shared Memory
    result = shmctl(shm_id, IPC_RMID, NULL);
    if(result == -1){
    	fprintf(stderr, "Errore nell'Eliminazione (Dealocazione) della Shared Memory'\n");
    	exit(EXIT_FAILURE);
	}
    
    // Eliminazione del Semaforo
    result = sem_del(sem_id);
    if(result == -1){
     	fprintf(stderr, "Errore nell'Eliminazione del Semaforo\n");
    	exit(EXIT_FAILURE);   	
	}
}

// FUNZIONI
int sem_up(int semid) {
	int result;
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = 1;
    sb.sem_flg = SEM_UNDO;
    result = semop(semid, &sb, 1);
    if(result == -1)
		return 0; // se semop() fallisce, la funzione sem_up() restituisce 0 al chiamante
	return 1;
}

int sem_down(int semid) {
	int result;
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = SEM_UNDO;
    result = semop(semid, &sb, 1);
	if(result == -1) 
		return 0; // se semop() fallisce, la funzione sem_down() restituisce 0 al chiamante
 	return 1;
}

int sem_set(int semid, int val) {
    union semun un;
    un.val = val;
    return semctl(semid, 0, SETVAL, un);
}

int sem_del(int semid){
	return semctl(semid, 0, IPC_RMID, 0);
}

	