# 📚 csapp.h 함수 설명 가이드

`csapp.h`는 CMU Computer Systems 교재에서 제공하는 **wrapper 함수 라이브러리**입니다.
일반 Unix/Linux 함수를 래핑해서 **자동 에러 처리**를 해줍니다.

---

## 🎯 핵심 개념

### Wrapper 함수 패턴
```c
// 원본 함수 (에러 처리 필요)
int fd = open("file.txt", O_RDONLY);
if (fd < 0) {
    perror("open");  // 에러 처리
    exit(1);
}

// Wrapper 함수 (에러 자동 처리)
int fd = Open("file.txt", O_RDONLY, 0);  // 실패하면 자동으로 프로그램 종료
```

**장점**: 매번 if 문으로 에러 체크할 필요 없음

---

## 📋 함수 분류 및 설명

### 1️⃣ 상수 및 타입 정의

```c
#define MAXLINE    8192   // 최대 텍스트 라인 길이
#define MAXBUF     8192   // 최대 I/O 버퍼 크기
#define LISTENQ    1024   // listen() 두 번째 인자 (대기 큐)

typedef struct sockaddr SA;  // 소켓 주소 구조체 (편의상 typedef)

typedef struct {             // Rio 구조체 (robust I/O)
    int rio_fd;              // 파일 디스크립터
    int rio_cnt;             // 버퍼의 미읽은 바이트 수
    char *rio_bufptr;        // 다음 미읽은 바이트 포인터
    char rio_buf[RIO_BUFSIZE];  // 내부 버퍼 (8KB)
} rio_t;
```

---

### 2️⃣ 에러 처리 함수

이 함수들은 에러 메시지를 출력하고 프로그램을 종료합니다.

```c
void unix_error(char *msg);        // Unix 시스템 함수 에러
void posix_error(int code, char *msg);  // POSIX 함수 에러
void dns_error(char *msg);          // DNS 조회 에러
void gai_error(int code, char *msg);    // getaddrinfo 에러
void app_error(char *msg);          // 응용 프로그램 에러
```

**사용 예**:
```c
if (some_function() < 0) {
    unix_error("function name");  // "function name: Permission denied" 출력 후 exit(1)
}
```

---

### 3️⃣ 프로세스 제어 함수

자식 프로세스 생성, 대기 등을 담당합니다.

```c
pid_t Fork(void);
// 새 프로세스 생성 (CGI 프로그램 실행할 때 사용)
// 반환값: 부모는 자식 PID, 자식은 0
// 예: CGI 처리할 때
if (Fork() == 0) {        // 자식 프로세스
    execve(cgi_prog, ...);  // CGI 프로그램 실행
} 
// 부모는 계속 다음 클라이언트 요청 처리

void Execve(const char *filename, char *const argv[], char *const envp[]);
// 현재 프로세스를 새 프로그램으로 교체
// 반환되지 않음 (성공하면 현재 코드는 실행 안 됨)
// 예:
char *args[] = {"program", "arg1", "arg2", NULL};
Execve("./program", args, environ);  // execve 실패시 자동 종료

pid_t Wait(int *status);
pid_t Waitpid(pid_t pid, int *iptr, int options);
// 자식 프로세스 종료 대기
// 예: 자식이 끝날 때까지 기다림
Wait(NULL);  // 아무 자식이나 끝날 때까지 대기

void Kill(pid_t pid, int signum);
// 프로세스에 신호 전송
// 예: SIGTERM으로 프로세스 종료
Kill(pid, SIGTERM);

unsigned int Sleep(unsigned int secs);
// 지정된 초 만큼 대기
Sleep(1);  // 1초 대기

unsigned int Alarm(unsigned int seconds);
// 지정된 시간 후 SIGALRM 신호 전송 (타임아웃 구현)
```

---

### 4️⃣ 신호(Signal) 처리 함수

프로세스 간 비동기 이벤트 처리 (좀비 프로세스 방지 등)

