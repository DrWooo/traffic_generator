#ifndef TDA_H
#define TDA_H

#include <stddef.h>

#define COMMON_PERIOD_MS 1000
#define USE_UNDEFINED 0
#define USE_UDP 1
#define USE_TCP 2
#define MESSAGE_DATASIZE 500
#define TCPIP_HEADERSIZE 40
#define UDPIP_HEADERSIZE 28
#define BUFFERS_SIZE 1500
#ifdef LOG_DEBUG_LEVEL
#define TRACE_LOG(fmt, args...) fprintf(stdout, fmt, ##args);
#else
#define TRACE_LOG(fmt, args...)
#endif

typedef struct configurationPacket
{
	int mode;
	int serverFlowPort;
	int clientFlowPort;
} configurationPacket_t;

typedef struct metadataPacket{
	unsigned long lostOverLastPeriod;
	unsigned long receivedOverLastPeriod;
	unsigned long lastIndexReceived;
} metadataPacket_t;

typedef struct dataPacket{
	unsigned long index;
	size_t size;
	char* content;
} dataPacket_t;






#endif
