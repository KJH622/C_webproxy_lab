# CSAPP Tiny 웹 서버 프로젝트 - 학습 기록

**프로젝트 목표:** 정적 및 동적 콘텐츠를 제공하는 간단한 HTTP/1.0 웹 서버(tiny.c) 구축

**학습자 학습 스타일:** 
- 완전한 코드보다 힌트 선호
- 직접 코드 작성으로 연습
- 다음 단계로 넘어가기 전 이해도 확인
- 구현 전 개념 이해 필수

---

## 학습 진행 상황 요약

### Phase 1: 개념 기초 ✅
**학습 주제:**
- 클라이언트-서버 모델: 4단계 트랜잭션 (요청 → 처리 → 응답 → 정리)
- 소켓 프로그래밍: socket(), bind(), listen(), accept(), connect()
- 네트워크 주소지정: IP 주소와 포트 번호
- HTTP 프로토콜: 요청(메소드, URI, 버전, 헤더), 응답(상태, 헤더, 본문)
- 에코 서버: 기본 클라이언트-서버 패턴
- 반복 서버: 클라이언트를 한 번에 하나씩 순차 처리

**상태:** 완료 및 이해함 ✅

---

### Phase 2: csapp 라이브러리 심화 ✅
**핵심 파일 이해:**
- **csapp.h:** 함수 선언 및 구조체 정의 헤더 파일
  - 상수: MAXLINE (8192), MAXBUF (8192), LISTENQ (1024)
  - rio_t 구조체: 버퍼 I/O 관리
  - 래퍼 함수: Unix I/O, 소켓, 프로세스 제어, Rio, Pthreads

- **csapp.c (래퍼 함수):** 에러 처리, 소켓 헬퍼, Rio 버퍼 I/O
  - Rio 함수: rio_readinitb(), rio_readlineb(), rio_writen()
  - 소켓 헬퍼: open_listenfd(), open_clientfd()
  - 프로세스 헬퍼: fork(), execve(), waitpid()

**핵심 개념:**
- Rio 버퍼링: 자동 에러 처리 I/O 작업
- 메모리 매핑(mmap): 가상 메모리를 통한 효율적 파일 전송
- 프로세스 제어: fork()로 자식 프로세스 생성, execve()로 프로세스 교체
- 파일 디스크립터 리디렉션(dup2): 표준 출력을 소켓으로 리디렉션

**상태:** 완료 및 이해함 ✅

---

### Phase 3: 참고 구현 분석 ✅
**파일:** .proxy/tiny.c (345줄 - 완전한 구현 참고)

**함수 개요:**
1. `main()`: 포트 처리, accept 루프, 연결 관리
2. `doit()`: 주요 HTTP 트랜잭션 처리
3. `read_requesthdrs()`: HTTP 헤더 파싱
4. `parse_uri()`: 정적/동적 콘텐츠 판별
5. `serve_static()`: mmap을 사용한 파일 전송
6. `get_filetype()`: MIME 타입 감지
7. `serve_dynamic()`: fork/execve를 사용한 CGI 실행
8. `clienterror()`: 에러 응답 생성

**핵심 패턴:**
- 정적 파일 서빙: mmap 효율성
- 동적 콘텐츠: fork → dup2 → execve 패턴
- 에러 처리: HTML 본문을 포함한 HTTP 에러 응답

**상태:** 분석 및 이해함 ✅

---

### Phase 4: 검증 질문 ✅

**Q1: "/home/tiny/cgi-bin/adder.cgi"에서 파일 확장자를 찾는 방법?**
- **답변:** strstr() 함수 ✅ 정답
- **이유:** strstr()은 파일명 내에서 부분 문자열(.html, .gif 등) 검색

**Q2: serve_static()에서 Read() 대신 mmap()을 사용하는 이유?**
- **답변:** mmap이 훨씬 더 효율적임 ✅ 정답
- **이유:** 
  - 시스템 콜 횟수 감소 (컨텍스트 스위칭 감소)
  - OS 커널 최적화: 페이지 캐싱, DMA
  - 파일이 가상 메모리에 직접 매핑 (버퍼링 오버헤드 없음)

**Q3: serve_dynamic()에서 execve() 전에 fork()가 필요한 이유?**
- **답변:** execve()는 프로세스 이미지를 교체함. fork() 없이 execve()를 호출하면 부모 프로세스(웹 서버)가 교체되어 새로운 클라이언트를 받을 수 없음. fork()로 자식 프로세스를 만들면 자식만 교체되고 부모는 계속 실행됨. ✅ 정답
- **이유:** 반복 서버 운영을 위해 부모-자식 프로세스 분리 필수

---

## 현재 단계: 구현 Phase

### 구현할 함수 (순서대로)

**1. get_filetype()** ✅ 완료
   - 입력: 파일명 (예: "index.html")
   - 출력: MIME 타입 (예: "text/html")
   - 구현: strstr()로 확장자 확인 (.html, .gif, .jpg, .png)
   - 난이도: 매우 쉬움 (8줄)
   - **상태:** 완성됨!

**2. clienterror()** ← 다음 함수
   - HTTP 에러 응답 생성
   - 클라이언트에게 HTML 본문 반환

**3. read_requesthdrs()**
   - HTTP 요청 헤더 파싱
   - 빈 줄(\r\n\r\n)까지 읽기

**4. parse_uri()**
   - URI가 정적/동적 콘텐츠인지 판별
   - 파일명 및 CGI 인자 추출

**5. serve_static()**
   - 클라이언트에게 정적 파일 전송
   - 효율적인 전송을 위해 mmap 사용

**6. serve_dynamic()**
   - CGI 프로그램 실행
   - fork → setenv → dup2 → execve → waitpid 패턴 사용

**7. doit()**
   - 주요 요청 처리 핸들러
   - 모든 위 함수 조율

---

## 구현 핵심 원칙

1. **모든 I/O에 Rio 사용:** Rio 함수가 자동으로 버퍼링 및 에러 처리
2. **정적 파일에 mmap 사용:** Read() 반복 호출보다 효율적
3. **execve() 전에 fork() 실행:** 부모 프로세스 보존하여 여러 클라이언트 처리 가능
4. **에러 처리:** 항상 HTTP 에러 응답 전송
5. **파일 권한:** 파일 접근 권한 및 MIME 타입 존중

---

## 프로젝트 파일

- **tiny/tiny.c:** 메인 서버 구현 (현재 작업 중)
- **.proxy/tiny.c:** 참고/답안 구현 (345줄)
- **csapp.h:** 함수 선언 (199줄)
- **csapp.c:** 래퍼 함수 구현
- **테스트 파일:** 테스트용 입력 파일 (index.html, adder.cgi 등)

---

## 테스트 전략

함수 구현 후:
1. 컴파일: `gcc -o tiny tiny.c csapp.c -lpthread`
2. 서버 실행: `./tiny 8000`
3. 정적 파일 테스트: `http://localhost:8000/index.html`
4. 동적(CGI) 테스트: `http://localhost:8000/cgi-bin/adder?a=10&b=20`
5. 에러 처리 검증: 잘못된 URI 및 권한 테스트

---

**마지막 수정:** 2026-04-21
**현재 작업:** get_filetype() 함수 구현 완료 ✅
**다음 목표:** clienterror() 함수 구현 시작
**최종 목표:** 7개 함수 모두 완성 및 통합 테스트
