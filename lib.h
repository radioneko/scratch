#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	union {
		struct sockaddr		name;
		struct sockaddr_in	in_name;
		struct sockaddr_un	un_name;
	};
	socklen_t	namelen;
	char		*a_addr;
} addr_t;

#ifdef __cplusplus
extern "C" {
#endif /* C++ */
extern const char U_XDIGITS[];
extern const char l_xdigits[];

void perror_fatal(const char *what, ...) __attribute__((format(printf, 1, 2)));
int set_nonblock(int sock,int value);
int nonblock_connect(int sock,struct sockaddr *sa,socklen_t sl,int timeout);
int init_addr(addr_t *addr,const char *str);
int make_and_bind_socket(const char *addr);

void ignore_sigpipe();

unsigned clock_diff(const struct timespec *start, const struct timespec *stop);
void hexdump(FILE *out, unsigned rel, const void *data, unsigned size);

/* misc functions */
unsigned xdigit2i(char c);

#ifdef __cplusplus
}
#endif /* C++ */
