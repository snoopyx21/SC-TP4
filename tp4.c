#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define N 3

typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int var;
	int reveil;
} monsem_t;

void monsem_init(monsem_t * s, int i)
{
	s->var = i;
	pthread_mutex_init(&(s->mutex), NULL);
	pthread_cond_init(&(s->cond), NULL);
}

void monsem_destroy(monsem_t * s)
{
	pthread_mutex_destroy(&(s->mutex));
	pthread_cond_destroy(&(s->cond));
}

void monsem_P(monsem_t * s)
{
	pthread_mutex_lock(&(s->mutex));
	s->var--;
	if (s->var < 0)
	{	
		while(s->reveil <=0)
			pthread_cond_wait(&(s->cond), &(s->mutex));
		s->reveil--;
	}
	pthread_mutex_unlock(&(s->mutex));
}

void monsem_V(monsem_t * s)
{
	pthread_mutex_lock(&(s->mutex));
	s->var++;
	if (s->var <= 0)
		pthread_cond_signal(&(s->cond));
	pthread_mutex_unlock(&(s->mutex));	
}

void * fct(void * arg)
{
	monsem_t * s = arg;
	s->reveil++;
	monsem_P(s);
	return NULL;
}

int main(void)
{
	monsem_t s;
	pthread_t tid[N];
	int i, err;
	monsem_init(&s, 0);

	for(i=0; i<N; ++i)
	{
		err = pthread_create(&tid[i], NULL, fct, &s);
		if (err < 0) 
		{
			perror("pthread_create()");
			exit(1);
		}
	}
	sleep(1);
	monsem_V(&s);

	monsem_destroy(&s);
	return 0;
}