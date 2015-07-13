#include "lib.h"
#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "config.h"
#include <ctype.h>
#include <inttypes.h>

static const char rqh[] =
	"GET /8/7E/92/7E92AFE5A3E0A1D2F308456185EA770BD845AFA0 HTTP/1.0\r\n"
	"Host: " HOST "\r\n"
	"Range: bytes=1-10,200-20000\r\n"
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

/* returns HTTP code or -1 if error occured, blocking function
 * returns:
 *    > 0 - http response bytes in buffer
 *     -1 - error writing request
 *     -2 - timeout waiting for response
 *     -3 - incomplete/bad headers
 */
int
http_handshake(int sock, const void *hdr, unsigned hdr_len, pstr_t *data)
{
	int bytes, code, i;
	char *p;
	char buf[4096];

	if (write_full_ms(sock, hdr, hdr_len, 1000) != hdr_len)
		return -1;

	bytes = read_ms(sock, buf, sizeof(buf), 5000);
	if (bytes < 12							/* minimal headers length */
		|| memcmp(buf, "HTTP/1.", 7) != 0
		|| (buf[7] != '0' && buf[7] != '1')
		|| buf[8] != 0x20
		|| !isdigit(buf[9]))
		return -2;

	/* read response code */
	code = 0;
	for (i = 9; i < bytes && isdigit(buf[i]); i++)
		code = code * 10 + (buf[i] - '0');

	/* find end-of-headers */
	p = memmem(buf, sizeof(buf), "\r\n\r\n", 4);
	if (p == NULL)
		return -3;

	data->len = buf + bytes - (p + 4);
	memcpy(data->data, buf, data->len);

	return code;
}

int main()
{
	int status;
	pstr_t body;
	uint64_t nbytes = 0;
	char buf[4096];
	int s = connect_to(HOST, 80);

	body.data = buf; body.len = sizeof(buf);
	status = http_handshake(s, rqh, sizeof(rqh), &body);
	printf("GOT %d + %u\n", status, body.len);

	for (;;) {
		int bytes = read_ms(s, buf, sizeof(buf), 5000);
		if (bytes <= 0)
			break;
		nbytes += bytes;
	}

	printf("%" PRIu64 " bytes transferred\n", nbytes);
	return 0;
}
