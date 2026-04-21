/* $begin tinymain */
/*
 * tiny.c - GET 메서드만 지원하는 단순 반복형 HTTP/1.0 웹 서버.
 *          정적(static) 콘텐츠와 동적(dynamic, CGI) 콘텐츠를 모두 제공한다.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

/* ── 함수 선언부 (프로토타입) ──────────────────────────────────────────────
 * C 언어에서는 함수를 사용하기 전에 미리 선언해야 한다.
 * 아래는 이 파일에서 구현할 함수들의 "목차"라고 생각하면 된다. */
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

/* ── main ──────────────────────────────────────────────────────────────────
 * 서버의 진입점.
 * 1) 포트(port)를 열어 클라이언트 연결을 기다린다(listen).
 * 2) 연결이 오면 수락(accept)하고, doit()으로 HTTP 요청을 처리한다.
 * 3) 처리가 끝나면 소켓을 닫고 다시 기다린다 → 반복(iterative) 서버. */
int main(int argc, char **argv)
{
  int listenfd, connfd;           /* 수신용 소켓 fd, 연결된 클라이언트 소켓 fd */
  char hostname[MAXLINE], port[MAXLINE]; /* 클라이언트의 호스트명, 포트 번호 */
  socklen_t clientlen;            /* 클라이언트 주소 구조체의 크기 */
  struct sockaddr_storage clientaddr; /* 클라이언트의 IP/포트 정보를 담는 구조체 */

  /* 실행 인자가 "프로그램 이름 + 포트 번호" 두 개인지 확인 */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  /* argv[1] 포트로 듣기(listen) 소켓을 열고 fd를 반환받는다 */
  listenfd = Open_listenfd(argv[1]);

  /* 무한 루프: 서버는 종료될 때까지 계속 요청을 받는다 */
  while (1)
  {
    clientlen = sizeof(clientaddr);

    /* 새 연결을 수락(accept)한다. 클라이언트가 올 때까지 여기서 블로킹된다 */
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept

    /* 클라이언트의 IP 주소와 포트 번호를 사람이 읽을 수 있는 문자열로 변환 */
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    doit(connfd);  /* 이 연결에서 온 HTTP 요청을 처리한다 */ // line:netp:tiny:doit
    Close(connfd); /* 처리가 끝났으니 소켓을 닫는다 */        // line:netp:tiny:close
  }
}

/* ── doit ──────────────────────────────────────────────────────────────────
 * 클라이언트 소켓 fd 하나를 받아 HTTP 요청 1건을 처리한다.
 * 흐름: 요청 읽기 → URI 분석 → 정적/동적 콘텐츠 전송 */
