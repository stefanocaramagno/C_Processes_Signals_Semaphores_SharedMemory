/*
Un sistema di monitoraggio rileva periodicamente temperatura e pressione da due sensori (un sensore di temperatura ed uno di pressione). 
Tale coppia di sensori è caratterizzata da un ID e dal valore di temperatura/pressione, ossia:

struct sh_mem {
    int id;
    int temperatura;
	int pressione;
};

Il sistema è composto da 2 processi (P0 e P1). 
P0 è il processo di controllo.
P1 è il processo che campiona i dati dai sensori.
P0, dopo aver creato e inizializzato gli attributi dei sensori (impostando l'ID a 1, il valore di temperatura a 50 e il valore della pressione a 800)
	- crea il processo figlio P1

P0, ciclicamente e per 10 volte, con pause di 1 secondo, elabora il valore dei sensori ed in particolare svolge le seguenti operazioni 
	- visualizza i valori di temperatura e pressione
	- scala del 2% il valore della temperatura, se supera la soglia di 58
	- aumenta del 2% il valore della pressione, se inferiore a 760
	
Al termine dei 10 cicli, P0 
	- imposta il valore della temperatura a -100
	- imposta il valore della pressione a 0
	- attende la terminazione di P1
	- elimina shared memory e semafori
	- termina

Il processo P1 ciclicamente, con pause di 2 secondi
	- aggiorna il valore di temperatura, incrementandolo di una certa percentuale random tra il 2% e il 4%
	- aggiorna il valore di pressione, decrementandolo di una certa percentuale random tra l'1% e il 3%
	- se il valore di temperatura è uguale a -100 e il valore di pressione è uguale a 0, termina

Utilizzare il meccanismo la shared memory per la comunicazione tra processi e i semafori per regolare l’accesso alle risorse condivise.
*/

// LIBRERIE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

// PROTOTIPI
int sem_down(int sem_id); // Funzione che si occupa Ottenere la Risorsa controllata dal Semaforo
int sem_up(int sem_id); // Funzione si occupa di Rilasciare la Risorsa controllata dal Semaforo
int sem_set(int sem_id, int val); // Funzione si occupa di Inizializzare il Semaforo
int sem_del(int sem_id); // Funzione si occupa di Eliminare il Semaforo

// STRUTTURA SEMAFORO
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
};

// STRUTTURA ELEMENTO CONDIVISO
struct sh_mem {
    int id;
    int temperatura;
	int pressione;
};

// COSTANTI
#define KEY_SEM (key_t) 1234 // Chiave per la Gestione del Semaforo
#define KEY_SHM (key_t) 5678 // Chiave per la Gestione della Shared Memory
#define NUM_CYCLES 10

// MAIN
int main(void){
	int sem_id; // Identificativo del Semaforo che andremo a Creare
	int shm_id; // Identificativo della Shared Memory che andremo a Creare

	int result; // Variabile per il Controllo di Possibili Errori	
	int n;
	int m;
	pid_t figlio;

	struct sh_mem *sh_data;
	
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
     sh_data = (struct sh_mem *)shmat(shm_id, NULL, 0);
	
	// Fulcro del Programma
	sh_data->id = 1;
	sh_data->temperatura = 50;
	sh_data->pressione = 800;
	
	figlio = fork();
	switch(figlio){
		case -1:
			perror("fork");
			exit(EXIT_SUCCESS);
		case 0:
			srand(time(NULL));
			while(1){
				result = sem_down(sem_id);
				if(result == -1){
					fprintf(stderr, "Errore nell'ottenere la risorsa controllata dal Semaforo\n");
					exit(EXIT_FAILURE);
				}
				
				if(sh_data->temperatura == -100 && sh_data->pressione == 0){
					exit(EXIT_SUCCESS);
				}
				
				n = ((rand()%(4-2+1))+2)/100;
				sh_data->temperatura = sh_data->temperatura + n*(sh_data->temperatura);
				
				m = ((rand()%(3-1+1))+1)/100;
				sh_data->pressione = sh_data->pressione - n*(sh_data->pressione);			
			
				result = sem_up(sem_id);
				if(result == -1){
					fprintf(stderr, "Errore nel rilasciare la risorsa controllata dal Semaforo\n");
					exit(EXIT_FAILURE);
				}
				sleep(2);
			}	
		default:
			break;
	}
	
	for(int i=0; i<NUM_CYCLES; i++){
		result = sem_down(sem_id);
		if(result == -1){
			fprintf(stderr, "Errore nell'ottenere la risorsa controllata dal Semaforo\n");
			exit(EXIT_FAILURE);
		}
			
		printf("\n[PROCESSO PADRE]: ITERAZIONE = %d",i+1);
		printf("\n[PROCESSO PADRE]: Temperatura = %d",sh_data->temperatura);		
		printf("\n[PROCESSO PADRE]: Pressione = %d\n",sh_data->pressione);		
		
		if(sh_data->temperatura > 58){
			sh_data->temperatura = sh_data->temperatura - (2/100)*sh_data->temperatura;
		}
		
		if(sh_data->pressione > 760){
			sh_data->pressione = sh_data->pressione + (2/100)*sh_data->pressione;		
		}		
		
		result = sem_up(sem_id);
		if(result == -1){
			fprintf(stderr, "Errore nel rilasciare la risorsa controllata dal Semaforo\n");
			exit(EXIT_FAILURE);
		}		
		sleep(1);
	}
		
	result = sem_down(sem_id);
	if(result == -1){
		fprintf(stderr, "Errore nell'ottenere la risorsa controllata dal Semaforo\n");
		exit(EXIT_FAILURE);
	}	
	
	sh_data->temperatura = -100;
	sh_data->pressione = 0;
	
	result = sem_up(sem_id);
	if(result == -1){
		fprintf(stderr, "Errore nel rilasciare la risorsa controllata dal Semaforo\n");
		exit(EXIT_FAILURE);
	}		
	
	printf("\n\n[PROCESSO PADRE]: Il Valore della Temperatura e' stato portato al seguente valore: %d",sh_data->temperatura);
	printf("\n[PROCESSO PADRE]: Il Valore della Pressione e' stato portato al seguente valore: %d",sh_data->pressione);	
	
	wait(NULL);
	
	// Sganciamento della Shared Memory
    result = shmdt(sh_data);
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
int sem_up(int semid){
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

int sem_down(int semid){
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

int sem_set(int semid, int val){
    union semun un;
    un.val = val;
    return semctl(semid, 0, SETVAL, un);
}

int sem_del(int semid){
	return semctl(semid, 0, IPC_RMID, 0);
}


