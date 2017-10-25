#include "tda.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#define DEFAULT_METADATAPORT 12346
#define DEFAULT_FLOWPORT 12345
#define BUFFERS_SIZE 1024

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

int launchMetaDataLink(int port)
{
	int sd;
	int sz = 1;
	//Get a socket
	sd = socket (PF_INET, SOCK_STREAM, 0);
  	if(sd < 0)
  	{
  		fprintf(stderr, "[ERROR] Unable to get a socket\n");
  		return(-1);
  	}
  	//Set the socket
  	setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &sz, 4);
  	//Prepare to bind
  	struct sockaddr_in portname;
  	memset (&portname, 0, sizeof portname);
  	portname.sin_port = htons (port);
  	portname.sin_family = AF_INET;
  	portname.sin_addr.s_addr = INADDR_ANY;
  	//Bind
  	if (bind (sd, (struct sockaddr *) &portname, sizeof portname) != 0)
  	{
  		fprintf(stderr, "[ERROR] Bind error.\n");
  		return (-1);
  	} 
  	//Listen for the client metadata link
  	listen(sd, 1);
  	//Prepare to accept
  	struct sockaddr_in from;
  	unsigned int size = 0;
  	//Accept and get connection descriptor
  	int cd = accept (sd, (struct sockaddr *) &from, &size);
  	if(cd < 0 )
  	{
  		fprintf(stderr, "[ERROR] Connection error\n");
  		return(-1);
  	}
  	//Prepare to read the initiator
  	metadataPacket_t* initiator = NULL;
  	char receptionBuffer[BUFFERS_SIZE];
  	//Read the initiator
  	if(recv(cd, receptionBuffer, sizeof(metadataPacket_t), 0) < 0)
  	{
  		fprintf(stderr, "[ERROR] initiator packet reception error\n");
  		return(-1);
  	}
  	initiator = (metadataPacket_t*) receptionBuffer;
  	if(initiator == NULL)
  	{
  		fprintf(stderr, "[ERROR] initiator packet cast error\n");
  		return(-1);
  	}
  	//Set the mode
  	//Alone here -> no need to protect
  	sharedMetrics->mode = initiator->mode;
  	//Return connexion descriptor
  	return(cd);

}

int initiateTarget(int port)
{
	int mode = sharedMetrics->mode;
	//Prepare to get a socket
	int sd;
	int sz = 1;
	//Get a socket
	sd = socket (PF_INET, SOCK_STREAM, 0);
  	if(sd < 0)
  	{
  		fprintf(stderr, "\n[ERROR] Unable to get a socket\n");
  		return(-1);
  	}
  	//Set the socket
  	setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &sz, 4);
  	 //Prepare to bind
  	struct sockaddr_in portname;
  	memset (&portname, 0, sizeof portname);
  	portname.sin_port = htons (port);
  	portname.sin_family = AF_INET;
  	portname.sin_addr.s_addr = INADDR_ANY;
  	//Bind
  	if (bind (sd, (struct sockaddr *) &portname, sizeof portname) != 0)
	{
		fprintf(stderr, "\n[ERROR] Unable to bind.\n");
		return (-1);
	}
	if(mode == USE_TCP)
	{
		fprintf(stdout, "TCP\n");
		fprintf(stdout, "Waiting for flow link establishment...\n");
		listen(sd, 1);
		//Prepare to accept
	  	struct sockaddr_in from;
	  	unsigned int size = 0;
	  	//Accept and get connection descriptor
	  	int cd = accept (sd, (struct sockaddr *) &from, &size);
	  	if(cd < 0 )
	  	{
	  		fprintf(stderr, "[ERROR] Connection error\n");
	  		return(-1);
	  	}
	}else{
		fprintf(stdout, "UDP\n");
	}


}

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
	fprintf(stdout, "Waiting for metadata link establishment ...\n");
	int mcd = launchMetaDataLink(DEFAULT_METADATAPORT);
	if(mcd < 0)
	{
		exit(-1);
	}
	fprintf(stdout, "Metadata link established.");

	//Create the socket according to the mode
	fprintf(stdout, "Initializing target, mode : ");
	int ftd = initiateTarget(DEFAULT_FLOWPORT);
	if(ftd < 0)
	{
		exit(-1);
	}
	fprintf(stdout, "Target initialized.\n", );




	free(sharedMetrics);
	return(0);
}
