#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 8096
#define ERROR 42
#define SORRY 43
#define LOG 44

struct {
	char *ext;	// file extension, ".html", ".gif"......
	char *filetype;
} extension[] = {
		{ "gif", "image/gif" },
		{ "jpg", "image/jpg" },
		{ "jpeg", "image/jpeg" },
		{ "png", "image/png" },
		{ "zip", "image/zip" },
		{ "gz", "image/gz" },
		{ "tar", "image/tar" },
		{ "htm", "text/htm" },
		{ "html", "text/html" },
		{ 0, 0} };

void log(int type, char *s1, char *s2, int num) {
	int fd;
	char logbuffer[BUFSIZE*2];

	switch(type) {
		case ERROR:
			(void)sprintf(logbuffer, "ERROR: %s:%s Error=%d exiting pid=%d", s1, s2, errno, getpid());
			break;
		case SORRY:
			(void)sprintf(logbuffer, "<HTML><BODY><H1>nweb Web Server Sorry: %s %s</H1></BODY></HTML>\r\n", s1,s2);
			break;
		case LOG:
			(void)sprintf(logbuffer, "INFO, %s:%s:%d", s1,s2,num);
			break;
	}

	if((fd = open("nweb.log", O_CREAT|O_WRONLY|O_APPEND, 0644)) >= 0) {
		(void)write(fd, logbuffer, strlen(logbuffer));
		(void)write(fd, "\n", 1);
		(void)close(fd);
	}
	if (type == ERROR || type == SORRY) 
		exit(3);
} 



