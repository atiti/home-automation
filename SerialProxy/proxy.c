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

#define BUFFER_SIZE 128

#define AMBILIGHT 0
#define HEYU 1

#define NUM_PORTS 2
#define REAL_PORT "/dev/ttyACM1"

char *portnames[NUM_PORTS] = { "Boblight", "HeyU" };
char portpriority[NUM_PORTS] = { 0, 1 };

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
	unsigned char buffer[BUFFER_SIZE];
	int buffSize = BUFFER_SIZE;
	int sps[NUM_PORTS] = {-1};
	int active_port = 0;
	int i = 0,rc=0, wc=0,maxfd = 0, rp;
	int ready = 1;
	struct termios options;
	int flow_control = 0;

	fd_set socks, fullsocks;
	FD_ZERO(&fullsocks);

	for(i=0;i<NUM_PORTS;i++) {
		if ((sps[i] = getpt()) == -1)
			fatalErrorExit(errno, "cannot open stream");
		if (grantpt(sps[i]) == -1)
			fatalErrorExit(errno, "cannot grant stream");
		if (unlockpt(sps[i]) == -1)
			fatalErrorExit(errno, "cannot unlock stream");
		printf("Parent role: child endpoint at %s [%s]\n", ptsname(sps[i]), portnames[i]);
		if (sps[i] > maxfd)
			maxfd = sps[i];

		FD_SET(sps[i],&fullsocks);
	}


	rp = open(REAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
	if (rp == -1) {
		fatalErrorExit(errno, "cannot open real port");
	}

	tcgetattr(rp, &options);

    	cfsetispeed(&options, B115200);
    	cfsetospeed(&options, B115200);

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
			rc = read(rp, buffer, buffSize);
			if (rc > 0) {
				printf("Received from arduino: %d bytes [%d]\n", rc, buffer[0]);
				if (buffer[0] == 255 && rc == 1) {
					ready = 1;
				} else if (rc > 1) { 
					if (buffer[rc-1] == 255) {
						ready = 1;
					} else {
						ready = 0;
					}
					for(i=0;i<rc;i++) {
						if (buffer[i] == 0x55) {
							flow_control = 0;
							printf("Found X10 termination, flow off\n");
						}
					}

					rc = write(sps[active_port], buffer, rc-1);
				} else {
					ready = 0;
				}
			}
		}
		for(i=0;i<NUM_PORTS;i++) {
			if (FD_ISSET(sps[i], &socks)) {
				rc = read(sps[i], buffer, buffSize);
				if (rc > 0) {
					if (i == active_port) {
						printf("Flow control ON\n");
						flow_control = 1;
					}
			
					if (flow_control && i != active_port)
						continue;

					if (!ready) 
						continue;
					printf("port %d [%s], received %d, ready: %d \n", i, portnames[i], rc, ready);
					wc = write(rp, buffer, rc);
				}
			}
		}
	}

	close(rp);
	return 0;
}
