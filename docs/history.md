# 대화 히스토리 (2026-04-20)

- **목표**: Iterative 에코 서버 구현 (echo_server.c, echo_client.c, Makefile)
- **경로**: `/workspaces/webproxy_lab_docker/webproxy-lab/echo_server/`
- **참조 라이브러리**: `../csapp.h`, `../csapp.c` (상위 폴더)
- **현재 진행**: echo_server.c, echo_client.c, Makefile 구현 완료, 통합 테스트 성공
- **다음 단계**: -

## echo_client.c 구현 내용
- 헤더파일 역할 학습 (`stdio`, `stdlib`, `string`, `unistd`, `sys/socket`, `netinet/in`, `arpa/inet`)
- 클라이언트 vs 서버 역할 구분 (`connect` vs `bind/listen/accept`)
- 구현 순서: `socket()` → `connect()` → `fgets()` → `write()` → `read()` → `fprintf()` → `close()`
- 서버 주소 구조체 설정: `sin_family`, `sin_port(htons+atoi)`, `sin_addr(inet_pton)`
- `argc/argv` 로 IP주소, 포트번호 입력받기

## echo_server.c 구현 내용
- 서버는 포트번호 1개만 인자로 받음 (IP는 `INADDR_ANY`)
- 구현 순서: `socket()` → `bind()` → `listen()` → `accept()` 루프 → `read()` → `write()` → `close()`
- `listenfd` (대기 소켓) vs `connfd` (통신 소켓) 구분
- `accept()` 반환값을 `connfd`에 저장, `clientlen`은 `socklen_t`로 선언 후 `&clientlen` 포인터로 전달
- `sockaddr_in *` → `(struct sockaddr *)` 캐스팅 필요 (bind, accept 인자)
- read/write/close는 while 루프 안에서 실행 (클라이언트마다 반복)

## Makefile
- `all: echo_server echo_client` — 둘 다 빌드
- `echo_server.c`, `echo_client.c` 각각 `gcc -g -Wall`로 컴파일
- `clean`: 두 바이너리 모두 삭제

## 통합 테스트
- 서버 먼저 실행 (`./echo_server 9999`) → 클라이언트 연결 (`./echo_client 127.0.0.1 9999`)
- 서버가 `listen()` 상태여야 클라이언트 `connect()` 가능 (순서 중요)
- `connect()` 2번째 인자: `(struct sockaddr *)&serveraddr` 캐스팅 필요
- 에코 흐름: 클라이언트 `write` → 서버 `read/write` → 클라이언트 `read/fprintf`