#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include "lib.h"

int
init_addr(addr_t *addr,const char *astring)
{
	addr->a_addr = strdup(astring);
	/* Make correct sockaddr */
	if (strncmp(astring,"unix:",5) == 0) {
		/* local namespace */
		struct sockaddr_un *name = &addr->un_name;
		name->sun_family = AF_LOCAL;
		if (strlen(astring + 5) >= sizeof(name->sun_path)) {
			errno = EINVAL;
			return 1;
		}
		strncpy (name->sun_path, astring+5, sizeof(name->sun_path));
		name->sun_path[sizeof(name->sun_path)-1] = '\0';
		addr->namelen = SUN_LEN(name);
	} else {
		/* inet namespace */
		struct sockaddr_in *name = &addr->in_name;
		char *colon = strchr(addr->a_addr, ':');
		if (colon || isdigit(*addr->a_addr)) {
			if (colon && colon != addr->a_addr) {
				struct hostent *hostinfo;
				*colon = 0;
				hostinfo = gethostbyname (addr->a_addr);
				if (hostinfo == NULL) {
					free (addr->a_addr);
					return 1;
				}
				*colon = ':';
				name->sin_addr = *(struct in_addr*)hostinfo->h_addr;
			} else {
				name->sin_addr.s_addr = htonl(INADDR_ANY);
			}
			name->sin_family = AF_INET;
			name->sin_port = htons(atoi(colon ? colon+1 : addr->a_addr));
			addr->namelen = sizeof(struct sockaddr_in);
		} else {
			free (addr->a_addr);
			return 1;
		}
	}
	return 0;
}

int
set_nonblock(int sock,int value)
{
	return ioctl(sock, FIONBIO, &value);
}

int
nonblock_connect(int sock,struct sockaddr *sa,socklen_t sl,int timeout)
{
	int res;
	set_nonblock (sock, 1);
	res = connect (sock, sa, sl);
	if (res == -1 && errno == EINPROGRESS) {
		struct pollfd pfd;
		pfd.fd = sock;
		pfd.events = POLLOUT;
		if (poll (&pfd, 1, timeout) == 1) {
			/* Check for socket error */
			int err;
			socklen_t l = sizeof(err);
			if (getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &l) != 0) return -1;
			if (!err) return 0; // Success
		}
	}
	return -1;
}

/* Create socket and bind it to specifed address */
int
make_and_bind_socket(const char *addr)
{
	int sock, yes = 1;
	addr_t sa;

	if (init_addr(&sa, addr) != 0)
		return -1;

	free(sa.a_addr);

	sock = socket(sa.name.sa_family, SOCK_STREAM, 0);
	if (sock == -1)
		return -1;

	if (set_nonblock(sock, 1) == -1)
		goto error;

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	if (bind(sock, &sa.name, sa.namelen) == -1)
		goto error;

	return sock;
error:
	yes = errno;
	close(sock);
	errno = yes;
	return -1;
}
