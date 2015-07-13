#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <openssl/sha.h>
#if 0
static void*
thr_wait(void *arg)
{
	int epfd;
	struct epoll_event e[1];
	int fd = (intptr_t)arg;

	epfd = epoll_create1(EPOLL_CLOEXEC);
	e->events = EPOLLIN | EPOLLET;
	e->data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, e);

	for (int i = 0; i < 1; i++) {
		printf("in epoll_wait\n");
		fd = epoll_wait(epfd, e, 1, 2000);
		if (fd) {
			printf("GOT EVENT!\n");
//			read(e->data.fd, &fd, sizeof(fd));
//			read(e->data.fd, &fd, sizeof(fd));
		} else {
			printf("timed out\n");
		}
	}
	close(epfd);
	return NULL;
}

int main()
{
	unsigned i, thr = 4;
	pthread_t t[thr];
	int pfd[2];

	if (pipe2(pfd, O_CLOEXEC | O_NONBLOCK) != 0)
		return 1;

	for (i = 0; i < thr; i++)
		pthread_create(t + i, NULL, thr_wait, (void*)(intptr_t)pfd[0]);

	printf("sleeping\n");
	sleep(1);
	write(pfd[1], &thr, sizeof(thr));

	for (i = 0; i < thr; i++) {
		void *res;
		pthread_join(t[i], &res);
	}
	return 0;
}
#endif

static void
ep_wait(int epfd)
{
	struct epoll_event e[5];
	int n = epoll_wait(epfd, e, sizeof(e) / sizeof(*e), 0);
	if (n == 0) {
		printf("timeout\n");
	} else if (n > 0) {
		printf("%d descriptors ready\n", n);
		for (int i = 0; i < n; i++) {
			char buf[128], *m = buf;
#define CAT(str) do { memcpy(m, str, sizeof(str) - 1); m += sizeof(str) - 1; } while (0)
#define CHECK(bit) if (e[i].events & bit) { if (m != buf) CAT(" | "); CAT(#bit); }
			CHECK(EPOLLIN);
			CHECK(EPOLLOUT);
			CHECK(EPOLLRDHUP);
			CHECK(EPOLLERR);
			CHECK(EPOLLHUP);
			*m = 0;
			printf("  * %d => %s (%0x)\n", e[i].data.fd, buf, e[i].events);
		}
	} else {
		printf("ERROR\n");
	}
}

static void
ep_add(int epfd, int fd, int events)
{
	struct epoll_event e;
	e.events = events;
	e.data.fd = fd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &e) != 0)
		printf("epoll_ctl(ADD, fd) => %s\n", strerror(errno));
}

static void
ep_mod(int epfd, int fd, int events)
{
	struct epoll_event e;
	e.events = events;
	e.data.fd = fd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &e) != 0)
		printf("epoll_ctl(MOD, fd) => %s\n", strerror(errno));
}

#define C(fn, fd, ...)		if (fn(fd, ##__VA_ARGS__) == -1) printf("! %s(%d) => %s\n", #fn, fd, strerror(errno));
#define R		C(read, pfd[0], &i, sizeof(i))
#define W		C(write, pfd[1], &i, sizeof(i))
#define SO		C(send, pfd[1], &i, sizeof(i), MSG_OOB)
int main()
{
	int pfd[2];
	int epfd = epoll_create1(EPOLL_CLOEXEC);
	int i;

	//(void)
	if (-1 ==
#if 0
		pipe2(pfd, O_CLOEXEC | O_NONBLOCK)
#else
		socketpair(PF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0, pfd)
#endif
	   )
	{
		perror("make_fds");
		return 1;
	}

	ep_add(epfd, pfd[0], EPOLLIN /*| EPOLLRDHUP*/ | EPOLLET);
//	ep_add(epfd, pfd[1], EPOLLOUT | EPOLLET);
	ep_wait(epfd);
	W;
	SO;
	ep_wait(epfd);
	R;
	R;
	ep_wait(epfd);
	close(pfd[1]);
	ep_wait(epfd);
	printf("%zu\n", sizeof(SHA_CTX));
	return 0;
}
