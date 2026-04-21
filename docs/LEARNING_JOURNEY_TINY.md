# CSAPP Tiny Web Server 프로젝트 - 학습 여정

**프로젝트 시작:** 2026-04-21
**학습자:** 지혀니 (russee201916545@gmail.com)
**목표:** HTTP/1.0 웹 서버(tiny.c) 구축 및 프록시 서버 이해

---

## Session 1: 기초 개념 학습

### 학습 목표
- 네트워크 프로그래밍 기초 개념 이해
- 클라이언트-서버 모델 파악
- CSAPP 라이브러리 함수 이해

### 학습한 개념

#### 1. 클라이언트-서버 모델
**4단계 트랜잭션:**
1. 클라이언트가 요청 생성 (HTTP GET)
2. 서버가 요청 수신 및 처리
3. 서버가 응답 생성
4. 클라이언트가 응답 수신

**비유:** 카페 손님과 점원의 상호작용
- 손님(클라이언트) → 주문(요청) → 점원(서버) → 완료(응답) → 손님(결과)

#### 2. 소켓 프로그래밍
**핵심 함수:**
- `socket()`: 소켓 생성
- `bind()`: 포트에 바인딩
- `listen()`: 클라이언트 연결 대기
- `accept()`: 클라이언트 연결 수락
- `connect()`: 서버에 연결

**흐름:**
```
Server: socket() → bind() → listen() → accept()
Client: socket() → connect()
```

#### 3. 네트워크 주소지정
- **IP 주소:** 컴퓨터 식별 (예: 127.0.0.1)
- **포트 번호:** 프로세스 식별 (예: 8000)
- **조합:** IP:포트로 특정 프로세스에 도달

#### 4. HTTP 프로토콜
**요청 형식:**
```
GET /index.html HTTP/1.0
Host: localhost:8000
User-Agent: Mozilla/5.0
...
(빈 줄)
```

**응답 형식:**
```
HTTP/1.0 200 OK
Content-type: text/html
Content-length: 1234
...
(빈 줄)
<HTML 본문>
```

#### 5. 에코 서버 (Echo Server)
- 가장 기본적인 클라이언트-서버 패턴
- 클라이언트 메시지를 받아서 그대로 돌려보내기
- 소켓 프로그래밍 학습의 시작점

#### 6. 반복 서버 (Iterative Server)
- 한 번에 한 클라이언트씩 처리
- 클라이언트 처리 후 다음 클라이언트 받기
- tiny.c의 기본 구조

---

## Session 2: CSAPP 라이브러리 심화 학습

### csapp.h 파일 분석

**핵심 상수:**
- `MAXLINE = 8192`: 최대 텍스트 라인 길이
- `MAXBUF = 8192`: 최대 I/O 버퍼 크기
- `LISTENQ = 1024`: listen() 백로그 크기

**rio_t 구조체 (Robust I/O):**
```c
typedef struct {
    int rio_fd;                /* 파일 디스크립터 */
    int rio_cnt;               /* 미읽은 바이트 수 */
    char *rio_bufptr;          /* 다음 미읽은 바이트 */
    char rio_buf[RIO_BUFSIZE]; /* 내부 버퍼 (8192) */
} rio_t;
```

**래퍼 함수의 목적:**
- 에러 처리 자동화
- 시스템 콜 인터페이스 단순화
- 버퍼 관리 자동화

### 핵심 함수 이해

#### 1. Rio (Robust I/O) 함수
**rio_readinitb(rio_t *rp, int fd):**
- Rio 버퍼를 초기화
- 파일 디스크립터 fd와 연결
- 이 후 Rio_readlineb, Rio_writen 사용 가능

**rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen):**
- Rio 버퍼에서 한 라인 읽기 (\n까지)
- 자동 에러 처리
- 최대 maxlen 바이트까지 읽음

**rio_writen(int fd, void *usrbuf, size_t n):**
- 정확히 n 바이트를 쓰기
- 반복 호출로 모든 바이트 전송 보장
- 자동 에러 처리

#### 2. 메모리 매핑 (mmap)
```c
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
```

**역할:**
- 파일을 메모리에 매핑
- 메모리처럼 파일 접근 가능

**효율성:**
- 시스템 콜 횟수 감소
- OS 커널의 페이지 캐싱 활용
- DMA(Direct Memory Access) 가능
- 반복 Read() 호출보다 훨씬 효율적

#### 3. 프로세스 제어
**fork():**
- 현재 프로세스 복제
- 자식 프로세스 생성
- 부모, 자식 모두 계속 실행

