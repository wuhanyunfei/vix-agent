#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>

#include "config.h"

#define TCP_PROTO 0
#define LISTEN_NUM 10

struct sockaddr_in hostaddr;
extern int errno;
extern __const char * __const sys_errlist[];

const char respfile[]="[0]F:";

void daemonize(int servfd);
void reap_status();
void do_esxi_session(int sockfd);

void daemonize(int servfd){
	int childpid, fd, fdtablesize, pid;
	int logfd;

	logfd = openlog();

	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	if((childpid=fork())<0){
		dumplog(logfd, "failed to fork fist child");
		closelog(logfd);
		exit(1);
	}else if(childpid > 0){
		closelog(logfd);
		exit(0);
	}
	setsid();
	signal(SIGHUP, SIG_IGN);

	if((pid=fork())!=0){
		closelog(logfd);
		exit(0);
	}

	for(fd=0,fdtablesize=getdtablesize();fd<fdtablesize;fd++){
		if(fd!=servfd){
			close(fd);
		}
	}

	chdir("/");
	umask(0);
	
	closelog(logfd);
}


void reap_status(){
	int pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG))>0);
}

void do_esxi_session(int sockfd){
	int localfd, i, n, len;
	char logbuf[BUFMAXLEN] = "\0";
	char msgbuf[MAXPACKLEN] = "\0";
	char respbuf[MAXPACKLEN] = "\0";
	char filebuf[MAXPACKLEN + 1] = "\0";
	char *ptr = NULL;
	int rfd;
	char ok[]="[0]";
	char pkgtail[] = "[EOF]";
	char testbuf[MAXPACKLEN + 1] = "\0";

	localfd = openlog();
	bzero(logbuf, BUFMAXLEN);
	sprintf(logbuf, "Do esxi session on socket fd: %d", sockfd);
	dumplog(localfd, logbuf);

	for(;;){
		bzero(msgbuf, sizeof(msgbuf));
		n = read(sockfd, msgbuf, sizeof(msgbuf));
		if(n <= 0)
			break;
		for(i=n-1;i>0;i--){
			if(msgbuf[i] == 10 || msgbuf[i] == 13){
				msgbuf[i] = 0;
			}else{
				break;
			}
		}
		dumplog(localfd, msgbuf);
		if(strncmp(msgbuf, "quit", 4) == 0){
			dumplog(localfd, "Bye!!!");
			break;
		}else{
			bzero(logbuf, sizeof(logbuf));
			sprintf(logbuf, "Incoming %s", msgbuf);
			dumplog(localfd, logbuf);
			handle_esxi_session(msgbuf, respbuf);
			dumplog(localfd, "TEST AFTER handle esxi");
			dumplog(localfd, respbuf);
		}
		if(strncmp(respbuf, respfile, strlen(respfile)) == 0){			
			ptr = respbuf + strlen(respfile);
			dumplog(localfd, ptr);
			// read content from large file and send them out
			rfd = open(ptr, O_RDONLY);
			if(rfd != -1){
				write(sockfd, ok, strlen(ok));
				bzero(testbuf, sizeof(testbuf));
				sprintf(testbuf, "sent:|%s|", ok);
				dumplog(localfd, testbuf);
				len = read(rfd, filebuf, MAXPACKLEN);
				while(len > 0){
					write(sockfd, filebuf, MAXPACKLEN);
					bzero(testbuf, sizeof(testbuf));
					sprintf(testbuf, "sent:|%s|", filebuf);
					dumplog(localfd, testbuf);
					bzero(filebuf, MAXPACKLEN + 1);
					len = read(rfd, filebuf, MAXPACKLEN);
				}
				close(rfd);
				// kill the temp file
				remove(ptr);
			}
		}else{
			write(sockfd, respbuf, strlen(respbuf));
			bzero(testbuf, sizeof(testbuf));
			sprintf(testbuf, "sent:|%s|", respbuf);
			dumplog(localfd, testbuf);
		}
		write(sockfd, pkgtail, strlen(pkgtail));
	}
	

	closelog(localfd);
}


int main(int argc, char * argv[]){
	//define
	int opt, len;
	int clilen;
	int childpid;
	int mainfd;
	int sockfd, newsockfd;
	int listen_port;
	char * listen_ip;
	struct sockaddr_in servaddr, cliaddr;
	char msgbuf[BUFMAXLEN] = "\0";
	int ret ; 


	if(argc < 3){
		fprintf(stderr, "Usage: vmmengine <127.0.0.1> <4564>\n");
		return 1;
	}
	listen_ip = argv[1];
	listen_port = atoi(argv[2]);
	// init
	mainfd = openlog();
	dumplog(mainfd, "vmmengine started");
	bzero(msgbuf, BUFMAXLEN);
	sprintf(msgbuf, "Listening at %s:%d", listen_ip, listen_port);
	dumplog(mainfd, msgbuf);
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(listen_ip);
	servaddr.sin_port = htons(listen_port);

	// main body
	// network
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
		dumplog(mainfd, "Failed to create server socket!");
		closelog(mainfd);
		exit(1);
	}

	opt = 1;
	len = sizeof(opt);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, &len);
	if(bind(sockfd, (struct sockaddr_in *)&servaddr, sizeof(servaddr))<0){
		dumplog(mainfd, "Failed to bind!");
		closelog(mainfd);
		exit(1);
	}
	listen(sockfd, LISTEN_NUM);

	daemonize(sockfd);

	signal(SIGCHLD, reap_status);
	
	for(;;){
		clilen = sizeof(cliaddr);
		newsockfd = accept(sockfd, (struct sockaddr_in *)&cliaddr, &clilen);
		if(newsockfd < 0 && errno == EINTR)
			continue;
		else if(newsockfd < 0)
			dumplog(mainfd, "Failed to accept connection! Kill me!");
			
		if((childpid = fork()) == 0){
			close(sockfd);
			do_esxi_session(newsockfd);
			exit(0);
		}
		close(newsockfd);
	}


	dumplog(mainfd, "vmmengine stopped nicely");
	closelog(mainfd);
	return 0;
}

