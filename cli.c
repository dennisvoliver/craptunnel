#include <time.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <linux/if_tun.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUFFSIZ 65536
#define MAXLINE 100
int pollem(int connfd, int tunfd);
int tun_alloc(char *dev);
int funnel(int tofd, int fromfd);
int myerror(char *errmsg);

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "arg ip address\n");
		return -1;
	}
	int sockfd;
	struct sockaddr_in servaddr;
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 1) {
		fprintf(stderr, "socket() error\n");
		return -1;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1195);
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
		fprintf(stderr, "inet_pton error\n");
		return -1;
	}
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		fprintf(stderr, "%s", strerror(errno));
		fprintf(stderr, "connect error\n");
		return -1;
	}

	char devnam[] = "tun45";
	int tunfd;
	if ((tunfd=tun_alloc(devnam)) < 0) {
		myerror("can't alloc tun45");
		return -1;
	}
	if (pollem(sockfd, tunfd) < 0) {
		myerror("pollem err");
		return -1;
	}
	return 0;
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
		return 0; /* never gets here */
	}
	while (1) {
		funnel(tunfd, connfd);
	}

	return 0; /* parent returns */
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
