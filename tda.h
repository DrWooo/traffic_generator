#ifndef TDA_H
#define TDA_H

#define COMMON_PERIOD_MS 1000
#define USE_UNDEFINED 0
#define USE_UDP 1
#define USE_TCP 2

typedef struct metadataPacket{
	int mode;
	unsigned long lostOverLastPeriod;
	unsigned long receivedOverLastPeriod;
	unsigned long lastIndexReceived;
} metadataPacket_t;

typedef struct dataPacket{
	unsigned long index;
	int size;
} dataPacket_t;
#endif