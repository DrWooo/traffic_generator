#ifndef TDA_H
#define TDA_H

#define COMMON_PERIOD_MS 1000
#define USE_UNDEFINED 0
#define USE_UDP 1
#define USE_TCP 2

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
	int size;
	char buffer[1500];
} dataPacket_t;






#endif