void doit(int fd)
{
  int is_static;                  /* 정적 콘텐츠면 1, 동적(CGI)이면 0 */
  struct stat sbuf;               /* 파일의 메타정보(크기, 권한 등)를 담는 구조체 */
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE]; /* 파일 경로, CGI 쿼리 인자 */
  rio_t rio;                      /* Robust I/O 버퍼 (csapp 라이브러리 제공) */

  /* ── 1단계: 요청 라인과 헤더를 읽는다 ── */
  Rio_readinitb(&rio, fd);        /* rio 버퍼를 fd 소켓에 연결해 초기화 */
  if (!Rio_readlineb(&rio, buf, MAXLINE)) /* 첫 번째 줄(요청 라인)을 읽는다 */
    return;                       /* 아무것도 안 왔으면 그냥 종료 */
  printf("%s", buf);

  /* 요청 라인 예: "GET /index.html HTTP/1.1" → method, uri, version으로 분리 */
  sscanf(buf, "%s %s %s", method, uri, version);

  /* Tiny 서버는 GET 메서드만 지원한다. 다른 메서드면 501 에러 반환 */
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
    return;
  }

  /* 나머지 요청 헤더들(Host:, User-Agent: 등)은 읽고 버린다 */
  read_requesthdrs(&rio);

  /* ── 2단계: URI를 파싱해 파일 경로와 CGI 인자를 추출한다 ── */
  is_static = parse_uri(uri, filename, cgiargs);

  /* 해당 파일이 실제로 존재하는지 확인한다 (없으면 404 반환) */
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    return;
  }

  /* ── 3단계: 정적 또는 동적 콘텐츠를 전송한다 ── */
  if (is_static) // 정적 콘텐츠인지
  { /* 정적 콘텐츠: 파일을 그대로 읽어서 전송 */
    /* 일반 파일인지(S_ISREG), 소유자 기준 읽기 권한이 있는지(S_IRUSR) 확인 */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  else
  { /* 동적 콘텐츠: CGI 프로그램을 실행해서 그 출력을 전송 */
    /* 일반 파일인지, 실행 권한이 있는지(S_IXUSR) 확인 */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

/* ── read_requesthdrs ──────────────────────────────────────────────────────
 * HTTP 요청 헤더를 한 줄씩 읽어서 출력만 하고 버린다.
 * 헤더의 끝은 빈 줄("\r\n")로 구분된다. */
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  /* "\r\n"(빈 줄)이 나올 때까지 계속 읽는다 */
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

/* ── parse_uri ─────────────────────────────────────────────────────────────
 * URI를 분석해 파일 경로(filename)와 CGI 인자(cgiargs)를 채운다.
 * 반환값: 정적 콘텐츠이면 1, 동적(CGI)이면 0.
 *
 * 예)
 *   정적: /index.html         → filename="./index.html", cgiargs=""
 *   동적: /cgi-bin/adder?x=1  → filename="./cgi-bin/adder", cgiargs="x=1" */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  { /* URI에 "cgi-bin"이 없으면 정적 콘텐츠 */
    strcpy(cgiargs, "");          /* CGI 인자는 없음 */
    strcpy(filename, ".");        /* 현재 디렉터리(".")를 기준으로 */
    strcat(filename, uri);        /* URI를 그대로 이어붙여 파일 경로 완성 */
    if (uri[strlen(uri) - 1] == '/') /* URI가 '/'로 끝나면 기본 페이지 제공 */
      strcat(filename, "home.html");
    return 1;                     /* 정적 */
  }
  else
  { /* URI에 "cgi-bin"이 있으면 동적 콘텐츠 */
    ptr = index(uri, '?');        /* '?' 위치를 찾는다 (쿼리 문자열 시작) */
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);   /* '?' 뒤를 CGI 인자로 복사 */
      *ptr = '\0';                /* URI에서 '?' 이후를 잘라낸다 */
    }
    else
      strcpy(cgiargs, "");        /* 쿼리 문자열이 없으면 빈 문자열 */
    strcpy(filename, ".");
    strcat(filename, uri);        /* CGI 프로그램의 경로 완성 */
    return 0;                     /* 동적 */
  }
}

/* ── serve_static ──────────────────────────────────────────────────────────
 * 정적 파일을 클라이언트에게 전송한다.
 * HTTP 응답 헤더를 먼저 보내고, 그 다음 파일 내용을 보낸다.
 * mmap()으로 파일을 메모리에 매핑해서 효율적으로 전송한다. */
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE];

  char buf[MAXBUF];
  char *p = buf;        /* buf에 순차적으로 쓰기 위한 포인터 */
  int n;
  int remaining = sizeof(buf); /* 남은 버퍼 공간 */

  /* 파일 확장자를 보고 Content-Type을 결정한다 (예: .html → text/html) */
  get_filetype(filename, filetype);

  /* ── HTTP 응답 헤더를 buf에 하나씩 이어붙인다 ── */
  n = snprintf(p, remaining, "HTTP/1.0 200 OK\r\n");
  p += n; remaining -= n;

  n = snprintf(p, remaining, "Server: Tiny Web Server\r\n");
  p += n; remaining -= n;

  n = snprintf(p, remaining, "Connection: close\r\n");
  p += n; remaining -= n;

  n = snprintf(p, remaining, "Content-length: %d\r\n", filesize);
  p += n; remaining -= n;

  /* 헤더 끝을 나타내는 빈 줄("\r\n\r\n")이 포함된다 */
  n = snprintf(p, remaining, "Content-type: %s\r\n\r\n", filetype);
  p += n; remaining -= n;

  /* 완성된 헤더를 클라이언트 소켓으로 전송 */
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* ── 파일 본문을 전송한다 ── */
  srcfd = Open(filename, O_RDONLY, 0); /* 파일을 읽기 전용으로 연다 */

  /* mmap: 파일을 메모리 주소 공간에 매핑한다 (디스크 I/O 없이 메모리처럼 접근) */
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);                  /* 매핑 후에는 fd가 필요 없으므로 닫는다 */
  Rio_writen(fd, srcp, filesize); /* 매핑된 메모리를 소켓으로 전송 */
  Munmap(srcp, filesize);        /* 메모리 매핑 해제 */
}