**execve(const char *filename, char *const argv[], char *const envp[]):**
- 현재 프로세스를 새 프로그램으로 교체
- 현재 코드는 사라지고 새 프로그램 실행
- fork() 없이 호출하면 부모 프로세스까지 교체됨

**waitpid(pid_t pid, int *iptr, int options):**
- 자식 프로세스 종료 대기
- 좀비 프로세스 방지

#### 4. 파일 디스크립터 리디렉션
**dup2(int fd1, int fd2):**
- fd1을 fd2에 복제
- 예: dup2(fd, STDOUT_FILENO) → stdout을 fd로 리디렉션
- CGI 출력을 클라이언트 소켓으로 보낼 때 사용

---

## Session 3: 참고 구현 분석 (.proxy/tiny.c)

### 함수 구조

**main():**
- 포트 번호로 서버 포트 생성 (Open_listenfd)
- 무한 루프로 클라이언트 연결 수락 (Accept)
- 각 연결에 대해 doit() 호출
- 연결 종료 (Close)

**doit():**
- 요청 파싱 (read_requesthdrs)
- URI 파싱 (parse_uri)
- 정적/동적 콘텐츠 판별 후 처리

**read_requesthdrs():**
- 요청 라인 다음부터 빈 줄까지 읽기
- 헤더는 처리 없이 버림

**parse_uri():**
- URI에서 파일명과 CGI 인자 추출
- CGI 프로그램 여부 판별 (cgi-bin 포함 여부)

**serve_static():**
- 파일 타입 결정 (get_filetype)
- HTTP 응답 헤더 생성
- mmap으로 파일 메모리 매핑
- Rio_writen으로 클라이언트에 전송

**serve_dynamic():**
- fork로 자식 프로세스 생성
- setenv로 CGI 환경 변수 설정
- dup2로 stdout을 클라이언트 소켓으로 리디렉션
- execve로 CGI 프로그램 실행
- waitpid로 자식 프로세스 종료 대기

**get_filetype():**
- 파일 확장자 확인 (strstr)
- MIME 타입 결정

**clienterror():**
- HTTP 에러 응답 생성
- HTML 에러 메시지 포함

---

## Session 4: 검증 질문 및 답변

### Q1: 파일 확장자 찾는 방법
**질문:** "/home/tiny/cgi-bin/adder.cgi"에서 파일 확장자를 찾으려면?

**답변:** `strstr()` 함수 사용 ✅

**설명:**
```c
strstr(filename, ".html")  // ".html"을 찾으면 포인터 반환, 없으면 NULL
```

---

### Q2: mmap() 사용 이유
**질문:** serve_static()에서 Read() 대신 mmap()을 사용하는 이유?

**답변:** mmap이 훨씬 더 효율적 ✅

**이유:**
1. **시스템 콜 감소:** 반복 Read() 호출 대신 한 번의 매핑
2. **커널 최적화:** 페이지 캐싱, DMA 활용
3. **버퍼링 오버헤드 없음:** 파일이 가상 메모리에 직접 매핑

**비교:**
```
Read() 방식: Read() syscall → Read() syscall → Read() syscall → ...
mmap 방식: mmap() → (이미 메모리에 있음) → Write()
```

---

### Q3: fork() 필요성
**질문:** serve_dynamic()에서 execve() 전에 fork()가 필요한 이유?

**답변:** execve()는 프로세스 이미지를 교체하기 때문 ✅

**설명:**
- **fork() 없는 경우:** execve() → 부모 프로세스까지 교체 → 서버 중단
- **fork() 있는 경우:** 
  - 부모: 계속 돌면서 새 클라이언트 받음
  - 자식: CGI 프로그램으로 교체, 완료 후 종료

**코드 흐름:**
```c
fork()              // 자식 프로세스 생성
↓ (자식)            ↓ (부모)
setenv()            Accept() → 새 클라이언트 대기
dup2()
execve()            
(CGI 실행)
```

---

## Session 5: 함수 구현

### 구현 원칙
사용자의 학습 스타일:
- 완전한 코드 제시 ❌
- 개념과 힌트 제공 ✅
- 직접 구현 장려 ✅
- 이해도 확인 후 진행 ✅

### Function 1: get_filetype()

**학습 과정:**
1. strstr()의 역할 이해
2. 4개의 if-else 구조 설계
3. 기본값 (text/plain) 설정

**최종 구현:**
```c
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  }
  else {
    strcpy(filetype, "text/plain");
  }
}
```

