#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <linux/if_tun.h>
#include <linux/if.h>
#define BUFFSIZ 65536
#define MAXLINE 100
int pollem(int connfd, int tunfd);
int tun_alloc(char *dev);
int funnel(int tofd, int fromfd);
int myerror(char *errmsg);
int main(int argc, char **argv)
{
	int listenfd, connfd;
	struct sockaddr_in servaddr;
	time_t	ticks;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		fprintf(stderr, "bind:%s", strerror(errno));
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(1195);	/* web */

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if (errno != 0) {
		fprintf(stderr, "listen: %s\n", strerror(errno));
		return -1;
	}

	if (listen(listenfd, 10) < 0) {
		fprintf(stderr, "listen: %s\n", strerror(errno));
		return -1;
	}

	char devnam[] = "tun45";
	int tunfd;
	if ((tunfd=tun_alloc(devnam)) < 0) {
		myerror("can't alloc tun45");
		return -1;
	}
	for ( ; ; ) {
		connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
		if (connfd < 0) {
			fprintf(stderr, "accept: %s\n", strerror(errno));
			return -1;
		}

		if (pollem(connfd, tunfd) < 0)
			;/* child stopped polling */
		close(connfd);
	}
}

int pollem(int connfd, int tunfd)
{
	pid_t pid;
	if ((pid=fork()) < 0) {
		myerror("can't fork");
		return -1;
	}
	if (pid == 0) { /* child */
		while (1) {
			funnel(connfd, tunfd);
		}
		/* should never get here */
		return -1;
	} else {
		while (1) {
			funnel(tunfd, connfd);
		}
	}
	return -1; /* never get here */
}
int funnel(int tofd, int fromfd)
{
	char buff[BUFFSIZ];
	int n = 0;
	while ((n=read(fromfd, buff, BUFFSIZ)) > 0) {
		if (write(tofd, buff, n) < 0) {
			myerror("funnel can't write");
			return -1;
		}
	}
	return n;
}
int myerror(char *errmsg)
{
	char buff[MAXLINE];
	sprintf(buff, "error: %s, strerror: %s\n", errmsg, strerror(errno));
	perror(buff);
	return 0;
}

int tun_alloc(char *dev)
{
	struct ifreq ifr;
	int fd;
	if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
		fprintf(stderr, "can't open");
		return -1;
	}
	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TUN;
	if (*dev)
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	int err = 0;
	if (ioctl(fd, TUNSETIFF, (void *) &ifr) != 0) {
	       if (errno == EBADFD) {
	  /* Try old ioctl */
				fprintf(stderr, "bad file desc");
		}
		fprintf(stderr, "err: %s", strerror(errno));
		fprintf(stderr, "ioctl err");
		return -1;
	}
	return fd;
}