```c
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
// 신호 핸들러 설정
// 예: 자식 종료 신호(SIGCHLD) 처리해서 좀비 프로세스 방지
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
Signal(SIGCHLD, sigchld_handler);

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
// 신호 차단/해제
// 예: 특정 신호 차단하기
sigset_t mask;
Sigemptyset(&mask);           // 빈 신호 집합
Sigaddset(&mask, SIGCHLD);    // SIGCHLD 추가
Sigprocmask(SIG_BLOCK, &mask, NULL);  // 차단

void Sigemptyset(sigset_t *set);   // 신호 집합 초기화 (비움)
void Sigfillset(sigset_t *set);    // 신호 집합 초기화 (모두 채움)
void Sigaddset(sigset_t *set, int signum);   // 신호 추가
void Sigdelset(sigset_t *set, int signum);   // 신호 제거
int Sigismember(const sigset_t *set, int signum);  // 신호 포함 여부 확인
```

---

### 5️⃣ Unix I/O 함수 (파일 디스크립터 기반)

일반 파일, 소켓 등을 다룹니다.

```c
int Open(const char *pathname, int flags, mode_t mode);
// 파일 열기
// flags: O_RDONLY(읽기), O_WRONLY(쓰기), O_RDWR(읽기쓰기), O_CREAT(생성)
// mode: 파일 권한 (예: 0644)
// 예:
int fd = Open("home.html", O_RDONLY, 0);  // 읽기 모드로 파일 열기

ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
// 읽기/쓰기
// 예:
char buf[MAXLINE];
ssize_t n = Read(fd, buf, MAXLINE);  // 최대 MAXLINE 바이트 읽기
Write(fd, buf, n);                   // 읽은 내용 쓰기

void Close(int fd);
// 파일 디스크립터 닫기
// 예:
Close(fd);  // 파일 닫기

off_t Lseek(int fildes, off_t offset, int whence);
// 파일 포인터 이동
// whence: SEEK_SET(처음), SEEK_CUR(현재), SEEK_END(끝)

int Dup2(int fd1, int fd2);
// 파일 디스크립터 복제 (fd1을 fd2로 복제)
// 주로 stdout/stdin 리다이렉트할 때 사용
// 예: CGI 프로그램의 stdout을 소켓으로 리다이렉트
Dup2(sock_fd, STDOUT_FILENO);  // 표준 출력이 소켓으로 가게 됨
execve(cgi_prog, ...);         // execve하면 CGI 출력이 소켓으로 감

void Stat(const char *filename, struct stat *buf);
void Fstat(int fd, struct stat *buf);
// 파일 정보 조회 (크기, 권한, 타입 등)
// 예:
struct stat sbuf;
Stat("home.html", &sbuf);
int filesize = sbuf.st_size;   // 파일 크기
int is_regular = S_ISREG(sbuf.st_mode);  // 일반 파일인지
int is_readable = S_IRUSR & sbuf.st_mode;  // 읽기 가능한지

int Select(int n, fd_set *readfds, fd_set *writefds, 
           fd_set *exceptfds, struct timeval *timeout);
// 여러 파일 디스크립터 동시 감시 (멀티플렉싱)
// 예: 읽기 가능할 때까지 대기
// (프록시의 병렬 처리에서는 스레드 사용하므로 덜 중요)
```

---

### 6️⃣ 디렉토리 함수

디렉토리 조회

```c
DIR *Opendir(const char *name);           // 디렉토리 열기
struct dirent *Readdir(DIR *dirp);        // 디렉토리 항목 읽기
int Closedir(DIR *dirp);                  // 디렉토리 닫기

// 예: 디렉토리 나열
DIR *dir = Opendir(".");
struct dirent *d;
while ((d = Readdir(dir)) != NULL) {
    printf("%s\n", d->d_name);  // 파일명 출력
}
Closedir(dir);
```

---

### 7️⃣ 메모리 할당 함수

```c
void *Malloc(size_t size);               // 메모리 할당
void *Realloc(void *ptr, size_t size);   // 메모리 재할당
void *Calloc(size_t nmemb, size_t size); // 할당 + 초기화
void Free(void *ptr);                    // 메모리 해제

// 예:
char *buf = Malloc(MAXLINE);  // MAXLINE 크기의 메모리 할당
strcpy(buf, "hello");
Free(buf);                     // 할당된 메모리 해제
```