**핵심 포인트:**
- `strstr()`: 부분 문자열 검색 (찾으면 포인터, 없으면 NULL)
- `strcpy()`: 문자열 복사 (filetype에 MIME 타입 저장)
- if-else 체인: 순서대로 확장자 확인

**학습 효과:**
- 문자열 함수 (strstr, strcpy) 실제 사용
- 조건문 (if-else) 구조화
- 함수 파라미터 이해 (포인터 사용)

---

### Function 2: clienterror()

**학습 과정:**
1. HTTP 에러 응답 구조 이해
   - HTML body 생성
   - HTTP 응답 헤더 생성
   - 클라이언트 전송

2. sprintf() 패턴 학습
   - 첫 번째: body에 초기값 저장
   - 이후: "%s새로운내용" 형식으로 계속 추가

3. Rio_writen() 사용
   - 각 부분을 나눠서 전송
   - strlen()으로 길이 계산

**최종 구현:**
```c
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // HTML body 구성
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // HTTP 응답 전송
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  
  sprintf(buf, "Content-type: text/html; charset=utf-8\r\n");
  Rio_writen(fd, buf, strlen(buf));
  
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));

  Rio_writen(fd, body, strlen(body));
}
```

**핵심 포인트:**
- **sprintf():** 형식화된 문자열 생성
- **Rio_writen():** 안전한 전송 (자동 에러 처리)
- **strlen():** 문자열 길이 계산
- **"%s..." 패턴:** 기존 내용에 계속 추가

**학습 효과:**
- sprintf() 형식 문자열 숙달
- Rio 라이브러리 실제 사용
- HTTP 응답 프로토콜 이해
- 문자열 처리 연습

---

### Function 3: read_requesthdrs()

**학습 과정:**
1. Rio_readlineb() 파라미터 이해
   - rp: 어디서 읽을 것 (Rio 버퍼)
   - buf: 어디에 저장 (문자 배열)
   - MAXLINE: 최대 몇 바이트 (버퍼 오버플로우 방지)

2. strcmp() 함수 이해
   - 두 문자열 비교
   - 같으면 0, 다르면 0 아닌 값 반환
   - while(strcmp(...)) → 다르면 true

3. while 루프 구조
   - 빈 줄("\r\n")이 아니면 계속 반복
   - 빈 줄 도달 시 루프 탈출

**최종 구현:**
```c
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
  }
}
```

**핵심 포인트:**
- **Rio_readlineb():** 한 라인씩 안전하게 읽기
- **strcmp():** 문자열 비교 (프로토콜 파싱에 필수)
- **while 루프:** 특정 조건까지 반복
- **HTTP 헤더 형식:** 빈 줄로 헤더 종료

**학습 효과:**
- Rio 버퍼 실제 사용
- strcmp() 함수 이해
- HTTP 프로토콜 헤더 처리
- 문자열 기반 루프 조건

---

## 핵심 개념 정리

### 1. C 문자열 함수
| 함수 | 목적 | 예시 |
|------|------|------|
| strstr() | 부분 문자열 검색 | `strstr("hello.html", ".html")` |
| strcpy() | 문자열 복사 | `strcpy(dest, src)` |
| strlen() | 문자열 길이 | `strlen("hello")` → 5 |
| strcmp() | 문자열 비교 | `strcmp("a", "b")` → 음수 |
| sprintf() | 형식 문자열 생성 | `sprintf(buf, "값: %d", 10)` |

### 2. Rio 라이브러리 패턴
```c
// 읽기
rio_t rio;
Rio_readinitb(&rio, fd);           // 초기화
Rio_readlineb(&rio, buf, MAXLINE); // 한 라인 읽기

// 쓰기
Rio_writen(fd, buf, strlen(buf));  // 쓰기
```

### 3. HTTP 프로토콜 구조
```
요청:
GET /path HTTP/1.0
Header1: value1
Header2: value2
(빈 줄)

응답:
HTTP/1.0 200 OK
Content-type: text/html
Content-length: 1234
(빈 줄)
<HTML 본문>
```

### 4. 함수 파라미터 패턴
- **char *:** 문자열 또는 배열 주소
- **rio_t *rp:** Rio 버퍼 구조체 (이미 초기화됨)
- **int fd:** 파일 디스크립터 (소켓)
- **MAXLINE:** 안전한 크기의 상수

---

### Function 4: parse_uri()

**학습 과정:**
1. URI 파싱 구조 이해
   - 정적 콘텐츠: "/index.html" → "./index.html"
   - 동적 콘텐츠: "/cgi-bin/adder?a=10&b=20" → 파일명과 인자 분리

