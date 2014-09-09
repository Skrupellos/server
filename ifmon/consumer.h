#ifndef __CONSUMER_H
#define __CONSUMER_H 
#include <net/if.h>
#include <stdbool.h>
#include <arpa/inet.h>



struct ConsumerMessage {
	char name[IFNAMSIZ];
	char ip[INET6_ADDRSTRLEN];
	unsigned char version;
	bool removed;
};

int  consumerConsume(const struct ConsumerMessage * msg);
int  consumerStop();
void consumerShutdown();
int  consumerConfig(const char *  path);

#endif
