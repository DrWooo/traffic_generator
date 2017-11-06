/*!
 * Client Echo 2
 *
 *
 * NOTE : La fonction randomLetter est copiée du travail de Pierre Lemarquand
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include "tda.h"
#include <pthread.h>

#define DOMAIN PF_INET
#define TYPE SOCK_STREAM
#define FILE_OPTION "--file"
#define UDP_OPTION "--udp"
#define TCP_OPTION "--tcp"
#define PORT_OPTION "--port"
#define ADDRESS_OPTION "--address"
#define HELP_OPTION "--help"
#define PACKETSIZE_OPTION "--pcktSize"
#define DATARATE_OPTION "--dataRate"
#define METADATAPORT_OPTION "--metPort"
#define PACKETSIZE_MAX 1500
#define PACKETSIZE_MIN 64
#define MAXFILENAMESIZE 50
#define MAXADDRESSSIZE 50
#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_SERVERFLOWPORT 12345
#define DEFAULT_CLIENTFLOWPORT 12344
#define DEFAULT_PROTOCOL USE_TCP
#define DEFAULT_DATARATE 1000000
#define DEFAULT_PACKETSIZE PACKETSIZE_MIN
#define DEFAULT_METADATAPORT 12346
 


#define HELP_MESSAGE "Traffic Generator. \n Client Programm. \n Use : ./client_echo2 [OPTIONS] \n Options : \n --file <filename> \t Use a file to generate the traffic sent \n --udp \t Use UDP protocol (Not applicable with -t)\n --tcp \t Use TCP protocol (Default)\n --addres \t Server address. Server address to test\n --help \t Print this help\n --pcktSize <size> \t Indicate the packet size in bytes, in range [64,1500]\n --dataRate <data_rate> \t Provide data rate in bits per seconds (only applicable for UDP mode)\n --metPort <port> \t Provide the port used to exchange metadata with the server"

typedef struct configuration{
	char filename[MAXFILENAMESIZE];
	int useFile;
	FILE* fileDescriptor;
	int protocol;
	int serverFlowPort;
	int clientFlowPort;
	char address[MAXADDRESSSIZE];
	int packetSize;
	int dataRate;
	int metadataPort;
	pthread_t sendingThread;
	pthread_t dataPrinterThread;
	int metadataLinkSocketDescriptor;
	int flowLinkSocketDescriptor;
} configuration_t;

configuration_t* sharedConfig;
pthread_mutex_t lock;


/*! 
 * \brief Returns random capital letter
 * \return a letter
 */
char randomLetter() { 
  char res;
  res = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[random () % 26];
  return res;
}
/*!
 * \brief Initiates the global variables
 * \return 0 on succes, non-zero value on failure
 */
int initializeGlobalVariables(void)
{
	pthread_mutex_init(&lock, NULL);
	sharedConfig = (configuration_t*) malloc(sizeof(configuration_t));
	if(sharedConfig == NULL)
	{
		fprintf(stderr, "[ERROR] Not enough memory to start client\n");
		return (-1);
	}
	sharedConfig->useFile = 0;
	sharedConfig->protocol = DEFAULT_PROTOCOL;
	sharedConfig->serverFlowPort = DEFAULT_SERVERFLOWPORT;
	sharedConfig->clientFlowPort = DEFAULT_CLIENTFLOWPORT;
	sharedConfig->packetSize = DEFAULT_PACKETSIZE;
	sharedConfig->dataRate = DEFAULT_DATARATE;
	sharedConfig->metadataPort = DEFAULT_METADATAPORT;
	strcpy(sharedConfig->address, DEFAULT_ADDRESS);
	return 0;
}

/**
 * \brief Parse Arguments
 * Stores the results in sharedConfig (unprotected)
 * \param[in] argc, the arguments counter
 * \param[in] argv, the arguments values
 * \return 0 on sucess, 
 * 1 to exit programm normally
 * -1 if option is not recognized, 
 * -2 if option is missing, 
 * -3 if incorrect range of a parameter
 */
