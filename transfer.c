#include "lib.h"
#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "config.h"

static const char rqh[] =
	"GET /8/7E/92/7E92AFE5A3E0A1D2F308456185EA770BD845AFA0 HTTP/1.0\r\n"
	"Host: " HOST "\r\n"
	"\r\n\r\n";

int connect_to(const char *host, int port)
{
	addr_t a;
	char buf[64];
	int s;

	sprintf(buf, "%s:%d", host, port);
	if (init_addr(&a, buf) != 0)
		perror_fatal("connect %s:%u", host, port);

	free(a.a_addr);

	s = socket(a.name.sa_family, SOCK_STREAM, 0);
	if (s == -1)
		perror_fatal("socket");

	if (nonblock_connect(s, &a.name, a.namelen, 1000) != 0)
		perror_fatal("connect(%s:%d)", host, port);

	return s;
}

int main()
{
	int s = connect_to(HOST, 80);

	return 0;
}
