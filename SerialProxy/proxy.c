#define _GNU_SOURCE

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>

#define AMBILIGHT 0
#define HEYU 1

#define NUM_PORTS 2
#define REAL_PORT "/dev/ttyUSB0"


void print_msg(char *buff, int len) {
	int i = 0;
	for(i=0;i<len;i++)
		printf("%x ", (unsigned char)buff[i]);
	printf("\n");
}

void fatalErrorExit(int error, char *message) {
	perror(message);
	perror(strerror(error));
	exit(1);
}

int main(int argc, char **argv) {
	char buffer[64];
	int buffSize = 64;
	int sps[NUM_PORTS] = {-1};
	int active_port = 0;
	int i = 0,rc=0, wc=0,maxfd = 0, rp;
	struct termios options;
	int flowcontrol = 0;

	fd_set socks, fullsocks;
	FD_ZERO(&fullsocks);

	for(i=0;i<NUM_PORTS;i++) {
		if ((sps[i] = getpt()) == -1)
			fatalErrorExit(errno, "cannot open stream");
		if (grantpt(sps[i]) == -1)
			fatalErrorExit(errno, "cannot grant stream");
		if (unlockpt(sps[i]) == -1)
			fatalErrorExit(errno, "cannot unlock stream");
		printf("Parent role: child endpoint at %s\n", ptsname(sps[i]));
		if (sps[i] > maxfd)
			maxfd = sps[i];

		FD_SET(sps[i],&fullsocks);
	}


	rp = open(REAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
	if (rp == -1) {
		fatalErrorExit(errno, "cannot open real port");
	}

	tcgetattr(rp, &options);

    	cfsetispeed(&options, B57600);
    	cfsetospeed(&options, B57600);

  	options.c_cflag |= (CLOCAL | CREAD);

    	tcsetattr(rp, TCSANOW, &options);

	if (rp > maxfd)
		maxfd = rp;
	FD_SET(rp, &fullsocks);

	active_port = HEYU;

	while (1) {
		FD_ZERO(&socks);
		memcpy(&socks, &fullsocks, sizeof(fd_set));
		
		rc = select(maxfd+1, &socks, NULL, NULL, NULL);
		if (FD_ISSET(rp, &socks)) { // Receive from the "REAL" port
			rc = read(rp, buffer, 1);
			if (rc > 0) {
				if (buffer[0] == 0x55)
					flowcontrol = 0;
	
				rc = write(sps[active_port], buffer, rc);
				//printf("USB %d: ", rc);
				//print_msg(buffer, rc);
			}
		}
		for(i=0;i<NUM_PORTS;i++) {
			if (FD_ISSET(sps[i], &socks)) {
				rc = read(sps[i], buffer, buffSize);
				if (rc > 0) {
					if (i != active_port && flowcontrol == 1) {
						printf("Packet dropped\n");
						continue;
					} else if (i == active_port)
						flowcontrol = 1;
					//printf("%d, %d: ", i, rc);
					//print_msg(buffer, rc);
					wc = write(rp, buffer, rc);
				}
			}
		}
	}

	close(rp);
	return 0;
}