int parseArguments(int argc, char* argv[])
{	
	//For each argument except the first one (programm name)
	for(int i = 1; i < argc; i++)
	{
		int valid = 0;
		/********** Check for file option ********************/
		if(strcmp(FILE_OPTION, argv[i]) == 0)
		{
			//Check if a next argument is provided
			if((i + 1) >= argc)
			{
				fprintf(stderr, "[ERROR] Please provide a valid filename after : \"%s\"\n", FILE_OPTION);
				fprintf(stderr, "Type \"--help\" for help\n");
				return (-2);
			}
			strcpy(sharedConfig->filename, argv[i+1]);
			sharedConfig->useFile = 1;
			//Jump to next arg  because i+1 is the filename
			i++;
			valid = 1;
		}
		/********** Check for UDP/TCP option ****************/
		if(strcmp(UDP_OPTION, argv[i]) == 0)
		{
			sharedConfig->protocol = USE_UDP;
			valid = 1;
		}
		if(strcmp(TCP_OPTION, argv[i]) == 0)
		{
			sharedConfig->protocol = USE_TCP;
			valid = 1;
		}
		/*********** Check for port option *****************/
		if(strcmp(PORT_OPTION, argv[i]) == 0)
		{
			//Check if a next argument is provided
			if((i + 1) >= argc)
			{
				fprintf(stderr, "[ERROR] Please provide a valid port after : \"%s\"\n", PORT_OPTION);
				fprintf(stderr, "Type \"--help\" for help\n");
				return (-2);
			}
			sharedConfig->serverFlowPort = atoi(argv[i+1]);
			valid = 1;
			//Jump to next argument as i+1 is the port number
			i++;
		}
		/************ Check for metadata port option ******/
		if(strcmp(METADATAPORT_OPTION, argv[i]) == 0)
		{
			//Check if a next argument is provided
			if((i + 1) >= argc)
			{
				fprintf(stderr, "[ERROR] Please provide a valid port after : \"%s\"\n", METADATAPORT_OPTION);
				fprintf(stderr, "Type \"--help\" for help\n");
				return (-2);
			}
			sharedConfig->metadataPort = atoi(argv[i+1]);
			valid = 1;
			//Jump to next argument as i+1 is the port number
			i++;
		}

		/*********** Check for address option *************/
		if(strcmp(ADDRESS_OPTION, argv[i]) == 0)
		{
			//Check if a next argument is provided
			if((i + 1) >= argc)
			{
				fprintf(stderr, "[ERROR] Please provide a valid address after : \"%s\"\n", ADDRESS_OPTION);
				fprintf(stderr, "Type \"--help\" for help\n");
				return (-2);
			}
			strcpy(sharedConfig->address, argv[i+1]);
			valid = 1;
			i++;
		}
		/************ Check for DataRate option ********/
		if(strcmp(DATARATE_OPTION, argv[i]) == 0)
		{
			//Check if a next argument is provided
			if((i + 1) >= argc)
			{
				fprintf(stderr, "[ERROR] Please provide a valid datarate after : \"%s\"\n", DATARATE_OPTION);
				fprintf(stderr, "Type \"--help\" for help\n");
				return (-2);
			}
			sharedConfig->dataRate = atoi(argv[i+1]);
			i++;
			valid = 1;
		}
		/*********** Check for packet size option ******/
		if(strcmp(PACKETSIZE_OPTION, argv[i]) == 0)
		{
			if((i + 1) >= argc)
			{
				fprintf(stderr, "[ERROR] Please provide a valid packet size after : \"%s\"\n", PACKETSIZE_OPTION);
				fprintf(stderr, "Type \"--help\" for help\n");
				return (-2);
			}
			int packetSize = atoi(argv[i+1]);
			if (packetSize < PACKETSIZE_MIN || packetSize > PACKETSIZE_MAX)
			{
				fprintf(stderr, "[ERROR] Please provide a packet size between %d and %d\n", PACKETSIZE_MIN, PACKETSIZE_MAX);
				fprintf(stderr, "Type \"--help\" for help\n");
				return(-3);
			}
			sharedConfig->packetSize = packetSize;
			valid = 1;
			i++;
		}
		/*********** Check for help option *************/
		if(strcmp(HELP_OPTION, argv[i]) == 0)
		{
			fprintf(stderr, "%s\n", HELP_MESSAGE);
			valid = 1;
			return(1);
		}
		if(!valid)
		{
				fprintf(stderr, "[ERROR] Unrecognized argument : \"%s\"\n", argv[i]); 
				fprintf(stderr, "%s\n", "Type \"--help\" for help\n");
				return(-1);
		}
	}
	return 0;
}

