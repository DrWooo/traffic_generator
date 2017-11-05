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


typedef struct configuration
{
	int mode;
	int serverMetadataPort;
	int serverFlowPort;
	struct sockaddr_in from;
	int metadataLinkSocketDescriptor;
	int metadataLinkTcpConnexionDescriptor;
	int flowLinkSocketDescriptor;
	int flowLinkTcpConnectionDescriptor;
	pthread_t targetThreadId;
	pthread_t metadataEmitterId;
} configuration_t;

//NB for config we use a shared config cause cast of target thread

typedef struct metrics 
{
	unsigned long lostOverLastPeriod;
	unsigned long receivedOverLastPeriod;
	unsigned long lastReceivedIndex;
} metrics_t;

//Both following can only be called by receiveFlow.
//Their presence prevent the use of the mutex
int glbReceiveDescriptor;
struct sockaddr* glbFrom;

metrics_t* sharedMetrics;
configuration_t* sharedConfig;
pthread_mutex_t lockConfig;
pthread_mutex_t lockStats;

/*!
 * \brief Opens a TCP socket used to exchange metadata with the emitter
 * Waits for a connexion, receives the first packet (initiator) that contains usefull informations
 * about the test to come.
 * Sets and reads the configuration in/from sharedConfig (unprotected)
 * \return 0 on succes, non-zero value on error
 */
int launchMetaDataLink(void)
{
	int port = sharedConfig->serverMetadataPort; //Get the port to use
	int sd, cd; //Socket descriptor, connection descriptor
	int optVal = 1; //Option value for setsockopt
	struct sockaddr_in portname; //Structure to bind
	char receptionBuffer[BUFFERS_SIZE]; //Buffer for the reception of the initiator

	//Get a socket
	sd = socket (PF_INET, SOCK_STREAM, 0);
  	if(sd < 0)
  	{
  		perror("socket@launchMetaDataLink");
  		return(-1);
  	}
  	//Store socket descriptor, set option
  	sharedConfig->metadataLinkSocketDescriptor = sd;
  	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optVal, 4);
  	//Prepare to bind
  	memset(&portname, 0, sizeof portname);
  	portname.sin_port = htons (port);
  	portname.sin_family = AF_INET;
  	portname.sin_addr.s_addr = INADDR_ANY;
  	//Bind
  	if (bind (sd, (struct sockaddr *) &portname, sizeof portname) != 0)
  	{
  		perror("bind@launchMetaDataLink");
  		return (-1);
  	} 
  	//Listen for the client metadata link, accept and get connection descriptor
  	listen(sd, 1);
  	unsigned int size = 0;
  	cd = accept (sd, (struct sockaddr *) &(sharedConfig->from), &size);
  	if(cd < 0 )
  	{
  		perror("accept@launchMetaDataLink");
  		return(-1);
  	}
  	//Store connexion descriptor
  	sharedConfig->metadataLinkTcpConnexionDescriptor = cd;
  	//PRead the initiator message
  	configurationPacket_t* initiator = NULL;
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
  	//Save config
  	sharedConfig->mode = initiator->mode;
  	sharedConfig->serverFlowPort = initiator->serverFlowPort;
  	//Change the remote emitter port index to match the clientFlowPort
  	//(will especially be used in recvfrom in case of an udp test)
  	sharedConfig->from.sin_port = htons(initiator->clientFlowPort);
  	return(0);
}

/*!
 * \brief Open and bind a socket for reception of the flow.
 * In case of TCP, wait for the connexion with the emitter.
 * Sets and reads the configuration in/from sharedConfig (unprotected)
 * \return 0 on succes, non-zero value on failure
 */
int initiateTarget()
{
	int mode = sharedConfig->mode;
	int serverFlowPort = sharedConfig->serverFlowPort;
	int sd; //Socket
	int sz = 1; //Option for socket
	struct sockaddr_in portname; //Structure to bind
	int type; //Socket type

	//Type depending on mode
	if(mode == USE_TCP)
	{
		type = SOCK_STREAM;
	}else{
		type = SOCK_DGRAM;
	}
	//Get a socket, set option
	sd = socket (PF_INET, type, 0);
  	if(sd < 0)
  	{
  		perror("socket@initiateTarget");
  		return(-1);
  	}
  	setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &sz, 4);
  	//Save socket descriptor
	sharedConfig->flowLinkSocketDescriptor = sd;
  	//Prepare to bind
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
	  	//Store connection descriptor
	  	sharedConfig->flowLinkTcpConnectionDescriptor = cd;
	  	return(0);
	}else{
		fprintf(stdout, "UDP\n");
		//Nothing to do more.
		return(0);
	}
}

