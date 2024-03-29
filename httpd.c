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

void Log(int type, char *s1, char *s2, int num) { 
	int fd;
	char logbuffer[BUFSIZE*2];

	switch(type) {
		case ERROR:
			(void)sprintf(logbuffer, "ERROR: %s:%s Error=%d exiting pid=%d", s1, s2, errno, getpid());
			break;
		case SORRY:
			(void)sprintf(logbuffer, "<HTML><BODY><H1>nweb Web Server Sorry: %s %s</H1></BODY></HTML>\r\n", s1,s2);
			(void)write(num,logbuffer,strlen(logbuffer));
			(void)sprintf(logbuffer,"SORRY: %s:%s",s1,s2);
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
	int i, j, file_fd;
	int buflen;
	int len;
	char *fstr; // fstr is used for "Content-Type:" 
	long ret;
	static char buffer[BUFSIZE];
	
	ret = read(fd, buffer, BUFSIZE);

	if (ret == 0 || ret == -1) 	/* read failure, stop now */
		Log(SORRY, "failed to read browser request","", fd);

	if (ret > 0 && ret < BUFSIZE)
		buffer[ret] = '\0'; /* terminate the buffer */
	else
		buffer[0] = '\0';

	for (i = 0; i < ret; i++) 
		if(buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i] = '*'; // WTF is this mean
	
	Log(LOG, "request", buffer, hit);

	/* now, let us theck the 'GET' or 'get' command */
	if(strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4)) // no 'get ' or 'GET ' in buffer if true
		Log(SORRY, "Only simple GET operation supported", buffer, fd);


	for (i = 4; i < BUFSIZE; i++) { /* null terminate after the second space to ignore extra stuff */
		if (buffer[i] == ' ') {
			buffer[i] = '\0'; // terminate the request after URL
			break;
		}
	}

	/* check for illegal parent directory use .. */
	for (j = 0; j < i-1; j++) 
		if(buffer[j] == '.' && buffer[j+1] == '.')
			Log(SORRY, "Parent directory (..) path names not supported", buffer, fd);


	if(!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6) )
		(void)strcpy(buffer,"GET /index.html");

	buflen = strlen(buffer);
	fstr = (char *)0;

	// now check wether the file extension in the request is legal
	for (i = 0; extension[i].ext != 0; i++) {
		len = strlen(extension[i].ext);
		if(!strncmp(&buffer[buflen-len], extension[i].ext, len)) { // ok, if it is a legal extension
			fstr = extension[i].filetype;
			break;
		}
	}

	if(fstr ==0) 
		Log(SORRY, "file extension type not supported", buffer, fd);

	// open the file for reading, don't worry, this is a '\0' after the URL
	if((file_fd = open(&buffer[5], O_RDONLY)) == -1)
		Log(SORRY, "failed to open file", &buffer[5], fd);


	// now let's log a legal request
	Log(LOG, "SEND", &buffer[5], hit);

	// we just prepared the http header
	(void)sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);

	// we send the buffer to the client
	(void)write(fd, buffer, strlen(buffer));


	// now we send the body of the http response, let's say, maybe a picture
	while((ret = read( file_fd, buffer, BUFSIZE)) > 0) 
		(void)write(fd, buffer,ret);


#ifdef LINUX
	sleep(1);	// to allow socket to drain
#endif

	exit(1);
}

	
int main(int argc, char **argv) {
	int i, hit;
	int listenfd,sockfd,  port;
	pid_t pid;
	size_t length;
	char *str;

	static struct sockaddr_in cli_addr; // static = initialised to zeros
	static struct sockaddr_in serv_addr; 


	// argc should be exactly 3, if not, try to find '-?'
	if ( argc < 3 || argc > 3 || !strcmp(argv[1], "-?")) {
		(void)printf("hint: nweb Port-Number Top-Directory\n\n"
					 "\tnweb is a small and very safe mini web server\n"
					 "\tnweb only serves out file/web pages with extensions named below\n"
					 "\t and only from the named directory or its  sub-directories.\n"
					 "\tThere is no fancy features = safe and secure.\n\n"
					 "\tExample: nweb 8181 /home/nwebdir &\n\n"
					 "\tOnly Supports:");

		// list the extensions that supported
		for (i = 0; extension[i].ext != 0; i++)
			(void)printf(" %s", extension[i].ext);
		

		(void)printf("\n\tNot Supported: URLs including \"..\", Java, javascript, CGI\n"
					 "\t Not Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
					 "\t No warranty given or implied\n\tWang Zhijun madfrogme@gmail.com\n");
		exit(0);
	}

	// check wether the top directory is any of them below, if so, WRONG
	if(!strncmp(argv[2], "/", 2) 	|| !strncmp(argv[2], "/etc", 5) || \
	   !strncmp(argv[2], "/bin", 5) || !strncmp(argv[2], "/lib", 5) || \
	   !strncmp(argv[2], "/tmp", 5) || !strncmp(argv[2], "/usr", 5) ||  \
	   !strncmp(argv[2], "/dev", 5) || !strncmp(argv[2], "/sbin", 6)) { 
		(void)printf("ERROR: Bad top directory %s, see nweb -?\n", argv[2]);
		exit(3);
	}

	// change to the directory containing contents
	if(chdir(argv[2]) == -1) {
		(void)printf("ERROR: Can't Change to directory %s\n", argv[2]);
		exit(4);
	}
	// become deamon, no wait() in parent, 
	if(fork() != 0)
		return 0; // parent returns OK to shell

	// ignore child death
	(void)signal(SIGCLD, SIG_IGN); 

	// ignore terminal hangups
	(void)signal(SIGHUP, SIG_IGN); 

	for(i = 0; i < 32; i++) 
		(void)close(i); // close open files

	(void)setpgrp(); // break away from process group

	// child process just become the leader of a new created group
	Log(LOG, "nweb starting", argv[1], getpid());

	// setup the network socket
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		Log(ERROR, "system call", "socket", 0);

	port = atoi(argv[1]);

	if(port < 0 || port > 60000) 
		Log(ERROR, "Invalid port number(try 1-60000)", argv[1], 0);


	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
		Log(ERROR, "system call", "bind", 0);

	if(listen(listenfd, 64) < 0)
		Log(ERROR, "system call", "listen", 0);

	for(hit = 1; ; hit++) {
		length = sizeof(cli_addr);

		if((sockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			Log(ERROR, "system call", "accept", 0);


		if ((pid = fork()) < 0)
			Log(ERROR, "system call", "fork", 0);

		else {
			if (pid == 0) { //child process
				(void)close(listenfd);
				web(sockfd, hit);
			}
			else {
				(void)close(sockfd);
			}
		}
	}

}