/**
 * \brief Print passed parameters
 * Reads from sharedConfig (unprotected)
 */
void printArguments(void)
{
	char mode[4];
	if(sharedConfig->protocol == USE_UDP)
	{
		strcpy(mode,"UDP");
	}else{
		strcpy(mode,"TCP");
	}
	fprintf(stdout, "Mode : %s\n", mode);
	fprintf(stdout, "Packet size : %dB\n", sharedConfig->packetSize);
	if(sharedConfig->protocol == USE_UDP)
	{
		fprintf(stdout, "Emission datarate : %dbps\n", sharedConfig->dataRate);
	}
	fprintf(stdout, "Address : %s\n", sharedConfig->address);
	fprintf(stdout, "Port tested : %d\n", sharedConfig->serverFlowPort);
	fprintf(stdout, "Metadata port : %d\n", sharedConfig->metadataPort);
	if(sharedConfig->useFile)
	{
		fprintf(stdout, "Use file : %s\n", sharedConfig->filename);
	}
}

/*!
 * \brief Open a tcp link with the server to exchange metadata
 * Reads from and Writes in sharedConfig (unprotected)
 * \return 0 on succes, non-zero value on failure
 */
int connectToMetadataServer()
{
	int sz = 1;
	int sd;
	struct sockaddr_in portname;
	struct hostent *hp;

	//Obtention de la socket
	sd = socket (PF_INET, SOCK_STREAM, 0);
  if(sd < 0)
  {
  	perror("socket@initiateTarget");
  	return(-1);
  }
  setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &sz, 4);
  sharedConfig->metadataLinkSocketDescriptor = sd;

  //Configuration de la structure
  hp = gethostbyname (sharedConfig->address);
  if (hp == NULL)
  {
  	fprintf(stderr, "[ERROR] Unable to translate address : %s\n", sharedConfig->address);
  	perror("gethostbyname@connectToMetadataServer");
    return (-1);
  }
  bzero (&portname, sizeof portname);
  portname.sin_port = htons (sharedConfig->metadataPort);
  portname.sin_family = AF_INET;
  memcpy(&portname.sin_addr.s_addr, hp->h_addr_list[0], sizeof(portname.sin_addr.s_addr));

  //Tentative de connexion
  if(connect(sd, (struct sockaddr *) &portname, sizeof portname) !=0)
  {
  	perror("connect@connectToMetadataServer");
  	return (-1);
  }
  return(0);
}

/*!
 * \brief Initiates the link with the server and tranfers to it
 * usefull information about the test about to start
 * Reads from sharedConfig (unprotected)
 * \return 0 on succes, non-zero value on failure
 */
int launchMetadata()
{
	if(connectToMetadataServer()<0) return (-1);
	configurationPacket_t* initiator = (configurationPacket_t*) malloc(sizeof(configurationPacket_t));
	if(initiator == NULL)
	{
		fprintf(stderr, "Unable to allocate memory for the initiator packet\n");
		return(-1);
	}
	initiator->mode = sharedConfig->protocol;
	initiator->serverFlowPort = DEFAULT_SERVERFLOWPORT;
	initiator->clientFlowPort = DEFAULT_CLIENTFLOWPORT;
	send(sharedConfig->metadataLinkSocketDescriptor, initiator, sizeof(metadataPacket_t), 0);
	free(initiator);
	return(0);
}

/*!
 * \brief Fill the memory from buffer to buffer + size - 2
 * with readeable characters, depending on the mode specified by sharedConfig->useFile
 * Fill  buffer + size - 1 with \0
 * Reads from sharedConfig (protected)
 * \param buffer : the pointer to the first char
 * \param size : number of char to create/read from file
 * \return 0 on succes, -1 on failure, 1 when nothing more is to be read out of the file (if applicable).
 */
