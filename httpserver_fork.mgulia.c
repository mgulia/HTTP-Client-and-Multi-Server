#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define LISTENQ 10
#define MAXLINE 512
extern int errno;

int open_listenfd(int port)
{
  int listenfd, optval=1;
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1;

  /* Eliminates "Address already in use" error from bind. */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		 (const void *)&optval , sizeof(int)) < 0)
    return -1;

  /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;

  /* Make it a listening socket ready to accept
     connection requests */
  if (listen(listenfd, LISTENQ) < 0)
    return -1;

  return listenfd;
}

void echo(int connfd)
{
  size_t n;
  char buf[MAXLINE];

  while((n = read(connfd, buf, MAXLINE)) != 0) {
    printf("server received %d bytes\n", n);
    write(connfd, buf, n);
  }
}

void helper(int connfd)
{
  size_t n;
  char buf[MAXLINE];
  char * content;

  bzero(buf, MAXLINE);
  read(connfd, buf, MAXLINE);
  content = &buf[4];
  int len = strlen(content);
  content[len-13] = '\0';
  int cipher = 0;
  cipher = atoi(&content[(strlen(content)-1)]);
  content[len-15] = '\0';

  if (fopen(content, "r") != NULL){
    char out[] = "HTTP/1.0 200 OK\r\n\r\n";
    write(connfd, out, strlen(out));
    char ch;
    FILE *fptr;
    fptr = fopen(content, "r");
    char asciiVal = 0;

    ch = fgetc(fptr);
    while (ch != EOF){

      if((int)ch <= 90 && (int)ch >= 65){
	       asciiVal = (int)ch - cipher;
	        if (asciiVal < 65){
	           asciiVal = 91 - (65 - asciiVal);
	        }
	    }

      else if ((int)ch <= 122 && (int)ch >= 97)
	    {
	       asciiVal = (int)ch - cipher;
	        if (asciiVal < 97){
	           asciiVal = 123-(97-asciiVal);
	        }
	    }

	    else
	    {
	       asciiVal = (int)ch;
	    }

	    write(connfd, &asciiVal, 1);
	    ch = fgetc(fptr);
    }

    fclose(fptr);
  }

  else if(fopen(content, "r") == NULL){
    if (errno == ENOENT){
      char out[] = "HTTP/1.0 404 NOT FOUND\r\n\r\n";
	     write(connfd, out, strlen(out));
    }
    else if (errno == EACCES)
    {
      char out[] = "HTTP/1.0 403 Forbidden\r\n\r\n";
	     write(connfd, out, strlen(out));
    }
  }
}


int main(int argc, char **argv) {
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;
  struct hostent *hp;
  char *haddrp;
  pid_t childpid;

  port = atoi(argv[1]); /* the server listens on a port passed
			   on the command line */
  listenfd = open_listenfd(port);

  size_t n;
  char buf[MAXLINE];
  char * content;

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
    hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
		       sizeof(clientaddr.sin_addr.s_addr), AF_INET);

    if ((childpid = fork()) == 0){
      helper(connfd);
      close(listenfd);
      exit(0);
    }
    close(connfd);
  }
}