---

### 8️⃣ 소켓 함수 (가장 중요!)

이것들이 웹서버/프록시 구현의 핵심입니다!

```c
int Socket(int domain, int type, int protocol);
// 소켓 생성
// domain: AF_INET (IPv4) 
// type: SOCK_STREAM (TCP)
// 예:
int sockfd = Socket(AF_INET, SOCK_STREAM, 0);

void Bind(int sockfd, struct sockaddr *my_addr, int addrlen);
// 소켓을 주소에 바인드 (포트에 연결)
// tiny 웹서버에서 포트 8000에 바인드할 때 사용
// 예:
Bind(sockfd, (SA *)&serveraddr, sizeof(serveraddr));

void Listen(int s, int backlog);
// 소켓을 리스닝 모드로 설정
// backlog: 대기 큐 크기
// 예:
Listen(sockfd, LISTENQ);

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
// 클라이언트 연결 수락
// 클라이언트가 연결할 때까지 블로킹 (대기)
// 반환값: 클라이언트 통신용 소켓 디스크립터
// 예:
struct sockaddr_storage clientaddr;
socklen_t clientlen = sizeof(clientaddr);
int connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
// 이제 connfd로 클라이언트와 통신 가능

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
// 원격 서버에 연결 (클라이언트 쪽)
// 예: 프록시가 웹서버에 연결
Connect(sockfd, (SA *)&serveraddr, sizeof(serveraddr));

void Setsockopt(int s, int level, int optname, 
                const void *optval, int optlen);
// 소켓 옵션 설정
// 예: 포트 재사용 가능하게 설정 (TIME_WAIT 대기시간 무시)
int optval = 1;
Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
```

---

### 9️⃣ 프로토콜 독립 함수 (중요!)

호스트명과 IP 주소 변환

```c
void Getaddrinfo(const char *node, const char *service, 
                 const struct addrinfo *hints, struct addrinfo **res);
// 호스트명 → IP 주소 변환 (DNS 조회)
// node: 호스트명 (예: "example.com")
// service: 포트 번호/서비스명 (예: "80", "http")
// hints: 조회 조건 (protocol 지정)
// res: 결과 포인터
// 예:
struct addrinfo hints, *res;
memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_INET;        // IPv4
hints.ai_socktype = SOCK_STREAM;  // TCP
Getaddrinfo("example.com", "80", &hints, &res);
// 이제 res에 IP 주소 정보 포함

void Getnameinfo(const struct sockaddr *sa, socklen_t salen,
                 char *host, size_t hostlen, char *serv, 
                 size_t servlen, int flags);
// IP 주소 → 호스트명 변환 (역DNS)
// 예: 클라이언트 주소 출력
char hostname[MAXLINE], port[MAXLINE];
Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, 
            port, MAXLINE, 0);
printf("Accepted connection from (%s, %s)\n", hostname, port);

void Freeaddrinfo(struct addrinfo *res);
// Getaddrinfo 결과 메모리 해제
// 예:
Freeaddrinfo(res);

void Inet_ntop(int af, const void *src, char *dst, socklen_t size);
void Inet_pton(int af, const char *src, void *dst);
// IP 주소 문자열 변환
// ntop: binary to printable (binary → 문자열)
// pton: printable to network (문자열 → binary)
```

---

### 🔟 Rio (Robust I/O) 함수 (매우 중요!)

**네트워크 통신에서 필수**. HTTP 요청/응답 읽고 쓸 때 사용합니다.

#### 기본 Rio 함수