2. CGI 여부 판별
   - `strstr(uri, "cgi-bin")` 사용
   - "cgi-bin" 포함 여부로 정적/동적 결정

3. 문자열 포인터 연산 이해
   - `ptr = strstr(uri, "?")` → '?'의 위치 찾기
   - `ptr+1` → '?' 다음부터의 문자열
   - `*ptr = '\0'` → 문자열 끝내기

4. 디렉토리 처리
   - URI가 "/"로 끝나면 home.html 추가
   - `uri[strlen(uri)-1]` → 마지막 문자 접근

**핵심 기술 설명:**

1) **CGI 여부 판별**
```c
if (!strstr(uri, "cgi-bin"))  // "cgi-bin" 없으면 정적
```

2) **마지막 문자 접근**
```c
uri[strlen(uri)-1]  // 마지막 문자
// uri = "/index.html" → 'l'
// uri = "/" → '/'
```

3) **'?' 검색 및 인자 분리**
```c
ptr = strstr(uri, "?");  // '?'의 위치 찾기
strcpy(cgiargs, ptr+1);  // '?' 다음부터 복사
*ptr = '\0';             // uri에서 '?' 이후 제거
```

4) ***ptr = '\0' 원리**
```
원본: "/cgi-bin/adder?a=10&b=20"
                      ↑
                   ptr가 가리킴

*ptr = '\0' 실행:
결과: uri는 "/cgi-bin/adder"로 인식됨
```

**최종 구현:**
```c
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // "cgi-bin" 여부로 정적/동적 콘텐츠 판별
  if (!strstr(uri, "cgi-bin")) {
    // 정적 콘텐츠인 경우
    strcpy(cgiargs, "");
    sprintf(filename, ".%s", uri);
    if (uri[strlen(uri)-1] == '/') {
        strcat(filename, "home.html");
    }
    return 0;
  }
  else {
    // 동적 콘텐츠인 경우
    ptr = strstr(uri, "?");
    if (ptr != NULL) {
        strcpy(cgiargs, ptr+1);
        *ptr = '\0';  // uri를 '?'에서 끊음
    }
    else {
        strcpy(cgiargs, "");
    }
    sprintf(filename, ".%s", uri);
    return 1;
  }
}
```

**핵심 포인트:**
- strstr()로 "cgi-bin" 검색해서 정적/동적 판별
- 포인터 연산 (ptr+1): 다음 위치로 이동
- *ptr = '\0': C 문자열 특성 활용 (null 종료)
- 배열 접근으로 마지막 문자 확인

**학습 효과:**
- 포인터 연산 실제 활용
- C 문자열의 null 종료 이해
- URI 파싱 로직 이해
- 정적/동적 콘텐츠 판별 구현

---

## 포인터와 문자열 심화 학습

### 1. 포인터 배열 접근
```c
char *ptr = strstr(uri, "?");
ptr[0] = '?'
ptr[1] = 'a'  // ptr+1과 같음
```

### 2. 문자열 끝내기 (*ptr = '\0')
```c
원본: "/cgi-bin/adder?a=10"
*ptr = '\0' 후: "/cgi-bin/adder"
```

### 3. 마지막 문자 접근
```c
uri = "/index.html"
길이 = 11
마지막 인덱스 = 11-1 = 10
uri[10] = 'l'
```

---

### Function 5: serve_static() - 정적 파일 전송 ✅ 완료

**학습 과정:**

serve_static()는 가장 복합적인 함수로, 여러 단계를 거쳐 구현합니다:
1. MIME 타입 결정
2. HTTP 응답 헤더 구성
3. 헤더 전송
4. 파일 열기
5. 파일 메모리 매핑
6. 매핑된 내용 전송
7. 정리 (메모리 언매핑, 파일 닫기)

**함수 시그니처:**
```c
void serve_static(int fd, char *filename, int filesize)
{
  // fd: 클라이언트 소켓
  // filename: 전송할 파일 경로
  // filesize: 파일 크기 (바이트)
}
```

**구현 단계별 학습:**

#### TODO 1: MIME 타입 결정
**개념:** 파일의 확장자를 보고 Content-type을 결정
```c
get_filetype(filename, filetype);
// filetype에 "text/html", "image/gif" 등이 저장됨
```

#### TODO 2: HTTP 응답 헤더 구성
**핵심 개념:** snprintf() 패턴으로 버퍼 관리
```c
char *p = buf;           // 현재 위치 포인터
int n;                   // 방금 쓴 바이트 수
int remaining = MAXBUF;  // 남은 공간
```

