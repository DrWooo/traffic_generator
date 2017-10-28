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
  		perror("socket@launchMetaDataLink");
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
  		perror("bind@launchMetaDataLink");
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
  		perror("accept@launchMetaDataLink");
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
  		perror("rcv@launchMetaDataLink");
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
  		perror("socket@initiateTarget");
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
		perror("bind@initiateTarget");
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
	  		perror("accept@initiateTarget");
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

int launchTcpTarget(int conDes)
{
	char buffer[BUFFERS_SIZE];
	dataPacket_t* packet;
	if(recv(conDes, buffer, sizeof(dataPacket_t), 0) < 0)
	{
		perror("recv@launchTcpTarget");
		return(-1);
	}
	packet = (dataPacket_t*) buffer;
	if(packet == NULL)
	{
		fprintf(stderr, "[ERROR] Unable to cast the received message to the data packet type");
		return(-1);
	}
	fprintf(stdout, "Packet %lu received\n", packet->index);
	fprintf(stdout, "%s\n", packet->content);
	return(0);
}

int main ()
{
	int ftd = 0;
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
	
	ftd = initiateTarget();
	//ftd is either a socket descriptor (UDP) or a connection descriptor (TCP)
	if(ftd < 0)
	{
		exit(-1);
	}
	fprintf(stdout, "Target initialized.\n");
	printConfiguration();

	/*********************** Start target *****************************/
	fprintf(stdout, "Starting target ... \n");
	int retcode = 0;
	if(sharedConfig->mode == USE_TCP)
	{
		retcode = launchTcpTarget(ftd);
	}else{
		//retcode = launchUdpTarget(ftd);
	}
	if(retcode < 0)
	{
		fprintf(stderr, "[ERROR] Unable to start target\n");
	}



	free(sharedMetrics);
	return(0);
}