```c
void rio_readinitb(rio_t *rp, int fd);
// Rio 구조체 초기화
// 예:
rio_t rio;
Rio_readinitb(&rio, connfd);  // 클라이언트 소켓으로 초기화

ssize_t rio_readn(int fd, void *usrbuf, size_t n);
// 정확히 n바이트 읽기 (소켓 또는 파일)
// 부분 읽기 문제 자동 처리
// 예: 10 바이트 읽음 (부분 읽기 발생해도 10바이트까지 읽음)
// 일반적으로 Rio_readn 쓰므로 덜 중요

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
// 한 줄 읽기 (줄바꿈(\n)까지)
// 내부 버퍼 사용해서 효율적
// 예: HTTP 요청 라인 읽기
// GET /index.html HTTP/1.0\r\n
// (최대 maxlen 바이트까지 읽음)

ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
// Rio 구조체를 사용한 정확히 n바이트 읽기
// 내부 버퍼 활용

ssize_t rio_writen(int fd, void *usrbuf, size_t n);
// 정확히 n바이트 쓰기 (소켓 또는 파일)
// 부분 쓰기 문제 자동 처리
// 예: HTTP 응답 전송
// "HTTP/1.0 200 OK\r\n..."
```

#### Wrapper Rio 함수 (대문자 = 자동 에러 처리)

```c
void Rio_readinitb(rio_t *rp, int fd);
ssize_t Rio_readn(int fd, void *usrbuf, size_t n);
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen(int fd, void *usrbuf, size_t n);

// 예: 클라이언트로부터 HTTP 요청 읽기
rio_t rio;
char buf[MAXLINE];

Rio_readinitb(&rio, connfd);                    // 초기화
Rio_readlineb(&rio, buf, MAXLINE);              // 첫 줄 읽기
printf("Request: %s", buf);                     // "GET /index.html HTTP/1.0"

// 나머지 헤더 읽기
while (strcmp(buf, "\r\n")) {
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Header: %s", buf);
}

// 응답 전송
char response[] = "HTTP/1.0 200 OK\r\nContent-length: 100\r\n\r\nHello";
Rio_writen(connfd, response, strlen(response));
```

**Rio가 중요한 이유**:
- 네트워크 통신에서 부분 읽기/쓰기 자동 처리
- 단순 read()/write()로는 데이터 손실 가능
- 내부 버퍼로 효율성 높음

---

### 1️⃣1️⃣ 클라이언트/서버 헬퍼 함수

**가장 편리한 함수들!** 복잡한 소켓 설정을 한 줄로 처리합니다.

```c
int Open_clientfd(char *hostname, char *port);
// 원격 서버에 연결하는 클라이언트 소켓 생성 및 연결
// hostname: 연결할 호스트명 (예: "example.com")
// port: 포트 (예: "80")
// 반환값: 연결된 소켓 디스크립터
// 예: 프록시가 원본 서버에 연결
int serverfd = Open_clientfd("example.com", "80");
Rio_writen(serverfd, request, strlen(request));  // 요청 전송

int Open_listenfd(char *port);
// 리스닝 소켓 생성 (서버용)
// port: 바인드할 포트 (예: "8000")
// 반환값: 리스닝 소켓 디스크립터
// 예: tiny 웹서버 시작
int listenfd = Open_listenfd("8000");
while (1) {
    int connfd = Accept(listenfd, ...);  // 클라이언트 수락
    doit(connfd);                         // 요청 처리
    Close(connfd);
}
```

**이 두 함수가 하는 일** (원래는 10단계 이상 필요):
1. DNS 조회 (Getaddrinfo)
2. 소켓 생성 (Socket)
3. 소켓 옵션 설정 (Setsockopt)
4. 포트 바인드 (Bind)
5. 리스닝 시작 (Listen)
6. 연결 (Connect)
7. 에러 처리

---

### 1️⃣2️⃣ Pthread (스레드) 함수

병렬 처리할 때 사용 (Proxy Phase 2)

```c
void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, 
                   void * (*routine)(void *), void *argp);
// 새 스레드 생성
// tidp: 스레드 ID 저장 포인터
// routine: 스레드 함수 (void* 반환, void* 인자)
// argp: 스레드 함수에 전달할 인자
// 예: 각 클라이언트마다 스레드 생성
void *thread_func(void *arg) {
    int connfd = *((int*)arg);
    handle_client(connfd);
    free(arg);
    return NULL;
}

while (1) {
    int connfd = Accept(listenfd, ...);
    int *arg = Malloc(sizeof(int));
    *arg = connfd;
    pthread_t tid;
    Pthread_create(&tid, NULL, thread_func, arg);
    Pthread_detach(tid);  // 자동 정리
}

void Pthread_join(pthread_t tid, void **thread_return);
// 스레드 종료 대기
// 예: 스레드가 끝날 때까지 대기
Pthread_join(tid, NULL);

void Pthread_detach(pthread_t tid);
// 스레드 자동 정리 설정 (명시적 join 필요 없음)

pthread_t Pthread_self(void);
// 현재 스레드 ID 조회
```

