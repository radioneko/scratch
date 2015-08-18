#include "lib.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	addr_t addr;
	double timeout = 5.0;
	int sock;

	if (argc < 2) {
		printf("Usage: connect host:port [timeout_in_seconds]\n");
		return 1;
	}

	if (argc > 2) {
		timeout = atof(argv[2]);
		if (timeout <= 0) {
			timeout = 5.0;
			printf("Warning: using timeout %gs\n", timeout);
		}
	}

	printf("Resolving...\n");
	if (init_addr(&addr, argv[1]) != 0)
		perror_fatal("init_addr(%s)", argv[1]);

	sock = socket(addr.name.sa_family, SOCK_STREAM, 0);
	if (sock == -1)
		perror_fatal("socket");

	printf("Connecting...\n");
	if (nonblock_connect(sock, &addr.name, addr.namelen, timeout * 1000) == -1)
		perror_fatal("connect(%s)", addr.a_addr);
	else
		printf("Success!\n");
	return 0;
}
