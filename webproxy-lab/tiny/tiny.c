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
 * get_filetype - 파일명을 보고 파일 형식을 결정
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

/*
 * clienterror - HTTP 에러 응답을 클라이언트에게 전송
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // TODO 1: HTML body 구성 (여러 sprintf 호출)
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // TODO 2: HTTP 응답 라인을 buf에 생성하고 Rio_writen()으로 전송
  // buf에 "HTTP/1.0 404 Not Found\r\n" 저장 후 전송
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  
  // TODO 3: Content-type 헤더 전송
  // buf에 "Content-type: text/html; charset=utf-8\r\n" 저장 후 전송
  sprintf(buf, "Content-type: text/html; charset=utf-8\r\n");
  Rio_writen(fd, buf, strlen(buf));
  
  // TODO 4: Content-length 헤더와 빈 줄 전송
  // buf에 "Content-length: 123\r\n\r\n" 저장 후 전송 → body의 길이를 계산해서 헤더에 포함!
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));

  // TODO 5: body 전송
  // 이미 만들어진 body (완전한 HTML) 전송
  Rio_writen(fd, body, strlen(body));
}

/*
 * read_requesthdrs - HTTP 요청의 헤더들을 읽고 버리기
 */
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  // TODO 1: 한 라인 읽기
  // Rio_readlineb(어디서 읽을 것인가, 읽은 데이터를 어디에 저장, 최대 몇 바이트)
  Rio_readlineb(rp, buf, MAXLINE);
  
  // TODO 2: "\r\n"이 아니면 계속 반복
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
  }
}

/*
 * parse_uri - HTTP 요청의 URI를 파싱해서 정적/동적 콘텐츠 판별
 * 정적 파일이면 0 반환. CGI 프로그램이면 1 반환.
 */
// uri : 요청 라인에서 받은 URI
// filename : 파일명을 저장할 포인터 (출력)
// cgiargs : CGI 인자를 저장할 포인터 (출력)
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // TODO 1: "cgi-bin" 여부로 정적/동적 콘텐츠 판별
  if (!strstr(uri, "cgi-bin")) {
    // 정적 콘텐츠인 경우
    // TODO 1-1: cgiargs를 빈 문자열로 설정
    strcpy(cgiargs, "");
    // TODO 1-2: filename을 "." + uri로 설정 (예: "./index.html")
    sprintf(filename, ".%s", uri);
    // TODO 1-3: uri가 "/"로 끝나면 filename에 "home.html" 추가 (예: "./")
    if (uri[strlen(uri)-1] == '/') { // 마지막 문자 접근
        strcat(filename, "home.html");
    }
    // TODO 1-4: return 0 (정적 콘텐츠)
    return 0;
  }
  else {
    // 동적 콘텐츠인 경우
    // TODO 2-1: '?'를 찾아서 ptr에 저장
    ptr = strstr(uri, "?");
    // TODO 2-2: ptr이 NULL이 아니면 cgiargs에 저장, 아니면 빈 문자열
    if (ptr != NULL) {
        strcpy(cgiargs, ptr+1);
        *ptr = '\0'; // ← 이 라인 추가! uri를 '?'에서 끊음
    }
    else {
        strcpy(cgiargs, "");
    }
    // TODO 2-3: filename에 프로그램 경로 저장 (예: "./cgi-bin/adder")
    sprintf(filename, ".%s", uri);
    // TODO 2-4: return 1 (동적 콘텐츠)
    return 1;
  }
}

/*
 * serve_static - Copy a regular file back to the client
 */
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE];
  char buf[MAXBUF];
  char *p = buf;
  int n;
  int remaining = sizeof(buf);

  // TODO 1: get_filetype() 호출해서 파일 타입 결정
  
  // TODO 2: HTTP 응답 헤더를 buf에 생성
  //         - "HTTP/1.0 200 OK\r\n"
  //         - "Server: Tiny Web Server\r\n"
  //         - "Connection: close\r\n"
  //         - "Content-length: %d\r\n" (파일크기)
  //         - "Content-type: %s\r\n\r\n" (파일타입)
  //         snprintf 사용할 것!
  
  // TODO 3: Rio_writen()으로 헤더 전송
  
  // TODO 4: 파일을 Open()으로 연다
  
  // TODO 5: Mmap()으로 파일을 메모리에 매핑
  
  // TODO 6: Close(srcfd) - 파일 디스크립터 종료
  
  // TODO 7: Rio_writen()으로 파일 데이터 전송
  
  // TODO 8: Munmap()으로 메모리 매핑 해제
}

/*
 * serve_dynamic - Run a CGI program on behalf of the client
 */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};
  pid_t pid;

  // TODO 1: 응답의 첫 부분 (상태 줄 + Server 헤더) 전송
  //         sprintf(buf, "HTTP/1.0 200 OK\r\n");
  //         Rio_writen(fd, buf, strlen(buf));
  //         sprintf(buf, "Server: Tiny Web Server\r\n");
  //         Rio_writen(fd, buf, strlen(buf));
  
  // TODO 2: Fork()로 자식 프로세스 생성
  
  if (pid == 0) {
    // 자식 프로세스에서
    // TODO 2-1: setenv("QUERY_STRING", cgiargs, 1)
    
    // TODO 2-2: Dup2(fd, STDOUT_FILENO) - stdout을 클라이언트로 리다이렉트
    
    // TODO 2-3: Close(fd) - 원본 fd 종료
    
    // TODO 2-4: Execve(filename, emptylist, environ) - CGI 실행
  }
  else {
    // 부모 프로세스에서
    // TODO 3: Waitpid(pid, &status, 0) - 자식 프로세스 대기
  }
}

/*
 * doit - Handle one HTTP request/response transaction
 */
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  // ── 1단계: 요청 읽기 ──
  // TODO 1-1: Rio_readinitb(&rio, fd) - rio 버퍼 초기화
  
  // TODO 1-2: Rio_readlineb(&rio, buf, MAXLINE) - 요청 라인 읽기
  
  // TODO 1-3: sscanf(buf, "%s %s %s", method, uri, version) - 파싱
  
  // TODO 1-4: GET이 아니면 clienterror() 호출하고 return
  
  // TODO 1-5: read_requesthdrs(&rio) - 나머지 헤더 읽기
  
  // ── 2단계: URI 분석 ──
  // TODO 2-1: is_static = parse_uri(uri, filename, cgiargs)
  
  // TODO 2-2: stat(filename, &sbuf) 호출해서 파일 존재 확인
  //          파일이 없으면 clienterror() 호출하고 return
  
  // ── 3단계: 콘텐츠 전송 ──
  if (is_static) {
    // 정적 콘텐츠
    // TODO 3-1: S_ISREG(sbuf.st_mode) && S_IRUSR & sbuf.st_mode 확인
    //          권한 없으면 clienterror() 호출하고 return
    
    // TODO 3-2: serve_static(fd, filename, sbuf.st_size) 호출
  }
  else {
    // 동적 콘텐츠
    // TODO 3-3: S_ISREG(sbuf.st_mode) && S_IXUSR & sbuf.st_mode 확인
    //          권한 없으면 clienterror() 호출하고 return
    
    // TODO 3-4: serve_dynamic(fd, filename, cgiargs) 호출
  }
}