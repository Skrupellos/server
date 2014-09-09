#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include "main.h"
#include "consumer.h"
#include "monitor.h"
#include "util.h"

char * progName;

void usage();

void
usage() {
	PRINT_ERR("Usage: %s SCRIPT", progName);
}


int
ifmonMain(int argc, char * argv[]) {
	progName = basename(argv[0]);
	
	if(argc != 2) {
		usage();
		return 1;
	}
	
	consumerConfig(argv[1]);
	
	monitorRun();
	
	consumerShutdown();
	
	return EXIT_SUCCESS;
}
