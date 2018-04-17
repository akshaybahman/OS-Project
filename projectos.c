#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define SIZE_BUFF 10		/* Size of shared buffer */

int buff[SIZE_BUFF];  	/* shared buffer */
int r = 0;  			/* place to add next element */
int f = -1; 			/* place to remove next element */
int count = 0;  		/* number elements in buffer */

int resourceAccess = 0;
int readerPriorityFlag = 0;     /* This flag is set to 1 when readers access the resources so that writers do not access it*/
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; 	/* mutex lock for buffer */
pthread_cond_t cons_cond = PTHREAD_COND_INITIALIZER; /* consumer waits on this cond var */
pthread_cond_t prod_cond = PTHREAD_COND_INITIALIZER; /* producer waits on this cond var */

pthread_mutex_t mtxData = PTHREAD_MUTEX_INITIALIZER; /* mutex lock for data buffer */

void *writer(void* param);
void *reader(void* param);

int main(int argc, char *argv[]) 
{
	srand(time(NULL));
	pthread_t thId[SIZE_BUFF];
	int idx;
	for (idx = 0; idx < SIZE_BUFF / 2; ++idx)
	{
	  if(pthread_create(&thId[idx], NULL, writer, NULL) != 0)
	  {
		fprintf(stderr, "Unable to create writer thread\n");
		exit(1);
	  }
	  if(pthread_create(&thId[idx + SIZE_BUFF/2], NULL, reader, NULL) != 0)
	  {
		fprintf(stderr, "Unable to create reader thread\n");
		exit(1);
	  }
	}
	for (idx = 0; idx < SIZE_BUFF; ++idx)
	{
		pthread_join(thId[idx], NULL);	
	}
	fprintf(stdout, "Parent thread quitting\n");
	return 0;
}

void *writer(void* param)
{	
	int r = rand() % 500;
	//fprintf(stdout, "Sleep for %d\n", r);
	usleep(r);
	//fprintf(stdout, "Thread writer\n");
	pthread_mutex_lock(&mtx);
		while(resourceAccess > 0 || readerPriorityFlag == 1)
			pthread_cond_wait(&prod_cond,&mtx);
		--resourceAccess;
	pthread_mutex_unlock(&mtx);
	
	// write data here	
	unsigned int tid = (unsigned int)pthread_self();

	pthread_mutex_lock(&mtxData);

	if (f != r)  // check if buffer is not full
	{
		int newVal = rand() % 300;
		buff[r] = newVal; 
		r = ( + 1) %  SIZE_BUFF; // set new position	
		int readersCount = resourceAccess < 0 ? 0 : resourceAccess;
		fprintf(stdout, "Data written by thread %u is %d with readers %d\n", tid, newVal, readersCount);
	}

	pthread_mutex_unlock(&mtxData);
	
	pthread_mutex_lock(&mtx);
		++resourceAccess;
		pthread_cond_broadcast(&cons_cond);
		pthread_cond_broadcast(&prod_cond);
	pthread_mutex_unlock(&mtx);

}

void *reader(void* param)
{	
	int r = rand()%(500);
	//fprintf(stdout, "Sleep for %d\n", r);
	usleep(r);
	//fprintf(stdout, "Thread reader\n");
	pthread_mutex_lock(&mtx);
//		while(resourceAccess < 0)
//			pthread_cond_wait(&mtx, &cons_cond);
		
		if(resourceAccess < 0) 
		{
			readerPriorityFlag = 1;
		}
		else
		{
			++resourceAccess;
		}
	pthread_mutex_unlock(&mtx);
	
	// read data here
	pthread_mutex_lock(&mtxData);
	if ((f + 1) % SIZE_BUFF != r)
	{
		f= (f + 1) % SIZE_BUFF;
		int val = buff[f];
		unsigned int tid = (unsigned int)pthread_self();
		fprintf(stdout, "Data read by thread %u\n is %d readers %d\n", tid, val, resourceAccess);	
	}
		
	pthread_mutex_unlock(&mtxData);		
	pthread_mutex_lock(&mtx);
		--resourceAccess;
		readerPriorityFlag = 0;
		pthread_cond_broadcast(&cons_cond);
		pthread_cond_broadcast(&prod_cond);
	pthread_mutex_unlock(&mtx);

}
