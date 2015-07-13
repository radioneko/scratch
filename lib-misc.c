#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

const char U_XDIGITS[] = "0123456789ABCDEF";
const char l_xdigits[] = "0123456789abcdef";

void perror_fatal(const char *what, ...)
{
	va_list ap;
	int err = errno;
	va_start(ap, what);
	vfprintf(stderr, what, ap);
	va_end(ap);
	fprintf(stderr, ": %s\n", strerror(err));
	exit(EXIT_FAILURE);
}

int ignore_signal(int sig)
{
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig,  &sa, NULL) == -1)
		return -1;
	return 0;
}

void
hexdump(FILE *out, unsigned rel, const void *data, unsigned size)
{
	const unsigned frame = 16;
	char hex[(3 * frame) + 2 * (frame - 1) / 4 + 1];
	char cc[frame + 1];
	unsigned k = (rel % frame) * 3 + (rel % frame) / 4 * 2, j = rel % frame;
	const uint8_t *src = data, *end = src + size;
	memset(hex, 0x20, sizeof(hex));
	memset(cc, 0x20, sizeof(cc));

	while (src < end) {
		unsigned i = src - (uint8_t*)data;
		while (j < frame && src < end) {
			hex[k] = 0x20;
			hex[k + 1] = l_xdigits[*src >> 4];
			hex[k + 2] = l_xdigits[*src & 0xf];
			k += 3;
			if ((j + 1) % 4 == 0 && j + 1 != frame) {
				hex[k] = 0x20;
				hex[k + 1] = '|';
				k += 2;
			}
			cc[j] = *src >= 0x20 && *src <= 127 ? *src : '.';
			src++;
			j++;
		}
		memset(hex + k, 0x20, sizeof(hex) - k);
		hex[sizeof(hex) - 1] = 0;
		cc[j] = 0;
		j = k = 0;
		fprintf(out, "%08x %s  %s\n", rel + i, hex, cc);
	}
}

unsigned
xdigit2i(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return c - 'a' + 10;
}

unsigned
clock_diff(const struct timespec *start, const struct timespec *stop)
{
	unsigned ms;
	if (stop->tv_nsec < start->tv_nsec) {
		ms = (stop->tv_sec - start->tv_sec - 1) * 1000000 + (1000000000 + stop->tv_nsec - start->tv_nsec) / 1000;
	} else {
		ms = (stop->tv_sec - start->tv_sec) * 1000000 + (stop->tv_nsec - start->tv_nsec) / 1000;
	}
	return ms;
}