int getContent(char* buffer, int size)
{
	int fromFile;
	FILE* tempFile;
	pthread_mutex_lock(&lock);
	fromFile = sharedConfig->useFile;
	tempFile = sharedConfig->fileDescriptor;
	pthread_mutex_unlock(&lock);
	if(fromFile)
	{
		int temp;
		if((temp = fgetc(tempFile)) == EOF) return 1;
		*buffer = (char) temp;
		for(char* index = buffer + 1; index < (buffer + size - 1); index ++)
		{
			temp = fgetc(tempFile);
			if(temp == EOF)
			{
				*index = '\0';
				return 0;
			}else{
				*index = (char) temp;
			}
		}
		*(buffer + size - 1) = '\0';
	}else{ 
		for(char* index = buffer; index < (buffer + size - 1); index++)
		{
			*index = randomLetter();
		}
		*(buffer + size - 1) = '\0';
	} 
	return 0;
}

/*!
 * \briefs Routine to be used in a different thread that generates tcp flow
 * Reads data from sharedConfig (protected)
 * \param noArgs : unused
 * \return NULL
 */
void* sendTcpRoutine(void* noArgs)
{
	pthread_mutex_lock(&lock);
	int fd = sharedConfig->flowLinkSocketDescriptor;
	int size = sharedConfig->packetSize;
	pthread_mutex_unlock(&lock);

	//Get a packet
	dataPacket_t* packet;
	packet = (dataPacket_t*) malloc(sizeof(dataPacket_t));
	if(packet == NULL)
	{
		fprintf(stderr, "[ERROR] Unable to get memory for flow packet (tcp send routine)\n");
		pthread_exit(NULL);
		return NULL;
	}
	//First index
	packet->index = 0;
	//Set the size
	packet->size = size;
	//Allocate place for the content
	packet->content = (char*) malloc(size -sizeof(dataPacket_t) - TCPIP_HEADERSIZE);
	//To store the number of bytes sent
	int sent = 0;
	//Buffer that stores all the content of what is to be sent
	char buffer[BUFFERS_SIZE];
	while(1)
  	{
  		packet->index++;
  		//Get the content
  		if(getContent(packet->content, size - sizeof(dataPacket_t) - TCPIP_HEADERSIZE) != 0) break;
  		//Preparing to send trough a buffer (char)
  		//First, copy the packet at the beginning of the buffer
  		memcpy(buffer, packet, sizeof(dataPacket_t));
  		//Then, copy the content
  		memcpy(buffer + sizeof(dataPacket_t), packet->content, packet->size - sizeof(dataPacket_t) - TCPIP_HEADERSIZE);
  		sent = send(fd, buffer, size - TCPIP_HEADERSIZE, 0);

  		//Print for debug
  		TRACE_LOG("Sent : %d. Expected : %lu, Index: %lu, Content : %s\n",sent, packet->size - TCPIP_HEADERSIZE, packet->index, packet->content);
  	}
  	free(packet->content);
  	free(packet);
  	pthread_exit(NULL);
  	return NULL;
}


/*!
 * \brief Starts the tcp data flow
 * This function performs a call to pthread_create to create the routinr used to
 * generate the traffic.
 * Reads and write from and to sharedConfig (unprotected) before the pthread_create call
 */
int launchDataFlowTcp(void)
{
	//Prepare the socket
	int sz = 1;
	int fd;
	struct sockaddr_in portname;
	struct hostent *hp;

	//Get the socket
	fd = socket (PF_INET, SOCK_STREAM, 0);
	if(fd < 0)
  {
  	perror("socket@launchMetaDataLink");
  	return(-1);
  }
	int result;
  result = setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &sz, 4);
  if(result < 0)
  {
  	fprintf(stderr, "[ERROR] Error while setting options for socket\n");
  }
 	//Disable naggling to keep packets aligned
 	int flag = 1;
  result = setsockopt(fd, IPPROTO_TCP,TCP_NODELAY, (char*) &flag, sizeof(int));
  if(result < 0)
  {
  	fprintf(stderr, "[ERROR] Error while setting options for socket\n");
  }
  //Store the socket identifier
  sharedConfig->flowLinkSocketDescriptor = fd;
  //Configuration de la structure pour connexion
  hp = gethostbyname (sharedConfig->address);
  if (hp == NULL)
  {
  	fprintf(stderr, "[ERROR] Unable to translate address : %s\n", sharedConfig->address);
  	perror("gethostbyname@launchDataFlowTcp");
    return (-1);
  }
  bzero (&portname, sizeof portname);
  portname.sin_port = htons (sharedConfig->serverFlowPort);
  portname.sin_family = AF_INET;
  memcpy(&portname.sin_addr.s_addr, hp->h_addr_list[0], sizeof(portname.sin_addr.s_addr));

  //Tentative de connexion
  if(connect (fd, (struct sockaddr *) &portname, sizeof portname) !=0)
  {
  	perror("connect@launchDataFlowTcp");
  	return (-1);
  }

  pthread_mutex_lock(&lock);
  if(pthread_create(&(sharedConfig->sendingThread), NULL, sendTcpRoutine, NULL) !=0){
  	fprintf(stderr, "[ERROR] Unable to start the sending thread\n");
  	pthread_mutex_unlock(&lock);
  	return (-1);
  }
  pthread_mutex_unlock(&lock);
  return 0;


}

