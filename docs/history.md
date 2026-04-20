# 대화 히스토리 (2026-04-20)

- **목표**: Iterative 에코 서버 구현 (echo_server.c, echo_client.c, Makefile)
- **경로**: `/workspaces/webproxy_lab_docker/webproxy-lab/echo_server/`
- **참조 라이브러리**: `../csapp.h`, `../csapp.c` (상위 폴더)
- **현재 진행**: echo_client.c 구현 완료
- **다음 단계**: echo_server.c 구현 → Makefile → 빌드/테스트

## echo_client.c 구현 내용
- 헤더파일 역할 학습 (`stdio`, `stdlib`, `string`, `unistd`, `sys/socket`, `netinet/in`, `arpa/inet`)
- 클라이언트 vs 서버 역할 구분 (`connect` vs `bind/listen/accept`)
- 구현 순서: `socket()` → `connect()` → `fgets()` → `write()` → `read()` → `fprintf()` → `close()`
- 서버 주소 구조체 설정: `sin_family`, `sin_port(htons+atoi)`, `sin_addr(inet_pton)`
- `argc/argv` 로 IP주소, 포트번호 입력받기