**패턴:**
```c
n = snprintf(p, remaining, "HTTP/1.0 200 OK\r\n");
p += n;           // 다음 쓸 위치로 이동
remaining -= n;   // 남은 공간 감소
```

#### TODO 3: 헤더 전송
```c
Rio_writen(fd, buf, strlen(buf));
```
- strlen(buf): 전체 헤더 길이 (마지막 n이 아님!)

#### TODO 4: 파일 열기
```c
srcfd = Open(filename, O_RDONLY, 0);
```
- O_RDONLY: 읽기 전용 모드
- 3번째 인자 0: 파일 생성 권한 (읽기만 하므로 무관)

#### TODO 5: 파일 메모리 매핑
```c
srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
```
**학습 포인트:**
- Open()의 3번째 인자: mode_t (파일 권한)
- Mmap()의 3번째 인자: int prot (보호 모드)
- 혼동 주의: O_RDONLY ≠ PROT_READ

#### TODO 6: 매핑된 파일 내용 전송 ✅
**개념:** 메모리에 매핑된 파일을 클라이언트에게 전송
```c
// TODO 6: 매핑된 파일 내용을 클라이언트에게 전송
Rio_writen(fd, srcp, filesize);
```
- srcp: Mmap()이 반환한 메모리 시작 주소
- filesize: 전송할 바이트 수

#### TODO 7: 정리 ✅
**개념:** 메모리 매핑과 파일 디스크립터 해제
```c
// TODO 7: 메모리 언매핑 + 파일 디스크립터 정리
Munmap(srcp, filesize);
Close(srcfd);
```
- Munmap + Close는 하나의 정리 단계
- Close()를 빠뜨리면 파일 디스크립터 누수 발생

**최종 구현:**
```c
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE];
  char buf[MAXBUF];
  char *p = buf;
  int n;
  int remaining = sizeof(buf);

  // TODO 1: MIME 타입 결정
  get_filetype(filename, filetype);

  // TODO 2: HTTP 응답 헤더를 buf에 생성
  n = snprintf(p, remaining, "HTTP/1.0 200 OK\r\n");
  p += n; remaining -= n;

  n = snprintf(p, remaining, "Server: Tiny Web Server\r\n");
  p += n; remaining -= n;

  n = snprintf(p, remaining, "Connection: close\r\n");
  p += n; remaining -= n;

  n = snprintf(p, remaining, "Content-length: %d\r\n", filesize);
  p += n; remaining -= n;

  n = snprintf(p, remaining, "Content-type: %s\r\n\r\n", filetype);
  p += n; remaining -= n;

  // TODO 3: 헤더 전송
  Rio_writen(fd, buf, strlen(buf));

  // TODO 4: 파일 열기
  srcfd = Open(filename, O_RDONLY, 0);

  // TODO 5: 파일을 메모리에 매핑
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

  // TODO 6: 매핑된 파일 내용을 클라이언트에게 전송
  Rio_writen(fd, srcp, filesize);

  // TODO 7: 메모리 언매핑 + 파일 디스크립터 정리
  Munmap(srcp, filesize);
  Close(srcfd);
}
```

**구현 상태:** ✅ TODO 1-7 전체 완료

**serve_static() 핵심 학습 내용:**
- snprintf() 포인터 패턴으로 버퍼 안전하게 관리
- mmap의 효율성 실체험 (시스템 콜 감소)
- 파일 I/O와 네트워크 I/O의 통합
- Munmap + Close 반드시 쌍으로 처리

---

## 다음 학습 단계

**현재 상태:**
- 완료: 5개 함수 (get_filetype, clienterror, read_requesthdrs, parse_uri, serve_static)
- 남은 함수: 2개 (serve_dynamic, doit)

**남은 함수:**
1. serve_dynamic() - fork/execve를 사용한 CGI 실행
2. doit() - 메인 트랜잭션 핸들러 (모든 함수 조율)

**예상 난이도:**
- serve_dynamic(): 어려움 (프로세스 제어, CGI 환경, 환경 변수)
- doit(): 어려움 (전체 흐름 조율, 에러 처리)

---

**최종 업데이트:** 2026-04-21
**진행 상황:** 5/7 함수 완료 (~71%)
**학습 시간:** ~ 5시간
**학습 효율:** 매우 높음 (개념 → 힌트 → 구현 → 검증 반복)
**핵심 학습 내용:** 문자열 처리, Rio 라이브러리, mmap, 포인터 연산