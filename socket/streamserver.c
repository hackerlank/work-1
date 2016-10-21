//流协议 与 粘包

// 包没有边界 ，手动加边界 ftp:// \n\r
// 自定义报文：


#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define ERR_EXIT(m) \
		do \
		{ \
			perror(m); \
			exit(EXIT_FAILURE); \
		} while(0)

struct packet {
	int len;
	char buf[1024];
};
// 1 一次全部读走  //2次读完数据  //出错分析  //对方关闭
// 思想：tcpip 是流协议， 不能保证1次读操作，能把全部报文读走，所以要循环读指定长度的数据
//按照 count 大小读数据
//若读取的数据长度 ssize_t < count 说明读到了一个结束符，对方关闭

//@ssize_t ：返回读的长度， 若ssize_t < count 读取失败
//@buf:	接收数据内存的首地址
//@count : 接收数据的长度
ssize_t readn(int fd, void *buf, size_t count) {
	//剩余要读的数据长度
	size_t nleft = count;
	ssize_t nread;

	char *bufp = (char *)buf;

	//nfleft = 0时退出循环，读满
	while(nleft > 0) {
		if ((nread = read(fd, bufp, nleft)) < 0)
		{	//由于信号中断 没有读取到数据
			if (errno == EINTR)
				continue;
			return -1; //在这里退出这个函数，没有读满count个函数，读取失败
		}		
		else if (nread == 0) //对方关闭
			return count - nleft; //返回已读的个数

		bufp += nread;
		//读取成功，计算剩余要读的长度
		nleft -= nread;
	}

	return count;
}

ssize_t writen(int fd, const void *buf, size_t count) {
	size_t nleft = count;
	ssize_t nwritten;
	char *bufp = (char *)buf;

	while(nleft > 0) {
		if ((nwritten = writen(fd, bufp, nleft)) < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		}
		else if (nwritten == 0) //对方关闭
			continue;

		bufp += nwritten;
		nleft -= nwritten;
	}
	//返回读到的数据个数
	return count;
}

ssize_t recv_peek(int sockfd, void *buf, size_t len) {
	while(1) {
		//MSG_PEEK 设置后只读，不清除
		int ret = recv(sockfd, buf, len, MSG_PEEK);
		if (ret == -1 && errno == EINTR)
			continue;
		return ret;
	}
}

//maxline 一行最大数
//先提前peek一下缓冲区， 如果有数据从缓冲区读数据
//1.缓冲区数据带\n
//2.缓冲区数据不带\n
ssize_t readline(int sockfd, void *buf, size_t maxline) {
	int ret;
	int nread;
	char *bufp = buf;
	int nleft = maxline;

	while(1) {
		//看一下缓冲区有没有数据， 并不移除内核缓冲区的数据
		ret = recv_peek(sockfd, bufp, nleft);
		if (ret < 0)
			return ret;
		else if (ret == 0) //对方已关闭
			return ret;

		nread = ret;
		int i;
		for ( i = 0; i < nread; i++)
		{
			if (bufp[i] == '\n') //如果缓冲区有\n
			{
				ret = readn(sockfd, bufp, i+1);//读走数据
				if (ret != i+1)
					exit(EXIT_FAILURE);

				return ret; //有\n就返回，并返回读走的数据
			}
		}

		if (nread > nleft) //如果读到的个数 大于 一行最大数 异常处理
			exit(EXIT_FAILURE);

		nleft -= nread; //若缓冲区没有\n, 把剩余的数据读走
		ret = readn(sockfd, bufp, nread);
		if (ret != nread)
			exit(EXIT_FAILURE);

		bufp += nread; //bufp 指针后移，再接着 看缓冲区的数据recv_peek, 直到遇到\n
	}

	return -1;
}

void do_service(int conn) {
	char recvbuf[1024];

	while(1) {
		memset(recvbuf, 0, sizeof(recvbuf));
		int ret = readline(conn, recvbuf, 1024);
		if (ret == -1)
			ERR_EXIT("readline");
		if (ret == 0)
		{
			printf("client close\n");
			break;
		}

		fputs(recvbuf, stdout);
		writen(conn, recvbuf, strlen(recvbuf));
	}
}

int main(void) {
	int listenfd;
	if ((listenfd = socket(PF_INET, SOCK_STREAM, IPPORT_TCP)) < 0)
		ERR_EXIT("socket");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8001); //host to network short
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//host to network long

	int on = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("setsockopt");

	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("bind");

	if (listen(listenfd, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	struct sockaddr_in peeraddr;
	socklen_t peerlen = sizeof(peeraddr);
	int conn;

	pid_t pid;

	while(1) {
		if ((conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen)) < 0)
			ERR_EXIT("accept");

		printf("client ip = %s, port = %d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));

		pid = fork();
		if (pid = -1)
			ERR_EXIT("fork");
		if (pid == 0)
		{
			close(listenfd);
			do_service(conn);
			exit(EXIT_SUCCESS);
		}
	}

	
}