/*!
 * \brief Waits the specified time
 * \param [in] microsec: time to wait in microsec
 */
void waitUs (int microsec)
{
  struct timeval tv;
  tv.tv_sec = microsec / 1000000;
  tv.tv_usec = (microsec - (tv.tv_sec * 1000000));       
  select (0, NULL, NULL, NULL, &tv);
}

void* waitRoutine(void* value)
{
	int* toWait = (int*) value;
	waitUs(*toWait);
	pthread_exit(NULL);
	return NULL;
}


void* sendUdpRoutine(void* noArgs)
{
	struct sockaddr_in to;
	struct hostent *hp;
	int toWaitUs;
	pthread_t waitThread;
	int size;
	int sd;
	pthread_mutex_lock(&lock);
	size = sharedConfig->packetSize;
	sd = sharedConfig->flowLinkSocketDescriptor;
  hp = gethostbyname (sharedConfig->address);
  if (hp == NULL)
  {
  	fprintf(stderr, "[ERROR] Unable to translate address : %s\n", sharedConfig->address);
  	perror("gethostbyname@sendUdpRoutine");
  	pthread_mutex_unlock(&lock);
  	pthread_exit(NULL);
    return NULL;
  }
  bzero (&to, sizeof to);
  to.sin_port = htons (sharedConfig->serverFlowPort);
  to.sin_family = AF_INET;
  memcpy(&to.sin_addr.s_addr, hp->h_addr_list[0], sizeof(to.sin_addr.s_addr));
  toWaitUs = 1000000 * 8 * sharedConfig->packetSize / (sharedConfig->dataRate) ;
  pthread_mutex_unlock(&lock);

  //Get a buffer
  char buffer[BUFFERS_SIZE];
	dataPacket_t* packet;
	packet = (dataPacket_t*) buffer;
	if(packet == NULL)
	{
		fprintf(stderr, "[ERROR] Cast error (tcp send routine)\n");
		pthread_exit(NULL);
		return NULL;
	}
	//First index
	packet->index = 0;
	//Set the size
	packet->size = size;
	//Link
	packet->content = buffer + sizeof(dataPacket_t);
	//To store the number of bytes sent
	int sent = 0;
	while(1)
	{
		//Start timer
		if(pthread_create(&waitThread, NULL, waitRoutine, &toWaitUs) !=0) break;
		packet->index++;
		//Get the content, place if after packet
		if(getContent(packet->content, size - sizeof(dataPacket_t) - UDPIP_HEADERSIZE) != 0) break;
		//Preparing to send trough a buffer (char).
		//NOTE : Here, everything is performed inside buffer[], so no copy is necessary (speeding up the routine).
		sent = sendto(sd, buffer, size - UDPIP_HEADERSIZE, 0, (struct sockaddr *) &to, sizeof to);
		if(sent < 0)
		{
			perror("sendto@sendUdpRoutine");
			pthread_exit(NULL);
			return NULL;
		}
		//Print for debug
		TRACE_LOG("Sent : %d. Expected : %lu, Index: %lu, Content : %s\n",sent, packet->size - UDPIP_HEADERSIZE, packet->index, packet->content);
		pthread_join(waitThread, NULL);
	}
	free(packet->content);
	free(packet);
	pthread_exit(NULL);
	return NULL;

}


