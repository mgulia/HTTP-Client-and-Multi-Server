#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAXLINE 512


int open_clientfd(char *hostname, int port)
{
  int clientfd;
  struct hostent *hp;
  struct sockaddr_in serveraddr;
  if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return - 1; /* check errno for cause of error */
  /* Fill in the server's IP address and port */
  if ((hp = gethostbyname(hostname)) == NULL)
    return -2; /* check h_errno for cause of error */

  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)hp->h_addr, (char *)&serveraddr.sin_addr.s_addr, hp->h_length);

  serveraddr.sin_port = htons(port);
  /* Establish a connection with the server */
  if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    return -1;

  return clientfd;
  //returns file scripter used to communicate with server
}



int main(int argc, char **argv)
{
  int clientfd, port;
  char *host, buf[MAXLINE];
  char *pathName;

  host = argv[1];
  port = atoi(argv[2]);;
  pathName = argv[3];

  sprintf(buf, "GET %s HTTP/1.0\r\n\r\n", pathName);

  clientfd = open_clientfd(host, port);

  if (clientfd < 0){
    printf("Error opening connection \n");
    exit(0);
  }

  write(clientfd, buf, strlen(buf));
  bzero(buf, MAXLINE);
  while (read(clientfd, buf, MAXLINE)) { fputs(buf, stdout); }

  char ok[2];
  memcpy(ok, &buf[9], 3);

  int val1 = atoi(ok);
  int val2 = 200;


  if (val1 != val2){
    exit(0);
  }

  char* path2;
  char pattern[] = "\r\n\r\n";

  path2 = (strstr(buf, pattern)) + 4;

  int size = strlen(path2) - 1;

  if(path2[size] == '\n'){
      path2[size] = '\0';
  }

  sprintf(buf, "GET %s HTTP/1.0\r\n\r\n", path2);

  clientfd = open_clientfd(host, port);

  if (clientfd < 0){
    printf("Error opening connection \n");
    exit(0);
  }

  int bytes = 0;
  write(clientfd, buf, strlen(buf));

  int loop = 1;
  while (loop){
    bytes = read(clientfd, buf, MAXLINE-1);
    if (bytes == 0){
       loop = 0;
     }

     buf[bytes] = '\0';
     fputs(buf, stdout);
    }

  close(clientfd);
  exit(0);
}
