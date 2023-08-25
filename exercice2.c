#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N 10

typedef struct semaphores {
    int compteur;
    int debloque;
    pthread_mutex_t mut;
    pthread_cond_t cond;
    } monsem_t;

typedef struct echange_threads{
    int il;
    int ie;
    monsem_t sem_l;
    monsem_t sem_e;
    int tableau[N];
    } echange;

int monsem_init(monsem_t *s, int c)
{
    s->compteur = c;
    s->debloque = 0;
    pthread_cond_init(&(s->cond), NULL);
    pthread_mutex_init(&(s->mut), NULL);
    return 0;    
}

int monsem_destroy(monsem_t *s)
{
    pthread_cond_destroy(&(s->cond));
    pthread_mutex_destroy(&(s->mut));
    return 0;
}

int monsem_P(monsem_t *s)
{
    pthread_mutex_lock(&(s->mut));
    s->compteur-=1;
    while(s->compteur < 0 && s->debloque <= 0)
    {
        pthread_cond_wait(&(s->cond), &(s->mut));
    }
    if(s->debloque>0)
        s->debloque-=1;
    pthread_mutex_unlock(&(s->mut));
    return 0;
}

int monsem_V(monsem_t *s)
{
    pthread_mutex_lock(&(s->mut));
    s->compteur+=1;
    if(s->compteur<=0)
    {
        s->debloque += 1;
        pthread_cond_signal(&(s->cond));
    }
    pthread_mutex_unlock(&(s->mut));
    return 0;
}

void* f(void *arg)
{
    echange *ech = arg;
    int val;
    int lire = 1;

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
	    write(1,&ech.tableau[ech.il],1);
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