/*!
 * /brief Prints the configuration on the stdout
 */
void printConfiguration()
{
	char mode[5];
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

/*!
 * \brief Receives some bytes on the flow port, either for UDP or TCP connection
 * Reads the configuration from sharedConfig (protected)
 * \param[out] buffer, the buffer where to store the bytes
 * \param[in] number of bytes to read
 * \param[in] substractHeader, set to 1 to substract the header to the given size
 * \return the size read
 */
int receiveFlow(char* buffer, size_t size, int substractHeader)
{
	int retval = -1;
	if(sharedConfig->mode == USE_TCP)
	{
		if(substractHeader)
		{
			retval = recv(glbReceiveDescriptor, buffer, size - sizeof(dataPacket_t) - TCPIP_HEADERSIZE, 0);
		}else{
			retval = recv(glbReceiveDescriptor, buffer, size, 0);
		}
	}else{
		socklen_t addrSize = sizeof(sharedConfig->from);
		if(substractHeader)
		{
			retval = recvfrom(glbReceiveDescriptor, buffer, size - sizeof(dataPacket_t) - UDPIP_HEADERSIZE, 0, glbFrom, &addrSize);
		}else{
			retval = recvfrom(glbReceiveDescriptor, buffer, size, 0, glbFrom, &addrSize);
		}
	}
	return retval;
}
/*!
 * \brief Routine to be executed in a separate thread as a target of the flow.
 * Receives the packet, identifies their composiion and generates metrics.
 * Stops when no more is to be received (recv or recvfrom returns 0)
 * Reads configuration from sharedConfig (protected)
 * Sets the metrics in sharedMetrics (protected)
 * NOTE : Initially, this function was supposed to work both with udp and tcp mode
 * but we had to seperate it because the behaviour of recvfrom is NOT the same as recv
 * Indeed, after research it appears that you can't read part of datagrams when using them.
 * \param [in] noArgs : unused
 * \return always NULL
 */
void* targetRoutineTcp(void* noArgs)
{
	char buffer[BUFFERS_SIZE]; //Buffer to store reception on one cycle
	dataPacket_t* packet; //The data packet structure, pointing into buffer, no dynamic allocation
	int continueLoop = 1; //Boolean to continue reception

	while(continueLoop)
	{
		int receivedFirst, receivedSecond; //Bytes received
		//Step 1 : Receive the bytes needed for the dataPacket_t structure...
		receivedFirst = receiveFlow(buffer, sizeof(dataPacket_t),0);
		if(receivedFirst < 0)
		{
			perror("receiveFlow@targetRoutine");
			pthread_exit(NULL);
			return NULL;
		}
		if(receivedFirst == 0)
		{
			continueLoop = 0;
		}
		//Step 2 : Retrieve the packet (starting at buffer[0], ie buffer)
		packet = (dataPacket_t*) buffer;
		if(packet == NULL)
		{
			fprintf(stderr, "[ERROR] Unable to cast the received message to the data packet type");
			pthread_exit(NULL);
			return NULL;
		}
		//Step 3 : Retrieve the content and store it next to the data packet, on the buffer
		receivedSecond = receiveFlow(buffer + sizeof(dataPacket_t), packet->size,1);
		//Step 4 : Link packet->content to be the right next byte
		packet->content = buffer + sizeof(dataPacket_t);

		//Print for debug 
		TRACE_LOG("Received : %d. Expected : %lu, Index: %lu, Content : %s\n", receivedFirst + receivedSecond, packet->size - (sharedConfig->mode == USE_TCP ? TCPIP_HEADERSIZE : UDPIP_HEADERSIZE), packet->index, packet->content);
		
		//Now, it's time for metrics ...
		pthread_mutex_lock(&lockStats);
		sharedMetrics->receivedOverLastPeriod++;
		if(packet->index < sharedMetrics->lastReceivedIndex)
		{
			//Late packet, was considered lost but it's not, decrement if able
			if(sharedMetrics->lostOverLastPeriod > 0) sharedMetrics->lostOverLastPeriod--;
		}else{
			if(packet->index > (sharedMetrics->lastReceivedIndex + 1))
			{
				//Packet of index sharedMetrics->lasReceivedIndex + 1 is missing ...
				sharedMetrics->lostOverLastPeriod++;
			}
			//Update last received index
			sharedMetrics->lastReceivedIndex = packet->index;
		}
		pthread_mutex_unlock(&lockStats);
	} //Loop
	pthread_exit(NULL);
	return NULL;
}

/*!
 * \brief Routine to be executed in a separate thread as a target of the flow.
 * Receives the packet, identifies their composiion and generates metrics.
 * Stops when no more is to be received (recv or recvfrom returns 0)
 * Reads configuration from sharedConfig (protected)
 * Sets the metrics in sharedMetrics (protected)
 * NOTE : Initially, this function was supposed to work both with udp and tcp mode
 * but we had to seperate it because the behaviour of recvfrom is NOT the same as recv
 * Indeed, after research it appears that you can't read part of datagrams when using them.
 * \param [in] noArgs : unused
 * \return always NULL
 */
void* targetRoutineUdp(void* noArgs)
{
	char buffer[BUFFERS_SIZE]; //Buffer to store reception on one cycle
	dataPacket_t* packet; //The data packet structure, pointing into buffer, no dynamic allocation
	int continueLoop = 1; //Boolean to continue reception

	while(continueLoop)
	{
		int received;
		//Step 1 : Receive ALL the datagram
		received = receiveFlow(buffer, BUFFERS_SIZE,0);
		//Note : on datagram mode, it's ok to provide a size bigger than what will actually be read
		if(received< 0)
		{
			perror("receiveFlow@targetRoutine");
			pthread_exit(NULL);
			return NULL;
		}
		//Step 2 : Retrieve the packet (starting at buffer[0], ie buffer)
		packet = (dataPacket_t*) buffer;
		if(packet == NULL)
		{
			fprintf(stderr, "[ERROR] Unable to cast the received message to the data packet type");
			pthread_exit(NULL);
			return NULL;
		}
		//Step 3 : Link the content
		packet->content = buffer + sizeof(dataPacket_t);

		//Print for debug 
		TRACE_LOG("Received : %d. Expected : %lu, Index: %lu, Content : %s\n", received, packet->size - (sharedConfig->mode == USE_TCP ? TCPIP_HEADERSIZE : UDPIP_HEADERSIZE), packet->index, packet->content);
		
		//Now, it's time for metrics ...
		pthread_mutex_lock(&lockStats);
		sharedMetrics->receivedOverLastPeriod++;
		if(packet->index < sharedMetrics->lastReceivedIndex)
		{
			//Late packet, was considered lost but it's not, decrement if able
			if(sharedMetrics->lostOverLastPeriod > 0) sharedMetrics->lostOverLastPeriod--;
		}else{
			if(packet->index > (sharedMetrics->lastReceivedIndex + 1))
			{
				//Packet of index sharedMetrics->lasReceivedIndex + 1 is missing ...
				sharedMetrics->lostOverLastPeriod++;
			}
			//Update last received index
			sharedMetrics->lastReceivedIndex = packet->index;
		}
		pthread_mutex_unlock(&lockStats);
	} //Loop
	pthread_exit(NULL);
	return NULL;
}



/*!
 * \brief Launches the reception target
 * \return 0 on succes, -1 on failure
 */
int launchTarget(void)
{
	pthread_mutex_lock(&lockConfig);
	if(sharedConfig->mode == USE_TCP)
	{
		glbReceiveDescriptor = sharedConfig->flowLinkTcpConnectionDescriptor;
		if(pthread_create(&sharedConfig->targetThreadId, NULL, targetRoutineTcp, NULL)!=0)
		{
			pthread_mutex_unlock(&lockConfig);
			fprintf(stderr, "[ERROR] Unable to start target\n");
			return (-1);
		}
	}else{
		glbReceiveDescriptor = sharedConfig->flowLinkSocketDescriptor;
		glbFrom = (struct sockaddr *) &(sharedConfig->from);
		if(pthread_create(&sharedConfig->targetThreadId, NULL, targetRoutineUdp, NULL)!=0)
		{
			pthread_mutex_unlock(&lockConfig);
			fprintf(stderr, "[ERROR] Unable to start target\n");
			return (-1);
		}
	}
	pthread_mutex_unlock(&lockConfig);
	return 0;
}
/*!
 * \brief Closes the sockets
 */
void closeSockets(void)
{
	close(sharedConfig->metadataLinkSocketDescriptor);
	close(sharedConfig->flowLinkSocketDescriptor);
}
/*!
 * \brief Get memory and initialize global variables
 */
int initializeGlobalVariables(void)
{
	//Initialize mutex
	pthread_mutex_init(&lockConfig, NULL);
	pthread_mutex_init(&lockStats, NULL);
	//Construct sharedMetrics
	sharedMetrics = (metrics_t*) malloc(sizeof(metrics_t));
	//Construct sharedConfig
	sharedConfig = (configuration_t*) malloc(sizeof(configuration_t));
	//Check for error
	if(sharedMetrics == NULL || sharedConfig == NULL)
	{
		fprintf(stderr, "[ERROR] Not enough memory to launch server \n");
		return(-1);
	}
	//Set default values
	sharedConfig->mode = USE_TCP;
	sharedConfig->serverMetadataPort = DEFAULT_METADATAPORT;
	sharedConfig->serverFlowPort = DEFAULT_FLOWPORT;
	memset(&sharedConfig->from, 0, sizeof sharedConfig->from);
	sharedConfig->metadataLinkSocketDescriptor = 0;
	sharedConfig->metadataLinkTcpConnexionDescriptor = 0;
	sharedConfig->flowLinkSocketDescriptor = 0;
	sharedConfig->flowLinkTcpConnectionDescriptor = 0;
	sharedConfig->targetThreadId = 0;
	sharedMetrics->lostOverLastPeriod = 0;
	sharedMetrics->receivedOverLastPeriod = 0;
	sharedMetrics->lastReceivedIndex = 0;
	return (0);
}
/*!
 * \brief Waits the specified time
 * \param [in] millisec : time to wait in milliseconds
 */
void waitMs (int millisec)
{
  struct timeval tv;
  tv.tv_sec = millisec % 1000;
  tv.tv_usec = (millisec - (tv.tv_sec * 1000)) * 1000;       
  select (0, NULL, NULL, NULL, &tv);
}

void* metadataEmitterRoutine(void* noArgs)
{
	metadataPacket_t stats;
	int fd;
	pthread_mutex_lock(&lockConfig);
	fd = sharedConfig->metadataLinkTcpConnexionDescriptor;
	pthread_mutex_unlock(&lockConfig);
	while(1)
	{
		pthread_mutex_lock(&lockStats);
		stats.lostOverLastPeriod = sharedMetrics->lostOverLastPeriod;
		stats.receivedOverLastPeriod = sharedMetrics->receivedOverLastPeriod;
		stats.lastIndexReceived = sharedMetrics->lastReceivedIndex;
		sharedMetrics->lostOverLastPeriod = 0;
		sharedMetrics->receivedOverLastPeriod = 0;
		pthread_mutex_unlock(&lockStats);
		send(fd, &stats, sizeof(metadataPacket_t), 0);

		//Print for debug
		fprintf(stdout, "Rec : %lu. Lost : %lu. LastId : %lu.\n", stats.receivedOverLastPeriod, stats.lostOverLastPeriod, stats.lastIndexReceived);

		//Wait for period
		waitMs(COMMON_PERIOD_MS);
	}
}

int launchMetadataEmitter(void)
{
	pthread_mutex_lock(&lockConfig);
	if(pthread_create(&sharedConfig->metadataEmitterId, NULL, metadataEmitterRoutine, NULL)!=0)
	{
		pthread_mutex_unlock(&lockConfig);
		fprintf(stderr, "[ERROR] Unable to start metadata emitter\n");
		return (-1);
	}
	pthread_mutex_unlock(&lockConfig);
	return 0;
}

int main ()
{


	/*************** Initialize global variables ***********************/
	if(initializeGlobalVariables() < 0) exit(-1);

	/****************** Initialize configuration link *****************/
	//Launch TCP server to establish metadata link
	fprintf(stdout, "Waiting for metadata link establishment ...\n");
	if(launchMetaDataLink() < 0) exit(-1);
	fprintf(stdout, "Metadata link established.\n");

	/***************** Initialize flow link **************************/
	//Create the socket according to the mode
	fprintf(stdout, "Initializing target, mode : ");
	if(initiateTarget() < 0) exit(-1);
	fprintf(stdout, "Target initialized.\n");
	//Print the configuration
	printConfiguration();

	/*********************** Start target *****************************/
	fprintf(stdout, "Starting target ... \n");
	if(launchTarget() < 0) exit(-1);
	fprintf(stdout, "Target started\n");

	/********************* Start metadata emitter ********************/
	fprintf(stdout, "Starting metada emitter ...\n");
	if(launchMetadataEmitter() < 0) exit(-1);
	fprintf(stdout, "Metadata emitter started.\n");

	/********************** Wait *************************************/
	//Wait until nothing more is to be received
	pthread_join(sharedConfig->targetThreadId, NULL);


	/********************** Exit *************************************/
	//Close, free and exit
	closeSockets();
	free(sharedMetrics);
	free(sharedConfig);
	return(0);
}
