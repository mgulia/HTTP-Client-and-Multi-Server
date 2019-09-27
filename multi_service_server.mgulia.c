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

int open_listenfd_UDP(int port)
{
  int listenfd, optval=1;
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
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
  pid_t childpid;

  if ((childpid = fork()) == 0){
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
	       if((int)ch <= 90 && (int)ch >= 65)
	       {
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

	       else{
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
      else if (errno == EACCES){
	       char out[] = "HTTP/1.0 403 Forbidden\r\n\r\n";
	        write(connfd, out, strlen(out));
      }
    }

   exit(0);
  }

  close(connfd);
}


int main(int argc, char **argv) {
  int listenfd, listenfd_UDP, connfd, port1, port2, clientlen;
  struct sockaddr_in clientaddr;
  struct hostent *hp;
  char *haddrp;
  pid_t childpid;

  fd_set rfds;
  struct timeval tv;
  int retval;
  char ch1, ch2;
  int maxfdp1;

  char * pathname;

  struct hostent *he;
  struct in_addr ip4addr;
  struct hostent *host;

  port1 = atoi(argv[1]);
  port2 = atoi(argv[2]);


  listenfd = open_listenfd(port1);
  listenfd_UDP = open_listenfd_UDP(port2);

  size_t n;
  char buf[MAXLINE];

  while(1){
    tv.tv_sec = 5; tv.tv_usec = 0; /* Wait up to five seconds. */
    FD_ZERO(&rfds);
    FD_SET(listenfd, &rfds);
    FD_SET(listenfd_UDP, &rfds);

    if (listenfd > listenfd_UDP){
      maxfdp1 = listenfd + 1;
    }

    else{
      maxfdp1 = listenfd_UDP + 1;
    }


    retval = select(maxfdp1, &rfds, NULL, NULL, &tv);
    if (FD_ISSET(listenfd, &rfds)){
       clientlen = sizeof(clientaddr);
       connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
       hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);

       helper(connfd);
    }

    else if (FD_ISSET(listenfd_UDP, &rfds)){
      clientlen = sizeof(clientaddr);
      int rec;
      char buffer [MAXLINE];
      rec = recvfrom (listenfd_UDP, buffer, MAXLINE, 0, (struct sockaddr *)&clientaddr, &clientlen);
      int arrLength = rec - 4;
      char arr [MAXLINE];

      memcpy(arr, buffer, arrLength);
      unsigned int i = 0;
      buffer[arrLength] = '\0';

      memcpy(&i, &buffer[arrLength], 4);
      i = ntohl(i);
      i += 1;
      i = htonl(i);

      inet_pton(AF_INET, buffer, &ip4addr);
      host = gethostbyaddr(&ip4addr, sizeof ip4addr, AF_INET);
      memcpy(&buffer[arrLength], &i, 4);

      char out [MAXLINE];
      sprintf(out, "%s", host->h_name);
      int length;
      length = strlen(out);
      memcpy(&out[length], &i, 4);

      sendto(listenfd_UDP, out, strlen(out)+4, 0, (struct sockaddr *)&clientaddr, clientlen);
    }

    close(connfd);
  }
}