int launchDataFlowUdp(void)
{
	//Prepare the socket
	int sz = 1;
	int fd;

	//Get the socket
	fd = socket (PF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
  {
  	perror("socket@launchMetaDataLink");
  	return(-1);
  }
	int result;
  result = setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &sz, 4);
  if(result < 0)
  {
  	fprintf(stderr, "[ERROR] Error while setting options for socket\n");
  }
  pthread_mutex_lock(&lock);
  sharedConfig->flowLinkSocketDescriptor = fd;
  if(pthread_create(&(sharedConfig->sendingThread), NULL, sendUdpRoutine, NULL) !=0){
  	fprintf(stderr, "[ERROR] Unable to start the sending thread\n");
  	pthread_mutex_unlock(&lock);
  	return (-1);
  }
  pthread_mutex_unlock(&lock);
  return 0;
}

/*!
 * \briefs Open the file an store its descriptor on sharedConfig
 * \return 0 on succes, non-zero value on error
 */
int openFile(void)
{
	sharedConfig->fileDescriptor = fopen(sharedConfig->filename, "r");
	if(sharedConfig->fileDescriptor == NULL)
	{
		perror("fopen@openFile");
		return (-1);
	}
	return (0);
}

void* dataPrinterRoutine(void* noArgs)
{
	char buffer[BUFFERS_SIZE];
	metadataPacket_t* stats;
	int received;
	int sd;
	int pktSize;
	unsigned long lastIndex;
	double dataRatembps;
	double relativeLost;
	pthread_mutex_lock(&lock);
	sd = sharedConfig->metadataLinkSocketDescriptor;
	pktSize = sharedConfig->packetSize;
	pthread_mutex_unlock(&lock);
	while(1)
	{
		received = recv(sd, buffer, sizeof(metadataPacket_t), 0);
		if(received < 0)
		{
			perror("recv@dataPrinterRoutine");
			return NULL;
		}
		stats = (metadataPacket_t*) buffer;
		if(stats == NULL)
		{
			fprintf(stderr, "[ERROR] Cast error on data printer routine\n");
			return NULL;
		}
		dataRatembps = ((double) stats->receivedOverLastPeriod) * pktSize * 8 / (COMMON_PERIOD_MS * 1000);
		relativeLost = ((double) stats->lostOverLastPeriod) / (stats->lastIndexReceived - lastIndex);
		lastIndex = stats->lastIndexReceived;
		fprintf(stdout, "Débit : %.2f Mb/s. Perte : %.2f %% \n",dataRatembps, relativeLost*100);
	}
}

int launchDataPrinter(void)
{
	pthread_mutex_lock(&lock);
  if(pthread_create(&(sharedConfig->dataPrinterThread), NULL, dataPrinterRoutine, NULL) !=0){
  	fprintf(stderr, "[ERROR] Unable to start the sending thread\n");
  	pthread_mutex_unlock(&lock);
  	return (-1);
  }
  pthread_mutex_unlock(&lock);
  return 0;
}




int main (int argc, char* argv[])
{
	/************* Initialize global variables ****************/
	if(initializeGlobalVariables() < 0) exit(-1);
	/************ Retrieve and print arguments ****************/
	int retcode = parseArguments(argc, argv);
	if(retcode < 0)	exit(-1);
	if(retcode > 0) exit(0);
	printArguments();
	if(sharedConfig->useFile)
	{
		if(openFile() < 0) exit(-1);;
	}

	/*********** Launch the metadata link *****************/
	if(launchMetadata()<0) exit(-1); 
	//Wait some time for the server to open the socket
	waitUs(100000);

	/*********** Launch the data flow ************************/
	if(sharedConfig->protocol == USE_TCP)
	{
		if(launchDataFlowTcp()<0) exit(-1);
	}else{
		if(launchDataFlowUdp()<0) exit(-1);
	}
	/************ Launch the data printer ********************/
	if(launchDataPrinter() < 0) exit(-1);

	pthread_t toJoin;
	pthread_mutex_lock(&lock);
	toJoin = sharedConfig->sendingThread;
	pthread_mutex_unlock(&lock);

	pthread_join(toJoin, NULL);

	pthread_mutex_lock(&lock);
	if(sharedConfig->useFile)
	{
		fclose(sharedConfig->fileDescriptor);
	}
	pthread_mutex_unlock(&lock);
	free(sharedConfig);
	return (0);
}