---

### 1️⃣3️⃣ 세마포어 함수

스레드 동기화 (캐싱/멀티스레드에서 사용)

```c
void Sem_init(sem_t *sem, int pshared, unsigned int value);
// 세마포어 초기화
// value: 초기값 (1 = 뮤텍스, n = 세마포어)

void P(sem_t *sem);  // wait() - 세마포어 감소
// 리소스 사용 전에 호출 (리소스 없으면 대기)

void V(sem_t *sem);  // signal() - 세마포어 증가
// 리소스 사용 후에 호출 (대기 중인 스레드 깨움)

// 예: 뮤텍스 (상호 배제)
sem_t mutex;
Sem_init(&mutex, 0, 1);  // 초기값 1

// 스레드 1
P(&mutex);          // 잠금
// 임계 영역 (critical section)
shared_variable++;
V(&mutex);          // 해제

// 스레드 2
P(&mutex);          // 잠금 (스레드 1이 가진 동안 대기)
// 임계 영역
shared_variable++;
V(&mutex);          // 해제
```

---

## 🚀 Tiny/Proxy 구현에서 자주 쓰는 함수

### Tiny 웹서버
```c
// 서버 시작
listenfd = Open_listenfd("8000");     // 포트 8000에 바인드

// 클라이언트 수락
connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, 
            port, MAXLINE, 0);         // 클라이언트 주소 출력

// HTTP 요청 읽기
rio_t rio;
Rio_readinitb(&rio, connfd);
Rio_readlineb(&rio, buf, MAXLINE);    // GET 라인 읽기
Rio_readlineb(&rio, buf, MAXLINE);    // 헤더 읽기

// 파일 정보
Stat(filename, &sbuf);
filesize = sbuf.st_size;
is_regular = S_ISREG(sbuf.st_mode);

// 파일 읽기
fd = Open(filename, O_RDONLY, 0);
Read(fd, buf, MAXLINE);
Close(fd);

// 응답 전송
Rio_writen(connfd, response, strlen(response));

// CGI 프로세스
if (Fork() == 0) {
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(connfd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
}
Wait(NULL);
```

### Proxy 서버
```c
// 프록시 서버 시작
listenfd = Open_listenfd("4500");

// 클라이언트 수락
connfd = Accept(listenfd, ...);

// 클라이언트 요청 읽기
Rio_readinitb(&client_rio, connfd);
Rio_readlineb(&client_rio, buf, MAXLINE);

// 원본 서버 연결
serverfd = Open_clientfd(hostname, port);
Rio_readinitb(&server_rio, serverfd);

// 요청 포워딩
Rio_writen(serverfd, request, strlen(request));

// 응답 읽기 및 전달
while (Rio_readlineb(&server_rio, buf, MAXLINE) > 0) {
    Rio_writen(connfd, buf, strlen(buf));
}
```

---

## 📚 참고 사항

1. **대문자 = Wrapper** (자동 에러 처리)
   - Open() vs open()
   - Read() vs read()
   - Rio_writen() vs rio_writen()

2. **Rio 함수 사용 기본 패턴**
   ```c
   rio_t rio;
   Rio_readinitb(&rio, fd);        // 초기화
   Rio_readlineb(&rio, buf, size); // 라인 읽기 (반복)
   Rio_writen(fd, buf, size);      // 쓰기
   ```

3. **소켓 기본 흐름**
   ```
   서버: Open_listenfd → Accept → read/write → Close
   클라이언트: Open_clientfd → write/read → Close
   ```

4. **파일 다루기**
   ```c
   Open() → Read()/Write() → Close()
   또는 Stat() → 정보 조회
   ```

---

**💡 팁**: Tiny/Proxy 구현할 때는 위의 "자주 쓰는 함수" 섹션만 자세히 공부해도 충분합니다!
