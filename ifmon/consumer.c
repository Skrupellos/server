#include <stdio.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/wait.h>
#include "consumer.h"
#include "util.h"


int start();

pid_t        pid = 0;
int          commpipe[2];
const char * cmd;


int
consumerConsume(const struct ConsumerMessage * msg) {
	int rv;
	
	if((rv = start()) != EXIT_SUCCESS) {
		return rv;
	}
	
	dprintf(commpipe[1], "%s %s IPv%i %s\n",
		msg->name,
		msg->removed ? "del" : "new",
		msg->version, msg->ip
	);
	
	return EXIT_SUCCESS;
}


int
consumerStop() {
	int rv;
	
	if(pid <= 0) {
		return EXIT_SUCCESS;
	}
	
	pid = 0;
	
	if(close(commpipe[1]) != 0) {
		PRINT_ERRNO("Can't close pipe to consumer while stopping consumption");
	}
	
	if(wait(&rv) <= 0) {
		RET_ERRNO("Failed waiting for consumer to exit");
	}
	
	if(rv != EXIT_SUCCESS) {
		PRINT_ERR("Consumer did not exit successfully with exit code %i", rv);
	}
	
	return rv;
}


void
consumerShutdown() {
	consumerStop();
}


int
consumerConfig(const char *  path) {
	cmd = path;
	return EXIT_SUCCESS;
}


int
start() {
	if(pid > 0) {
		return EXIT_SUCCESS;
	}
	
	// Setup communication pipeline first
	if(pipe(commpipe) != 0) {
		RET_ERRNO("Can't create pipe");
	}

	// Attempt to fork and check for errors
	if((pid=fork()) < 0) {
		RET_ERRNO("Can't spoon");
	}
	else if(pid > 0){
		// PARENT
		close(commpipe[0]);
		setvbuf(stdout, (char*)NULL, _IONBF,0); // Set non-buffered output on stdout
	}
	else{
		// CHILD
		dup2(commpipe[0], 0);
		close(commpipe[1]);
		execlp(cmd, cmd, NULL);
		EXIT_ERRNO("Failed to start consumer");
	}
	
	return EXIT_SUCCESS;
}
