#include "tda.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
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

typedef struct configuration
{
	int mode;
	int serverFlowPort;
	struct sockaddr_in from;
	int metadataLinkSocketDescriptor;
	int flowLinkSocketDescriptor;
} configuration_t;

typedef struct metrics 
{
	unsigned long lostOverLastPeriod;
	unsigned long receivedOverLastPeriod;
	unsigned long lastReceivedIndex;
} metrics_t;

metrics_t* sharedMetrics;
configuration_t* sharedConfig;
//Protect access to sharedMetrics and sharedConfig
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
  	unsigned int size = 0;
  	//Accept and get connection descriptor
  	int cd = accept (sd, (struct sockaddr *) &(sharedConfig->from), &size);
  	if(cd < 0 )
  	{
  		fprintf(stderr, "[ERROR] Connection error\n");
  		return(-1);
  	}
  	//Register socket for futher close
  	sharedConfig->flowLinkSocketDescriptor = sd;
  	//Prepare to read the initiator
  	configurationPacket_t* initiator = NULL;
  	char receptionBuffer[BUFFERS_SIZE];
  	//Read the initiator
  	if(recv(cd, receptionBuffer, sizeof(metadataPacket_t), 0) < 0)
  	{
  		fprintf(stderr, "[ERROR] initiator packet reception error\n");
  		return(-1);
  	}
  	initiator = (configurationPacket_t*) receptionBuffer;
  	if(initiator == NULL)
  	{
  		fprintf(stderr, "[ERROR] initiator packet cast error\n");
  		return(-1);
  	}
  	//Read the initiator and save config
  	sharedConfig->mode = initiator->mode;
  	sharedConfig->serverFlowPort = initiator->serverFlowPort;
  	//Change the remote emitter port index to match the clientFlowPort
  	//(will especially be usefull for UDP connection)
  	sharedConfig->from.sin_port = htons(initiator->clientFlowPort);
  	return(cd);
}

int initiateTarget()
{
	int mode = sharedConfig->mode;
	int serverFlowPort = sharedConfig->serverFlowPort;
	//Prepare to get a socket
	int sd;
	int sz = 1;
	//Get a socket
	sd = socket (PF_INET, SOCK_STREAM, 0);
  	if(sd < 0)
  	{
  		fprintf(stderr, "\n[ERROR] Unable to get a socket for the flow link\n");
  		return(-1);
  	}
  	//Set the socket
  	setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &sz, 4);
  	//Prepare to bind
  	struct sockaddr_in portname;
  	memset (&portname, 0, sizeof portname);
  	portname.sin_port = htons (serverFlowPort);
  	portname.sin_family = AF_INET;
  	portname.sin_addr.s_addr = INADDR_ANY;
  	//Bind
  	if (bind (sd, (struct sockaddr *) &portname, sizeof portname) != 0)
	{
		fprintf(stderr, "\n[ERROR] Unable to bind the server port %d for flow link.\n", serverFlowPort);
		return (-1);
	}
	//Save socket for futher close
	sharedConfig->flowLinkSocketDescriptor = sd;
	if(mode == USE_TCP)
	{
		fprintf(stdout, "TCP\n");
		fprintf(stdout, "Waiting for flow link establishment...\n");
		listen(sd, 1);
		//Prepare to accept
	  	unsigned int size = 0;
	  	//Accept, get connection descriptor and replace the from structure inside the 
	  	//sharedConfig by this structur obtained on TCP connection
	  	int cd = accept (sd, (struct sockaddr *) &(sharedConfig->from), &size);
	  	if(cd < 0 )
	  	{
	  		fprintf(stderr, "[ERROR] Connection error for flow link (TCP)\n");
	  		return(-1);
	  	}
	  	return(cd);
	}else{
		fprintf(stdout, "UDP\n");
		//Nothing to do more.
		return(sd);
	}


}

void printConfiguration()
{
	char mode[3];
	if(sharedConfig->mode == USE_TCP)
	{
		strcpy(mode, "TCP");
	}else{
		strcpy(mode, "UDP");
	}
	fprintf(stdout, "\n-------- Test configuration --------\n");
	fprintf(stdout, "Mode : %s\n", mode);
	fprintf(stdout, "Metadata link on port : %d\n", DEFAULT_METADATAPORT);
	fprintf(stdout, "Flow link server port : %d\n", sharedConfig->serverFlowPort);
	fprintf(stdout, "Flow link remote port : %d\n", sharedConfig->from.sin_port);
	fprintf(stdout, "------------------------------------\n\n");
}

void* tagetRoutine(void* sd)
{
	int* ftd = (int*) sd;
	if(ftd == NULL)
	{
		fprintf(stderr, "[ERROR] Socket descriptor cast error on target thread\n");
	}
	
}

int main ()
{
	pthread_t targetThread;
	/*************** Initialize global variables ***********************/
	//Initialize mutex
	pthread_mutex_init(&lock, NULL);
	//Construct sharedMetrics
	sharedMetrics = (metrics_t*) malloc(sizeof(metrics_t));
	//Construct sharedConfig
	sharedConfig = (configuration_t*) malloc(sizeof(configuration_t));
	if(sharedMetrics == NULL || sharedConfig == NULL)
	{
		fprintf(stderr, "[ERROR] Not enough memory to launch server \n");
		return(-1);
	}
	/****************** Initialize configuration link *****************/
	//Launch TCP server to establish metadata link
	fprintf(stdout, "Waiting for metadata link establishment ...\n");
	int mcd = launchMetaDataLink(DEFAULT_METADATAPORT);
	if(mcd < 0)
	{
		exit(-1);
	}
	fprintf(stdout, "Metadata link established.\n");

	/***************** Initialize flow link **************************/
	//Create the socket according to the mode
	fprintf(stdout, "Initializing target, mode : ");
	int ftd = initiateTarget();
	if(ftd < 0)
	{
		exit(-1);
	}
	fprintf(stdout, "Target initialized.\n");
	printConfiguration();

	/*********************** Start target *****************************/
	fprintf(stdout, "Starting target ... \n");
	startTarget();

	while(1);




	free(sharedMetrics);
	return(0);
}
