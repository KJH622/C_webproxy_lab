# tiny.c 함수 설명

## main
- 포트를 열고 클라이언트 연결을 무한 대기
- 연결이 오면 `doit` 호출 후 연결 종료
- 이미 구현되어 있음

---

## doit(int fd)
**역할:** HTTP 요청 한 건을 처리하는 핵심 함수

순서:
1. 요청 첫 줄 읽기 → `GET /home.html HTTP/1.0`
2. GET 메서드인지 확인 (아니면 에러 응답)
3. 나머지 헤더 읽기 → `read_requesthdrs`
4. URI 파싱 → `parse_uri`
5. 정적/동적 분기 → `serve_static` or `serve_dynamic`

---

## read_requesthdrs(rio_t *rp)
**역할:** 요청 헤더를 끝까지 읽고 버리기

- 첫 줄(GET ...) 이후 헤더들을 한 줄씩 읽음
- 빈 줄(`\r\n`)이 오면 헤더 끝이므로 멈춤
- 헤더 내용은 tiny에서 사용하지 않으므로 그냥 읽기만 함

---

## parse_uri(char *uri, char *filename, char *cgiargs)
**역할:** URI를 실제 파일 경로와 CGI 인자로 변환

- `cgi-bin`이 포함되면 → 동적 (cgiargs 추출, return 0)
- 없으면 → 정적 (filename에 `./` 붙이기, return 1)
- 예: `/home.html` → `./home.html`
- 예: `/cgi-bin/adder?1&2` → `./cgi-bin/adder`, cgiargs=`1&2`

---

## serve_static(int fd, char *filename, int filesize)
**역할:** 파일을 읽어서 HTTP 응답으로 클라이언트에게 전송

순서:
1. HTTP 응답 헤더 전송 (`HTTP/1.0 200 OK` 등)
2. 파일을 열어서 `mmap`으로 메모리에 매핑
3. `rio_writen`으로 파일 내용 전송
4. `munmap`으로 메모리 해제

---

## get_filetype(char *filename, char *filetype)
**역할:** 파일 확장자로 MIME 타입 결정

| 확장자 | MIME 타입 |
|--------|-----------|
| .html  | text/html |
| .gif   | image/gif |
| .png   | image/png |
| .jpg   | image/jpeg |
| 그 외  | text/plain |

---

## serve_dynamic(int fd, char *filename, char *cgiargs)
**역할:** CGI 프로그램을 실행해서 결과를 클라이언트에게 전송

순서:
1. HTTP 응답 헤더 전송
2. `fork`로 자식 프로세스 생성
3. 자식: 환경변수 설정 후 `execve`로 CGI 프로그램 실행
4. 부모: 자식이 끝날 때까지 대기

---

## clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
**역할:** 에러 발생 시 HTTP 에러 응답 전송

- 예: 404 Not Found, 501 Not Implemented
- HTML 형식으로 에러 메시지를 만들어서 전송

---

## 함수 요약 표

| 함수 | 역할 |
|---|---|
| `main` | 포트를 열고 클라이언트 연결을 무한 반복으로 수락 |
| `doit` | HTTP 요청 1건을 처음부터 끝까지 처리 |
| `read_requesthdrs` | 요청 헤더를 읽고 버림 (파싱 안 함) |
| `parse_uri` | URI → 파일 경로 + CGI 인자로 분해 |
| `serve_static` | 파일을 mmap으로 읽어 그대로 전송 |
| `get_filetype` | 확장자로 MIME 타입 결정 |
| `serve_dynamic` | fork+execve로 CGI 프로그램 실행, stdout을 소켓으로 연결 |
| `clienterror` | HTML 에러 페이지를 HTTP 응답으로 전송 |
