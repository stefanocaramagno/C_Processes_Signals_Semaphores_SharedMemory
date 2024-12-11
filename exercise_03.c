/*
Implementare un programma in cui viene simulato un sistema di controllo motori.
Il programma prevede che un processo padre P1, dopo aver generato due processi figli P2 e P3, 
comunichi ogni 2 secondi a P2 e P3 un'azione da eseguire, 
scelta cosualmente tra: Avanti, Indietro, Sinistra, Destra, Stop.

Dopo 20 invii, P1 trasmette il comando "Termina" e attende la terminazione dei figli.
Entrambi i Processi Figli stampano a video l'azione da eseguire ogni volta che questa cambia. 
Se invece leggono il comando "Termina" terminano.
*/

// LIBRERIE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>

// PROTOTIPI
int sem_down(int sem_id); // Si Occupa di ottenere una Risorsa controllata dal Semaforo
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

// COSTANTI
#define KEY_SEM (key_t) 9876 // Chiave per la Gestione del Semaforo
#define KEY_SHM (key_t) 9875 // Chiave per la Gestione della Shared Memory

// MAIN
int main(void){
    int semid; // Identificativo del Semaforo che andremo a Creare
	int shmid; // Identificativo della Shared Memory che andremo a Creare
	
	int r; 
	
    pid_t m1; // Si definisce il Processo Figlio n°1
	pid_t m2; // Si definisce  il Processo Figlio n°2
	
	char *comando;
    char *cmds[] = {"Avanti", "Indietro", "Destra", "Sinistra", "Stop"};
    char oldcmd[100] = "";
    
    int result; // Variabile per il Controllo di Possibili Errori

    // Creazione del Semaforo
    semid = semget(KEY_SEM, 1, 0666 | IPC_CREAT); 
    if(semid == -1){
    	fprintf(stderr, "Errore di Creazione del Semaforo\n");
		exit(EXIT_FAILURE);	
	}
    
    // Inizializzazione del Semaforo
    result = sem_set(semid, 1);
    if(result == -1){
    	fprintf(stderr, "Errore nell'Inizializzazione del Semaforo\n");
    	exit(EXIT_FAILURE);
	} 
	  
    // Creazione della Shared Memory
    shmid = shmget(KEY_SHM, sizeof(char)*100, 0666 | IPC_CREAT); 
    if(shmid == -1){
    	fprintf(stderr, "Errore di Creazione del Shared Memory\n");
		exit(EXIT_FAILURE);	
	}
	
	// Si aggancia la Shared Memory con lo spazio degli Indirizzi di Memoria del Processo Padre
	comando = (char *)shmat(shmid, NULL, 0);
	
	// Fulcro del Programma
    m1 = fork();
    switch(m1) {
        case -1: 
			break;
        case 0:
            while(1){
				result = sem_down(semid);
				if(result == -1){
    				fprintf(stderr, "Errore nell'ottenere la risorsa controllata dal Semaforo\n");
					exit(EXIT_FAILURE);						
				} 
                if(strncmp(comando, "Termina", 7) == 0){
                    shmdt(comando);
                    sem_up(semid);
                    exit(0);
                }
                if(strncmp(comando, oldcmd, 100) != 0) {
                    strncpy(oldcmd, comando, 100);
                    printf("Figlio 1: Ricevuto %s\n", comando);
                }
				result = sem_up(semid); 
				if(result == -1){
    				fprintf(stderr, "Errore nel rilasciare la risorsa controllata dal Semaforo\n");
					exit(EXIT_FAILURE);						
				} 	
                sleep(1);
            }
        default:
            break;
    }

    m2 = fork();
    switch(m2) {
        case -1: break;
        case 0:
            while(1){
				result = sem_down(semid);
				if(result == -1){
    				fprintf(stderr, "Errore nell'ottenere la risorsa controllata dal Semaforo\n");
					exit(EXIT_FAILURE);						
				} 
                if(strncmp(comando, "Termina", 7) == 0){
                    shmdt(comando);
                    sem_up(semid);
                    exit(0);
                }
                if(strncmp(comando, oldcmd, 100) != 0) {
                    strncpy(oldcmd, comando, 100);
                    printf("Figlio 2: Ricevuto %s\n", comando);
                }
				result = sem_up(semid); 
				if(result == -1){
    				fprintf(stderr, "Errore nel rilasciare la risorsa controllata dal Semaforo\n");
					exit(EXIT_FAILURE);						
				} 	
                sleep(1);
            }
        default:
            break;
    }

    srand(getpid());
    for(int i=0; i<20; i++) {
        r = rand() % 5;
		result = sem_down(semid);
		if(result == -1){
    		fprintf(stderr, "Errore nell'ottenere la risorsa controllata dal Semaforo\n");
			exit(EXIT_FAILURE);	
			
        strcpy(comando, cmds[r]);
        printf("Padre: %s\n", comando);
        
		result = sem_up(semid); 
		if(result == -1){
    		fprintf(stderr, "Errore nel rilasciare la risorsa controllata dal Semaforo\n");
			exit(EXIT_FAILURE);						
		} 
        sleep(2);
    }

	result = sem_down(semid);
		if(result == -1){
    		fprintf(stderr, "Errore nell'ottenere la risorsa controllata dal Semaforo\n");
			exit(EXIT_FAILURE);	
			
    strcpy(comando, "Termina");
    
	result = sem_up(semid); 
		if(result == -1){
    		fprintf(stderr, "Errore nel rilasciare la risorsa controllata dal Semaforo\n");
			exit(EXIT_FAILURE);						
	} 
	
    wait(NULL); 
	wait(NULL);
	
	// Sganciamento della Shared Memory
    result = shmdt(comando);
    if(result == -1){
     	fprintf(stderr, "Errore nello Sganciamento della Shared Memory'\n");
    	exit(EXIT_FAILURE);   	
	}
    
    // Eliminazione (Deallocazione) della Shared Memory
    result = shmctl(shmid, IPC_RMID, NULL);
    if(result == -1){
    	fprintf(stderr, "Errore nell'Eliminazione (Dealocazione) della Shared Memory'\n");
    	exit(EXIT_FAILURE);
	}
    
    // Eliminazione del Semaforo
    result = sem_del(semid);
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

