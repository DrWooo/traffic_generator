#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include "tda.h"

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
#define DEFAULT_PORT 12345
#define DEFAULT_PROTOCOL USE_TCP
#define DEFAULT_DATARATE 1000000
#define DEFAULT_PACKETSIZE PACKETSIZE_MIN
#define DEFAULT_METADATAPORT 12346


#define HELP_MESSAGE "Traffic Generator. \n Client Programm. \n Use : ./client_echo2 [OPTIONS] \n Options : \n --file <filename> \t Use a file to generate the traffic sent \n --udp \t Use UDP protocol (Not applicable with -t)\n --tcp \t Use TCP protocol (Default)\n --addres \t Server address. Server address to test\n --help \t Print this help\n --pcktSize <size> \t Indicate the packet size in bytes, in range [64,1500]\n --dataRate <data_rate> \t Provide data rate in bits per seconds (only applicable for UDP mode)\n --metPort <port> \t Provide the port used to exchange metadata with the server"

/**
 * \brief Structur used to retrieve the results of the parsing method
 */
typedef struct ParseResults{
	char filename[MAXFILENAMESIZE];
	int useFile;
	int protocol;
	int port;
	char address[MAXADDRESSSIZE];
	int packetSize;
	int dataRate;
	int metadataPort;
} ParseResults_t;

/**
 * \brief Parse Arguments
 * \param[in] argc, the arguments counter
 * \param[in] argv, the arguments values
 * \param[out] results, the results of the parse operation. Shall be allocated by user code.
 * \return 0 on sucess, 
 * 1 to exit programm normally
 * -1 if option is not recognized, 
 * -2 if option is missing, 
 * -3 if incorrect range of a parameter
 */
int parseArguments(int argc, char* argv[], ParseResults_t* results)
{	
	//By default : 
	results->useFile = 0;
	results->protocol = DEFAULT_PROTOCOL;
	results->port = DEFAULT_PORT;
	results->packetSize = DEFAULT_PACKETSIZE;
	results->dataRate = DEFAULT_DATARATE;
	results->metadataPort = DEFAULT_METADATAPORT;
	strcpy(results->address, DEFAULT_ADDRESS);


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
			strcpy(results->filename, argv[i+1]);
			results->useFile = 1;
			//Jump to next arg  because i+1 is the filename
			i++;
			valid = 1;
		}
		/********** Check for UDP/TCP option ****************/
		if(strcmp(UDP_OPTION, argv[i]) == 0)
		{
			results->protocol = USE_UDP;
			valid = 1;
		}
		if(strcmp(TCP_OPTION, argv[i]) == 0)
		{
			results->protocol = USE_TCP;
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
			results->port = atoi(argv[i+1]);
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
			results->metadataPort = atoi(argv[i+1]);
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
			strcpy(results->address, argv[i+1]);
			valid = 1;
			i++;
		}
		/************ Check for DataRate option ********/
		if(strcmp(DATARATE_OPTION, argv[i]) == 0)
		{
			//Check if a next argument is provided
			if((i + 1) >= argc)
			{
				fprintf(stderr, "[ERROR] Please provide a valid address after : \"%s\"\n", ADDRESS_OPTION);
				fprintf(stderr, "Type \"--help\" for help\n");
				return (-2);
			}
			results->dataRate = atoi(argv[i+1]);
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
			results->packetSize = packetSize;
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
 * \param[in]
 */
void printArguments(ParseResults_t myParseResults)
{
	char mode[4];
	if(myParseResults.protocol == USE_UDP)
	{
		strcpy(mode,"UDP");
	}else{
		strcpy(mode,"TCP");
	}
	fprintf(stdout, "Mode : %s\n", mode);
	fprintf(stdout, "Packet size : %dB\n", myParseResults.packetSize);
	if(myParseResults.protocol == USE_UDP)
	{
		fprintf(stdout, "Emission datarate : %dbps\n", myParseResults.dataRate);
	}
	fprintf(stdout, "Address : %s\n", myParseResults.address);
	fprintf(stdout, "Port tested : %d\n", myParseResults.port);
	fprintf(stdout, "Metadata port : %d\n", myParseResults.metadataPort);
	if(myParseResults.useFile)
	{
		fprintf(stdout, "Use file : %s\n", myParseResults.filename);
	}
}

int connectToMetadataServer(ParseResults_t myParseResults)
{
	int sz = 1;
	int fd;
	struct sockaddr_in portname;
	struct hostent *hp;

	//Obtention de la socket
	fd = socket (PF_INET, SOCK_STREAM, 0);
  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &sz, 4);
  
  //Configuration de la structure
  hp = gethostbyname (myParseResults.address);
  if (hp == NULL)
  {
  	fprintf(stderr, "[ERROR] Unable to translate address : %s\n", myParseResults.address);
    return (-1);
  }
  bzero (&portname, sizeof portname);
  portname.sin_port = htons (myParseResults.metadataPort);
  portname.sin_family = AF_INET;
  memcpy(&portname.sin_addr.s_addr, hp->h_addr_list[0], sizeof(portname.sin_addr.s_addr));

  //Tentative de connexion
  if(connect (fd, (struct sockaddr *) &portname, sizeof portname) !=0)
  {
  	fprintf(stderr, "[ERROR] Unable to connect metadata link to server %s:%d\n", myParseResults.address, myParseResults.metadataPort);
  	return (-1);
  }
  return(fd);
}

int launchMetadata(ParseResults_t myParseResults)
{
	int fd = connectToMetadataServer(myParseResults);
	if(fd < 0) return(-1);
	return(0);
}

int main (int argc, char* argv[])
{
	/************ Retrieve and print arguments ****************/
	ParseResults_t myParseResults;
	int retcode = parseArguments(argc, argv, &myParseResults);
	if(retcode < 0)	exit(-1);
	if(retcode > 0) exit(0);
	printArguments(myParseResults);

	/*********** Launch the data acquisition *****************/
	if(launchMetadata(myParseResults)<0) exit(-1);

	/*********** Launch the data flow ************************/
	/*
	if(myParseResults.protocol == USE_TCP)
	{
		if(launchDataFlowTcp(myParseResults)<0) exit(-1);
	}else{
		if(launchDataFlowUdp(myParseResults)<0) exit(-1);
	}
	*/


return (0);
}
