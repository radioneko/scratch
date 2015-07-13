#include <openssl/sha.h>
#include <unistd.h>
#include <stdio.h>
#include "lib.h"

static void
digest(SHA_CTX *c, const char *msg)
{
	char ds[41];
	unsigned char d[20];
	unsigned i;

	printf("%s: %s\n", msg, ds);
}

#define adds(c, str) SHA1_Update(c, str, sizeof(str))

int main()
{
	int pfd[2];
	SHA_CTX c[2];

	pipe(pfd);

	SHA1_Init(c);
	adds(c, "hello");
	adds(c, " world");
	digest(c, "original");
	digest(c + 1, "copy");
	return 0;
}
