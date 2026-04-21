/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

/*
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype)
{
  // TODO 1: filename에 ".html"이 있으면 filetype을 "text/html"로 설정
  if (strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  }
  // TODO 2: filename에 ".gif"이 있으면 filetype을 "image/gif"로 설정
  else if (strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  }
  // TODO 3: filename에 ".png"이 있으면 filetype을 "image/png"로 설정
  else if (strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  }
  // TODO 4: filename에 ".jpg"이 있으면 filetype을 "image/jpeg"로 설정
  else if (strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  }
  // TODO 5: 위의 어느것도 아니면 filetype을 "text/plain"으로 설정
  else {
    strcpy(filetype, "text/plain");
  }
  
  // 힌트: strstr() 함수 사용! (문자열 안에 부분 문자열 찾기)
  // 예: if (strstr(filename, ".html"))
  //       strcpy(filetype, "text/html");
}