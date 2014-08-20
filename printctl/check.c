#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <fcntl.h>

void usage(char* cmd) {
	fprintf(stderr, "Usage: %s  %s://PATH  NOTIFY-USER-DATA\n", cmd, cmd);
}

int main(int argc, char* argv[]) {
	char* progname;
	char* filename;
	char* cmd;
	int cmdLen;
	int skip;
	int fd;
	int res;
	
	progname = basename(argv[0]);
	skip = strlen(progname) + 3;
	
	if(argc != 3 || strlen(argv[1]) <= skip) {
		usage(progname);
		return 1;
	}
	
	// Determine filename
	filename = argv[1] + skip;
	
	// Determine cmd
	if(argv[2][0] != '\0') {
		cmd = argv[2];
	}
	else {
		cmd = progname;
	}
	
	cmdLen = strlen(cmd);
	
	// Terminate the cmd (overwriting the \0)
	cmd[cmdLen++] = '\n';
	
	// Open - non-blocking and write-only (fails if named pipe has no reader)
	fd = open(filename, O_WRONLY | O_NONBLOCK);
	if(fd < 0) {
		fprintf(stderr, "Can't open file '%s' (%s)\n", filename, strerror(errno));
		return 1;
	}
	
	// Write
	res = write(fd, cmd, cmdLen);
	if(res < 0) {
		fprintf(stderr, "Can't write to file '%s' (%s)\n", filename, strerror(errno));
		return 1;
	}
	else if(res != cmdLen) {
		fprintf(stderr, "Couldn't write command to file '%s'\n", filename);
		return 1;
	}
	
	// Close
	res = close(fd);
	if(res != 0) {
		fprintf(stderr, "Can't close file '%s' (%s)\n", filename, strerror(errno));
		return 1;
	}
	
	return 0;
}