/* ── get_filetype ──────────────────────────────────────────────────────────
 * 파일 이름의 확장자를 보고 MIME 타입 문자열을 filetype에 채운다.
 * 브라우저가 파일을 어떻게 해석할지 알려주는 Content-Type 값에 쓰인다. */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain"); /* 알 수 없는 확장자는 평문으로 처리 */
}

/* ── serve_dynamic ─────────────────────────────────────────────────────────
 * CGI 프로그램을 자식 프로세스로 실행하고, 그 출력을 클라이언트에게 전달한다.
 * 흐름:
 *   1) 부모가 HTTP 응답의 첫 부분(상태 줄 + Server 헤더)을 먼저 전송
 *   2) fork()로 자식 프로세스 생성
 *   3) 자식: QUERY_STRING 환경변수 설정 → stdout을 소켓으로 리다이렉트 → execve로 CGI 실행
 *   4) 부모: 자식이 끝날 때까지 waitpid()로 대기 */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};
  pid_t pid; /* 자식 프로세스 ID */

  /* ── 1단계: 상태 줄과 Server 헤더를 먼저 전송 (나머지는 CGI가 출력) ── */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* ── 2단계: fork()로 자식 프로세스를 생성한다 ── */
  if ((pid = Fork()) < 0)
  { /* fork 실패: -1 반환 */
    perror("Fork failed");
    return;
  }

  if (pid == 0)
  { /* ── 자식 프로세스 (CGI를 실행하는 주체) ── */

    /* CGI 프로그램이 쿼리 인자를 읽어갈 환경변수를 설정한다
     * 예: "x=1&y=2" → QUERY_STRING=x=1&y=2 */
    setenv("QUERY_STRING", cgiargs, 1);

    /* stdout(표준 출력)을 클라이언트 소켓으로 리다이렉트한다.
     * 이렇게 하면 CGI가 printf()로 출력하는 내용이 직접 클라이언트에게 전달된다. */
    if (Dup2(fd, STDOUT_FILENO) < 0)
    {
      perror("Dup2 error");
      exit(1);
    }
    Close(fd); /* 원본 fd는 더 이상 필요 없으므로 닫는다 */

    /* CGI 프로그램을 실행한다. 성공하면 이 프로세스는 CGI로 교체되어 돌아오지 않는다 */
    Execve(filename, emptylist, environ);

    /* 여기까지 오면 Execve가 실패한 것이다 */
    perror("Execve error");
    exit(1);
  }
  else
  { /* ── 부모 프로세스 ── */
    int status;
    /* 자식(CGI)이 끝날 때까지 기다린다. 좀비 프로세스 방지를 위해 반드시 필요하다 */
    if (waitpid(pid, &status, 0) < 0)
    {
      perror("Wait error");
    }
    printf("Child process %d terminated with status %d\n", pid, status);
    /* 부모는 정상 반환 → doit()이 소켓을 닫는다 */
  }
}

/* ── clienterror ───────────────────────────────────────────────────────────
 * 클라이언트에게 HTTP 에러 응답을 보낸다.
 * 예: 404 Not Found, 403 Forbidden, 501 Not Implemented
 *
 * 매개변수:
 *   cause    - 에러를 유발한 요인 (파일명, 메서드명 등)
 *   errnum   - HTTP 상태 코드 문자열 (예: "404")
 *   shortmsg - 상태 코드에 대한 짧은 설명 (예: "Not found")
 *   longmsg  - 사용자에게 보여줄 긴 설명 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* ── HTML 형식의 응답 본문(body)을 만든다 ── */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* ── HTTP 응답 헤더를 전송한 뒤 본문을 전송한다 ── */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); /* 상태 줄 */
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");           /* 헤더: 타입 */
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); /* 헤더: 크기 + 빈 줄 */
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));                    /* 본문 */
}
