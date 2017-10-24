#include "isocket.h"
#include <stdio.h>

#define PORT 8080
#define CLIENT_QUEUE_L 4

int main ()
{


	/*
	sock = socket(AF_INET, SOCK_STREAM, 0);
	remplir la struct sockaddr_in;
	bind(...);
	listen(...);
	while(1) {
    accept(...);
    send(sock_fd, response, sizeof(response), 0);
    close(...);
    }
    */

    char response[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html><html><head><title>Mon serveur web</title>"
"<body bgcolor=lightred><h1>Hello from my own web server !</h1></body></html>\r\n";

	//Ouverture de la socket
	int fd = i_socket();
	if(fd == -1)
	{
		fprintf(stderr, "Ouverture socket impossible \n");
		exit(1);
	}
	//Association de la socket au port et attente
	if(i_bind(fd, PORT) < 0)
	{
		fprintf(stderr, "Association port %d impossible \n", PORT);
		exit(1);	
	}
	while(1)
	{		
		//Un client demande la connexion -> accepter
		int s2 = i_accept(fd);
		fprintf(stdout, "Nouvelle connexion\n");
		//Envoi de la page html
		send(s2, response, sizeof(response), 0);
		//Deconnection
		close(s2);
		fprintf(stderr, "Deconnexion\n");
		//En attente du prochain client
		//listen(fd,CLIENT_QUEUE_L);
	}


return (0);
}
