#include "tda.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

typedef struct metrics 
{
	int mode;
	unsigned long lostOverLastPeriod;
	unsigned long receivedOverLastPeriod;
	unsigned long lastReceivedIndex;
} metrics_t;

metrics_t* sharedMetrics;
//Protect access to sharedMetrics
pthread_mutex_t lock;

int main ()
{
	//Construct sharedMetrics
	sharedMetrics = (metrics_t*) malloc(sizeof(metrics_t));
	if(sharedMetrics == NULL)
	{
		fprintf(stderr, "[ERROR] Not enough memory to launch server \n");
		return(-1);
	}
	//Initialize mutex
	pthread_mutex_init(&lock, NULL);

	//Launch TCP server to establish metadata link
	//int fd = launchMetaDataLink();
	free(sharedMetrics);
	return(0);
}
