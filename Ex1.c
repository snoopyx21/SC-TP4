#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define N 10

/* Representation d'un semaphore*/
typedef struct semaphores {
    int compteur;           // Valeur du semaphore
    int debloque;           // Nombre de processus autorises a etre reveilles
    pthread_mutex_t mut;    // Mutex permettant la modification du compteur en
                            // section critique.
    pthread_cond_t cond;    // Variable condition permettant de liberer
                            // momentanement un mutex.
    } monsem_t;

/* Structure modelisant un lecteur/ecrivain en se servant de threads */
typedef struct echange_threads{
    int il;         // indice du lecteur
    int ie;         // indice de l'ecrivain
    monsem_t sem_l; // semaphore du lecteur
    monsem_t sem_e; // semaphore de l'ecrivain
    int tableau[N]; // tableau dans lequel les donnees vont etre echangees
    } echange;

/**
 * @brief Initialise un semaphore.
 *
 * @param s un pointeur sur le semaphore a initialiser.
 * @param c la valeur d'initialisation du semaphore.
 *
 * @return 0 en cas de succes.
*/
int monsem_init(monsem_t *s, int c)
{
    s->compteur = c;
    s->debloque = 0;
    
    if((errno=pthread_cond_init(&(s->cond), NULL))!=0)
    {
        perror("Error pthread_cond_init");
        exit(13);
    }
    
    if((errno=pthread_mutex_init(&(s->mut), NULL))!=0)
    {
        perror("Error pthread_mutex_init");
        exit(12);
    }
    
    return 0;    
}

/**
 * @brief Detruit les elements d'un semaphore.
 *
 * Detruit le mutex et la variable condition associee a un semaphore.
 *
 * @param s un pointeur vers le sempahore a supprimer.
*/
int monsem_destroy(monsem_t *s)
{
    if((errno=pthread_cond_destroy(&(s->cond)))!=0)
    {
        perror("Error pthread_cond_destroy");
        exit(11);
    }
    
    if((errno=pthread_mutex_destroy(&(s->mut)))!=0)
    {
        perror("Error pthread_mutex_destroy");
        exit(10);
    }
    
    return 0;
}

/**
 * @brief Effectue une opération P sur un semaphore.
 *
 * Verrouille le mutex associe au semaphore pour empecher des modifications
 * simultanees.
 * Decremente le compteur.
 * Tant que le compteur du semaphore est negatif, et qu'aucun processus/thread
 * n'est autorise a sortir d'attente, le thread/processus se met en attente
 * en utilisant une variable condition qui deverouille le mutex en attendant que
 * le processus/thread soit reveille.
 * Si une autorisation de sortie d'attente a ete recu, on decremente le compteur
 * d'autorisations.
 * Puis pour finir le mutex est deverouille.
 *
 * @param s un pointeur vers le semaphore sur lequel effectuer l'operation
 *
 * @return 0 en cas de succes.
*/
int monsem_P(monsem_t *s)
{
    if((errno=pthread_mutex_lock(&(s->mut)))!=0)
    {
        perror("Error pthread_mutex_lock");
        exit(9);
    }
    
    s->compteur-=1;
    
    while(s->compteur < 0 && s->debloque <= 0)
    {
        if((errno=pthread_cond_wait(&(s->cond), &(s->mut)))!=0)
        {
            perror("Error pthread_cond_wait");
            exit(8);
        }
    }
    
    if(s->debloque>0)
        s->debloque-=1;
        
    if((errno=pthread_mutex_unlock(&(s->mut)))!=0)
    {
        perror("Error pthread_mutex_unlock");
        exit(7);
    }
    
    return 0;
}

/**
 * @brief Effectue une operation V sur un semaphore.
 *
 * Verrouille le mutex associe au semaphore pour empecher des modifications
 * simultanees.
 * Incremente le compteur.
 * Si le compteur est negatif, c'est qu'un thread/processus est en attente,
 * le compteur d'autorisation de reveil est incremente, et un signal est envoye
 * pour debloque un des threads en attente.
 * Deverouille le mutex. 
 *
 * @param s un pointeur vers le semaphore sur lequel effectuer l'operation
 *
 * @return 0 en cas de succes.
*/
int monsem_V(monsem_t *s)
{
    if((errno=pthread_mutex_lock(&(s->mut)))!=0)
    {
        perror("Error pthread_mutex_lock");
        exit(6);
    }
    
    s->compteur+=1;
    
    if(s->compteur<=0)
    {
        s->debloque += 1;
        if((errno=pthread_cond_signal(&(s->cond)))!=0)
        {
            perror("Error pthread_cond_signal");
            exit(5);
        }
    }
    
    if((errno=pthread_mutex_unlock(&(s->mut)))!=0)
    {
        perror("Error pthread_mutex_unlock");
        exit(4);
    }
    
    return 0;
}

/**
 * @brief Lit tout les caracteres de l'entree standard, un a un.
 *
 * Tant que la fin de fichier (EOF) n'as pas ete atteinte, lire un caractere
 * et le stocker dans la structure d'echange.
 * La synchronisation entre les deux threads se fait grace a deux sempahores
 * personnalises.
 *
 * @param arg un pointeur sur la structure d'echange entre les deux threads.
 *
 * @return NULL pas de retour necessaire
*/
void* f(void *arg)
{
    int val, lire = 1;
    echange *ech = arg;

    while(lire)
    {
        val = getchar();
        
        monsem_P(&ech->sem_e);
        
        ech->tableau[ech->ie]=val;
        if(val==EOF)
            lire = 0;
        ech->ie = (ech->ie+1)%N;
        
        monsem_V(&ech->sem_l);
    }
    
    pthread_exit(NULL);
}

/**
 * @brief Affiche un a un les caracteres lus par le thread secondaire.
 *
 * Initialise une structure echange qui servira a la communication entre les
 * deux threads.
 * Creation du thread secondaire.
 * Tant que le threads secondaire n'informe pas qu'il n'y a plus rien a ecrire
 * (presence de EOF), le caractere suivant dans la structure, est ecrit sur la
 * sortie standard.
 * Attente de la fin du thread secondaire.
 * La synchronisation des deux threads se fait a l'aide de deux semaphores
 * personnalises.
*/
int main(void)
{
	pthread_t tid[1];
    echange ech;
    
    ech.il = 0;
    ech.ie = 0;
	monsem_init(&ech.sem_l, 0);
	monsem_init(&ech.sem_e, N);
	
	pthread_create(&tid[0], NULL, f, &ech);

	while(1)
	{
	    monsem_P(&ech.sem_l);
	    if(ech.tableau[ech.il]==EOF)
	        break;
	    if(write(1,&ech.tableau[ech.il],1)==-1)
	    {
	        perror("Error write");
	        exit(3);
	    }
	    ech.il = (ech.il+1)%N;
	    monsem_V(&ech.sem_e);
	}
	
	if(pthread_join(tid[0], NULL)==-1)
	{
	    perror("pthread_join");
	    exit(2);
	}
	
	monsem_destroy(&ech.sem_l);
	monsem_destroy(&ech.sem_e);
	
	return 0;
}
