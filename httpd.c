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
			(void)write(num,logbuffer,strlen(logbuffer));
			(void)sprintf(lobbuffer,"SORRY: %s:%s",s1,s2);
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

/* child web server process, so we can exit on errors */
void web(int fd, int hit) {
	int j;
	int buflen;
	char *fstr;
	long ret;
	static char buffer[BUFSIZE];
	
	ret = read(fd, buffer, BUFSIZE);

	if (ret == 0 || ret == -1) 	/* read failure, stop now */
		log(SORRY, "failed to read browser request", fd);

	if (ret > 0 && ret < BUFSIZE)
		buffer[ret] = '\0'; /* terminate the buffer */
	else
		buffer[0] = '\0';

	for (i = 0; i < ret; i++) 
		if(buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i] = '*'; // WTF is this mean
	
	log(LOG, "request", buffer, hit);

	/* now, let us theck the 'GET' or 'get' command */
	if(strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4)) // no 'get ' or 'GET ' in buffer if true
		log(SORRY, "Only simple GET operation supported", buffer, fd);


	for (i = 4; i < BUFSIZE, i++) { /* null terminate after the second space to ignore extra stuff */
		if (buffer[i] == ' ') {
			buffer[i] = '\0'; // terminate the request after URL
			break;
		}
	}

	/* check for illegal parent directory use .. */
	for (j = 0; j < i-1; j++) 
		if(buffer[j] == '.' && buffer[j+1] == '.')
			log(SORRY, "Parent directory (..) path names not supported", buffer, fd);


	if(!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6) )
		(void)strcpy(buffer,"GET /index.html");

	buflen = strlen(buffer);
	fstr = (char *)0;

	for (i = 0; extension[i].ext != 0; i++) {

	



