#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DOMAIN PF_INET
#define TYPE SOCK_STREAM
#define FILE_OPTION "--file"
#define UDP_OPTION "--udp"
#define TCP_OPTION "--tcp"
#define PORT_OPTION "--port"
#define ADDRESS_OPTION "--address"
#define HELP_OPTION "--help"
#define MAXFILENAMESIZE 50
#define MAXADDRESSSIZE 50
#define USE_UDP 1
#define USE_TCP 2
#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 12345
#define DEFAULT_PROTOCOL USE_TCP

#define HELP_MESSAGE "Traffic Generator. \n Client Programm. \n Use : ./client_echo2 [OPTIONS] \n Options : \n --file <filename> \t Use a file to generate the traffic sent \n --udp \t Use UDP protocol (Not applicable with -t)\n --tcp \t Use TCP protocol (Default)\n --addres \t Server address. Server address to test\n --help \t Print this help\n"

/**
 * \brief Structur used to retrieve the results of the parsing method
 */
typedef struct ParseResults{
	char filename[MAXFILENAMESIZE];
	int useFile;
	int protocol;
	int port;
	char address[MAXADDRESSSIZE];
} ParseResults_t;

/**
 * \brief Parse Arguments
 * \param[in] argc, the arguments counter
 * \param[in] argv, the arguments values
 * \param[out] results, the results of the parse operation. Shall be allocated by user code.
 * \return 0 on sucess, -1 if option is not recognized, -2 if option is missing
 */
int DoParseArguments(int argc, char* argv[], ParseResults_t* results)
{	
	//By default : 
	results->useFile = 0;
	results->protocol = DEFAULT_PROTOCOL;
	results->port = DEFAULT_PORT;
	results->address = DEFAULT_ADDRESS;

	
	strcpy(results->filename, argv[i+1]);
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
			strcpy(results->address, argv[i]);
			valid = 1;
			i++;
		}
		/*********** Check for help option *************/
		if(strcmp(HELP_OPTION, argv[i]) == 0)
		{
			fprintf(stderr, "%s\n", HELP_MESSAGE);
			valid = 1;
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

int main (int argc, char* argv[])
{
	ParseResults_t myParseResults;
	if(DoParseArguments(argc, argv, &myParseResults) < 0)
	{
		exit(-1);
	}
return (0);
}
