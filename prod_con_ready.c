#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#define THREADS	1000
#define BUFSIZE 20

/*
** This simple producer-consumer just has producers writing X's in an array
** and consumers changing the X's back to NULL.
** With no protection of the critical section, the race condition problems
** become obvious quickly, and manifest in the following ways:
** 1> Items are left over even though the number of producers and consumers is equal
** 2> The program hangs forever
** 3> The program crashes with a segmentation fault
*/





typedef struct item_t
{
	char *product;
	int size;
} ITEM;

ITEM *makeItem()
{
	int i;
	ITEM *p = malloc( sizeof(ITEM) );
	p->size = random()%80;
	p->product = malloc(p->size);	
	for ( i = 0; i < p->size-1; i++ )
		p->product[i] = 'X';
	p->product[i] = '\0';

	return p;
}

void useItem( ITEM *p )
{
	printf( "%s\n", p->product );
	free( p->product );
	free( p );
}

int count = 0;
ITEM* buffer[BUFSIZE];
sem_t place, product;
pthread_mutex_t lock;

void *produce( void *ign )
{
	// Wait for room in the buffer
	
	//while(count > BUFSIZE);
	//
	sem_wait(&place);
	pthread_mutex_lock(&lock);
	ITEM *item = makeItem();
	// Produce the X in the next slot in the buffer
	buffer[count] = item;
	printf( "this item is produced %s\n", (*buffer[count]).product );
	count++;
	pthread_mutex_unlock(&lock);
	sem_post(&product);
	pthread_exit( NULL );
}

void *consume( void *ign )
{
	// Wait for items in the buffer
	sem_wait(&product);
	pthread_mutex_lock(&lock);
	// Overwrite the last X with a null
	printf( "this item is consumed %s\n", (*buffer[count-1]).product );
	useItem(buffer[--count]);
	pthread_mutex_unlock(&lock);
	sem_post(&place);
	pthread_exit( NULL );
}

int main( int argc, char **argv )
{
	pthread_t threads[THREADS*2];
	int status, i, j;
	sem_init(&place, 0, 20);
	sem_init(&product, 0, 0);
	count = 0;
	for ( j = 0, i = 0; i < THREADS; i++ )
	{
		status = pthread_create( &threads[j++], NULL, produce, NULL );
		if ( status != 0 )
		{
			printf( "pthread_create error %d.\n", status );
			exit( -1 );
		}
		status = pthread_create( &threads[j++], NULL, consume, NULL );
		if ( status != 0 )
		{
			printf( "pthread_create returned error %d.\n", 
				status );
			exit( -1 );
		}
	}
	for ( j = 0; j < THREADS*2; j++ )
		pthread_join( threads[j], NULL );
	printf( "Finally, the count is %d.\n", count );
	pthread_exit( 0 );
}
