#ifndef _TRAFFIC_GENERATOR_LIB_H_
#define _TRAFFIC_GENERATOR_LIB_H_


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

/**
 * Get a socket for the specified protocol type
 * \param[in] type : SOCK_STREAM for TCP oriented socket or SOCK_DGRAM for UDP oriented socket
 * \return socket descriptor
 */
int getSocket(int type);

#endif