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

/* Try to read from non-blocking descriptor. Returns -1 and EAGAIN if timout reached
 * and not a single byte of data was received. */
int
read_ms(int fd, void *buf, unsigned len, unsigned ms)
{
	int l;
	do {
		l = read(fd, buf, len);
		if (l == -1) {
			int e = errno;
			if (e == EINTR)
				continue;
			if (e == EAGAIN) {
				struct pollfd pfd;
				pfd.fd = fd;
				pfd.events = POLLIN;
				if (poll(&pfd, 1, ms) == 1)
					continue;
				errno = EAGAIN;
			}
			return -1;
		}
	} while (l < 0);
	return l;
}
/* Try to write to non-blocking descriptor. Returns -1 and EAGAIN if timeout reached.
 * Function returns immediatly as soon as any data had been send. */
int
write_ms(int fd, const void *buf, unsigned len, unsigned ms)
{
	int l;
	do {
		l = write(fd, buf, len);
		if (l == -1) {
			int e = errno;
			if (e == EINTR)
				continue;
			if (e == EAGAIN) {
				struct pollfd pfd;
				pfd.fd = fd;
				pfd.events = POLLOUT | POLLERR;
				if (poll(&pfd, 1, ms) == 1 && (pfd.revents & POLLOUT))
					continue;
				errno = EAGAIN;
			}
			return -1;
		}
	} while (l < 0);
	return l;
}

/* Write whole buffer to specified descriptor.
 * On error returns number of bytes that were successfully sent. That means error condition for
 * this function is result != len.
 * ms - timeout in millisecond between successful writes. So having write timeout of
 * 2 seconds (2000 ms) may theoretically result in 2 * len seconds to send whole buffer. */
int
write_full_ms(int fd, const void *buf, unsigned len, unsigned ms)
{
	unsigned pos = 0;
	do {
		int l = write_ms(fd, buf + pos, len - pos, ms);
		if (l < 0)
			break;
		pos += l;
	} while (pos < len);
	return pos;
}

/* write whole iovec or return an error */
int
writev_full(int fd, struct iovec *iov, unsigned iov_cnt)
{
	int nwritten = 0;
	while (iov_cnt) {
		int bytes = writev(fd, iov, iov_cnt);
		if (bytes > 0) {
			nwritten += bytes;
			while (bytes && bytes >= iov->iov_len) {
				bytes -= iov->iov_len;
				iov++;
				iov_cnt--;
			}
			if (bytes) {
				iov->iov_base = (char*)iov->iov_base + bytes;
				iov->iov_len -= bytes;
			}
		} else {
			switch (errno) {
			case EINTR:
			case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
			case EWOULDBLOCK:
#endif
				continue;
			default:
				return -1;
			}
		}
	}
	return nwritten;
}

/* Same as writev_full but with timeouts between successive writevs */
int
writev_full_ms(int fd, struct iovec *iov, unsigned iov_cnt, unsigned ms)
{
	int nwritten = 0;
	while (iov_cnt) {
		int bytes = writev(fd, iov, iov_cnt);
		if (bytes > 0) {
			nwritten += bytes;
			while (bytes && bytes >= iov->iov_len) {
				bytes -= iov->iov_len;
				iov++;
				iov_cnt--;
			}
			if (bytes) {
				iov->iov_base = (char*)iov->iov_base + bytes;
				iov->iov_len -= bytes;
			}
		} else {
			switch (errno) {
			case EINTR:
				continue;
			case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
			case EWOULDBLOCK:
#endif
				{
					struct pollfd pfd;
					pfd.fd = fd;
					pfd.events = POLLOUT | POLLERR;
					if (poll(&pfd, 1, ms) == 1 && (pfd.revents & POLLOUT))
						continue;
				}
				/* fallthrough */
			default:
				return -1;
			}
		}
	}
	return nwritten;
}
