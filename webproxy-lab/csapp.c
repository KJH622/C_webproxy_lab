/*
 * csapp.c - CS:APP3e 교재에서 제공하는 헬퍼 함수 모음
 *
 * 이 파일은 크게 다음 역할을 합니다:
 *   1. 에러 처리 함수 (unix_error, posix_error 등)
 *   2. 프로세스 제어 래퍼 (Fork, Execve 등)
 *   3. 시그널 처리 래퍼 (Signal, Sigprocmask 등)
 *   4. Sio - 시그널 핸들러 안에서도 안전하게 쓸 수 있는 I/O
 *   5. Unix I/O 래퍼 (Open, Read, Write, Close 등)
 *   6. 소켓 인터페이스 래퍼 (Socket, Bind, Listen, Accept, Connect 등)
 *   7. RIO - 끊김 없이 안정적으로 읽고 쓰는 I/O 패키지
 *   8. 클라이언트/서버 헬퍼 (open_clientfd, open_listenfd)
 *   9. Pthreads / 세마포어 래퍼
 *
 * [래퍼(Wrapper)란?]
 *   기존 함수를 한 겹 감싸서, 에러가 나면 바로 프로그램을 종료하고
 *   메시지를 출력해 주는 함수입니다.
 *   예) socket() → Socket(), read() → Read()
 *   대문자로 시작하는 함수가 래퍼 함수입니다.
 */
/* $begin csapp.c */
#include "csapp.h"

/**************************
 * Error-handling functions
 * 에러 처리 함수들
 **************************/
/* $begin errorfuns */
/* $begin unixerror */

/*
 * unix_error - Unix 스타일 에러 출력 후 종료
 *
 * errno(에러 번호)를 이용해 에러 메시지를 stderr에 출력하고 프로그램을 종료합니다.
 * strerror(errno)는 errno 숫자를 사람이 읽을 수 있는 문자열로 변환합니다.
 *
 * 사용 예: open() 실패 → unix_error("Open error")
 *   출력:  "Open error: No such file or directory"
 */
void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

/*
 * posix_error - POSIX 스레드 함수용 에러 출력 후 종료
 *
 * pthread_create 등 POSIX 함수는 errno 대신 반환값으로 에러 코드를 줍니다.
 * 그 코드를 받아서 에러 메시지를 출력합니다.
 */
void posix_error(int code, char *msg) /* Posix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

/*
 * gai_error - getaddrinfo() 함수용 에러 출력 후 종료
 *
 * getaddrinfo()는 호스트 이름(예: "www.google.com")을 IP 주소로 변환하는 함수입니다.
 * 실패 시 errno 대신 자체 에러 코드를 반환하므로 gai_strerror()로 문자열 변환합니다.
 */
void gai_error(int code, char *msg) /* Getaddrinfo-style error */
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}

/*
 * app_error - 애플리케이션 수준의 에러 출력 후 종료
 *
 * errno와 무관하게 단순히 메시지만 출력하고 종료합니다.
 * 프로그래머가 직접 감지한 논리 에러 등에 사용합니다.
 */
void app_error(char *msg) /* Application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
/* $end errorfuns */

/*
 * dns_error - 구식 gethostbyname() 에러 처리 (현재는 거의 사용 안 함)
 *
 * gethostbyname()은 스레드 안전하지 않아 현재는 getaddrinfo()를 권장합니다.
 */
void dns_error(char *msg) /* Obsolete gethostbyname error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}


/*********************************************
 * Wrappers for Unix process control functions
 * 프로세스 제어 함수 래퍼들
 ********************************************/

/*
 * Fork - fork() 래퍼
 *
 * [fork란?]
 *   현재 프로세스를 복사해 자식 프로세스를 만드는 시스템 콜입니다.
 *   부모에게는 자식의 PID(프로세스 ID)가 반환되고, 자식에게는 0이 반환됩니다.
 *
 * Fork()는 fork() 실패 시 에러를 출력하고 종료합니다.
 */
/* $begin forkwrapper */
pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

/*
 * Execve - execve() 래퍼
 *
 * [execve란?]
 *   현재 프로세스의 메모리를 새 프로그램으로 완전히 교체하는 함수입니다.
 *   보통 Fork() 후 자식 프로세스에서 사용합니다.
 *
 *   filename : 실행할 프로그램 경로
 *   argv     : 프로그램에 전달할 인자 배열
 *   envp     : 환경 변수 배열
 */
void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if (execve(filename, argv, envp) < 0)
	unix_error("Execve error");
}

/*
 * Wait - wait() 래퍼
 *
 * [wait란?]
 *   자식 프로세스 하나가 종료될 때까지 부모가 기다리는 함수입니다.
 *   자식이 종료되면 그 PID를 반환하고, status에 종료 상태를 저장합니다.
 */
/* $begin wait */
pid_t Wait(int *status)
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait error");
    return pid;
}
/* $end wait */

/*
 * Waitpid - waitpid() 래퍼
 *
 * Wait()보다 정밀한 버전입니다.
 *   pid     : 기다릴 자식 PID (-1이면 아무 자식)
 *   iptr    : 종료 상태 저장 포인터
 *   options : WNOHANG(즉시 반환), WUNTRACED(중지된 자식도 감지) 등
 */
pid_t Waitpid(pid_t pid, int *iptr, int options)
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0)
	unix_error("Waitpid error");
    return(retpid);
}

/*
 * Kill - kill() 래퍼
 *
 * [kill이란?]
 *   이름과 달리 프로세스를 "죽이는" 것만이 아니라 시그널을 보내는 함수입니다.
 *   예: Kill(pid, SIGTERM) → 해당 프로세스에 종료 시그널을 보냄
 */
/* $begin kill */
void Kill(pid_t pid, int signum)
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill error");
}
/* $end kill */

/*
 * Pause - pause() 래퍼
 *
 * 시그널이 올 때까지 프로세스를 잠재웁니다.
 * 시그널을 받으면 즉시 반환합니다.
 */
void Pause()
{
    (void)pause();
    return;
}

/*
 * Sleep - sleep() 래퍼
 *
 * 지정한 초(secs) 동안 프로세스를 잠재웁니다.
 * 시그널에 의해 일찍 깨어나면 남은 시간을 반환합니다.
 */
unsigned int Sleep(unsigned int secs)
{
    unsigned int rc;

    if ((rc = sleep(secs)) < 0)
	unix_error("Sleep error");
    return rc;
}

/*
 * Alarm - alarm() 래퍼
 *
 * 지정한 초 뒤에 현재 프로세스에 SIGALRM 시그널을 보내도록 예약합니다.
 * 이전에 예약된 알람이 있으면 그 남은 시간을 반환합니다.
 */
unsigned int Alarm(unsigned int seconds) {
    return alarm(seconds);
}

/*
 * Setpgid - setpgid() 래퍼
 *
 * 프로세스의 프로세스 그룹 ID를 변경합니다.
 * 주로 시그널을 특정 그룹 전체에 보내기 위해 그룹을 재배정할 때 사용합니다.
 */
void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid error");
    return;
}

/*
 * Getpgrp - getpgrp() 래퍼
 *
 * 현재 프로세스의 프로세스 그룹 ID를 반환합니다.
 */
pid_t Getpgrp(void) {
    return getpgrp();
}

/************************************
 * Wrappers for Unix signal functions
 * 시그널 처리 함수 래퍼들
 ***********************************/

/*
 * Signal - sigaction() 기반 시그널 핸들러 등록 래퍼
 *
 * [시그널이란?]
 *   OS가 프로세스에게 비동기적으로 보내는 알림입니다.
 *   예: Ctrl+C → SIGINT, 자식 종료 → SIGCHLD
 *
 * Signal()은 단순한 signal() 대신 sigaction()을 사용해 더 안정적입니다.
 *   SA_RESTART : 시그널로 중단된 시스템 콜을 자동으로 재시작합니다.
 *
 * 이전에 등록된 핸들러를 반환합니다.
 */
/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* 현재 처리 중인 시그널과 같은 종류는 블록 */
    action.sa_flags = SA_RESTART; /* 인터럽트된 시스템 콜을 자동 재시작 */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}
/* $end sigaction */

/*
 * Sigprocmask - sigprocmask() 래퍼
 *
 * 시그널 마스크(차단할 시그널 집합)를 변경합니다.
 *   how = SIG_BLOCK   : set에 있는 시그널들을 추가로 차단
 *   how = SIG_UNBLOCK : set에 있는 시그널들의 차단을 해제
 *   how = SIG_SETMASK : 마스크를 set으로 완전히 교체
 */
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
	unix_error("Sigprocmask error");
    return;
}

/*
 * Sigemptyset - 시그널 집합을 비웁니다 (모든 시그널 차단 해제 상태)
 */
void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
	unix_error("Sigemptyset error");
    return;
}

/*
 * Sigfillset - 시그널 집합에 모든 시그널을 추가합니다
 */
void Sigfillset(sigset_t *set)
{
    if (sigfillset(set) < 0)
	unix_error("Sigfillset error");
    return;
}

/*
 * Sigaddset - 시그널 집합에 특정 시그널 하나를 추가합니다
 */
void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
	unix_error("Sigaddset error");
    return;
}

/*
 * Sigdelset - 시그널 집합에서 특정 시그널 하나를 제거합니다
 */
void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("Sigdelset error");
    return;
}

/*
 * Sigismember - 특정 시그널이 집합에 포함되어 있는지 확인합니다
 * 포함되어 있으면 1, 없으면 0 반환
 */
int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) < 0)
	unix_error("Sigismember error");
    return rc;
}

/*
 * Sigsuspend - 원자적으로 마스크를 교체하고 시그널을 기다립니다
 *
 * 시그널이 올 때까지 sleep하다가, 시그널을 받으면 깨어납니다.
 * sigsuspend()는 항상 -1을 반환하므로 EINTR 여부로 정상 동작 확인합니다.
 */
int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* 항상 -1 반환 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}

/*************************************************************
 * The Sio (Signal-safe I/O) package
 * 시그널 안전 I/O 패키지
 *
 * [왜 별도 패키지가 필요한가?]
 *   printf(), malloc() 같은 표준 라이브러리 함수들은 내부적으로
 *   전역 상태(락, 버퍼)를 사용하기 때문에, 시그널 핸들러 안에서
 *   이 함수들을 호출하면 데드락이나 데이터 손상이 발생할 수 있습니다.
 *   Sio 패키지는 write() 시스템 콜만 직접 사용해 이 문제를 피합니다.
 *************************************************************/

/* Private sio functions (내부 전용 함수들) */

/* $begin sioprivate */
/*
 * sio_reverse - 문자열을 뒤집습니다 (K&R 방식)
 *
 * sio_ltoa에서 숫자를 문자열로 변환할 때,
 * 낮은 자리부터 순서대로 쌓인 문자를 뒤집기 위해 사용합니다.
 */
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/*
 * sio_ltoa - long 정수를 문자열로 변환합니다 (K&R 방식)
 *
 *   v : 변환할 정수
 *   s : 결과를 저장할 문자열 버퍼
 *   b : 진법 (10 = 십진수, 16 = 16진수 등)
 *
 * 음수 처리도 지원합니다 (2016년에 버그 수정됨).
 */
static void sio_ltoa(long v, char s[], int b)
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
	v = -v;

    do {
        /* 나머지를 문자로 변환: 0~9이면 '0'+'나머지', 10~이면 'a'+'나머지-10' */
        s[i++] = ((c = (v % b)) < 10)  ?  c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
	s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s); /* 역순으로 쌓였으니 뒤집어서 정상 순서로 */
}

/*
 * sio_strlen - 문자열 길이를 반환합니다 (strlen의 시그널 안전 버전)
 */
static size_t sio_strlen(char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}
/* $end sioprivate */

/* Public Sio functions (외부에서 사용 가능한 함수들) */
/* $begin siopublic */

/*
 * sio_puts - 문자열을 표준 출력(stdout)에 씁니다
 *
 * printf() 대신 write() 시스템 콜을 직접 사용해 시그널 안전합니다.
 * 쓴 바이트 수를 반환합니다.
 */
ssize_t sio_puts(char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s));
}

/*
 * sio_putl - long 정수를 표준 출력에 씁니다
 *
 * 내부적으로 sio_ltoa로 문자열 변환 후 sio_puts로 출력합니다.
 */
ssize_t sio_putl(long v) /* Put long */
{
    char s[128];

    sio_ltoa(v, s, 10); /* 10진수로 변환 */
    return sio_puts(s);
}

/*
 * sio_error - 에러 메시지를 출력하고 프로세스를 즉시 종료합니다
 *
 * exit() 대신 _exit()를 사용하는 이유:
 *   _exit()는 atexit 핸들러, stdio 버퍼 플러시 등을 건너뛰고 즉시 종료합니다.
 *   시그널 핸들러 안에서는 _exit()가 안전합니다.
 */
void sio_error(char s[]) /* Put error message and exit */
{
    sio_puts(s);
    _exit(1);
}
/* $end siopublic */

/*******************************
 * Wrappers for the SIO routines
 * Sio 함수의 래퍼들 (에러 처리 포함)
 ******************************/
ssize_t Sio_putl(long v)
{
    ssize_t n;

    if ((n = sio_putl(v)) < 0)
	sio_error("Sio_putl error");
    return n;
}

ssize_t Sio_puts(char s[])
{
    ssize_t n;

    if ((n = sio_puts(s)) < 0)
	sio_error("Sio_puts error");
    return n;
}

void Sio_error(char s[])
{
    sio_error(s);
}

/********************************
 * Wrappers for Unix I/O routines
 * Unix I/O 함수 래퍼들
 *
 * [파일 디스크립터(fd)란?]
 *   리눅스에서 파일, 소켓, 파이프 등 모든 I/O 자원을 나타내는 정수입니다.
 *   0=stdin, 1=stdout, 2=stderr, 3이상=사용자가 열거나 만든 자원
 ********************************/

/*
 * Open - open() 래퍼
 *
 * 파일을 열고 파일 디스크립터(fd)를 반환합니다.
 *   pathname : 파일 경로
 *   flags    : O_RDONLY(읽기), O_WRONLY(쓰기), O_CREAT(없으면 생성) 등
 *   mode     : O_CREAT 사용 시 파일 권한 (예: 0644)
 */
int Open(const char *pathname, int flags, mode_t mode)
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
	unix_error("Open error");
    return rc;
}

/*
 * Read - read() 래퍼
 *
 * fd에서 최대 count 바이트를 buf에 읽어옵니다.
 * 실제로 읽은 바이트 수를 반환합니다 (count보다 작을 수 있음).
 * EOF이면 0, 에러이면 -1 반환.
 */
ssize_t Read(int fd, void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0)
	unix_error("Read error");
    return rc;
}

/*
 * Write - write() 래퍼
 *
 * buf의 내용 count 바이트를 fd에 씁니다.
 * 실제로 쓴 바이트 수를 반환합니다.
 */
ssize_t Write(int fd, const void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
	unix_error("Write error");
    return rc;
}

/*
 * Lseek - lseek() 래퍼
 *
 * 파일의 읽기/쓰기 위치(오프셋)를 변경합니다.
 *   whence = SEEK_SET : 파일 처음부터 offset
 *   whence = SEEK_CUR : 현재 위치에서 offset
 *   whence = SEEK_END : 파일 끝에서 offset
 */
off_t Lseek(int fildes, off_t offset, int whence)
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
	unix_error("Lseek error");
    return rc;
}

/*
 * Close - close() 래퍼
 *
 * 파일 디스크립터를 닫아 자원을 해제합니다.
 * 소켓도 close()로 닫습니다.
 * 닫지 않으면 자원 누수(resource leak)가 발생합니다.
 */
void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
	unix_error("Close error");
}

/*
 * Select - select() 래퍼
 *
 * [select란?]
 *   여러 파일 디스크립터를 동시에 감시해, 읽기/쓰기가 준비된 fd를 알려줍니다.
 *   이를 통해 하나의 프로세스가 여러 연결을 동시에 처리할 수 있습니다(I/O 멀티플렉싱).
 *
 *   n        : 감시할 fd의 최댓값 + 1
 *   readfds  : 읽기 준비 여부를 감시할 fd 집합
 *   writefds : 쓰기 준비 여부를 감시할 fd 집합
 *   timeout  : 최대 대기 시간 (NULL이면 무한 대기)
 */
int Select(int  n, fd_set *readfds, fd_set *writefds,
	   fd_set *exceptfds, struct timeval *timeout)
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
	unix_error("Select error");
    return rc;
}

/*
 * Dup2 - dup2() 래퍼
 *
 * fd1을 fd2로 복사합니다(fd2가 이미 열려 있으면 먼저 닫음).
 * 주로 표준 입출력 리다이렉션에 사용합니다.
 * 예: Dup2(sockfd, STDOUT_FILENO) → stdout을 소켓으로 연결
 */
int Dup2(int fd1, int fd2)
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	unix_error("Dup2 error");
    return rc;
}

/*
 * Stat - stat() 래퍼
 *
 * 파일 경로로 파일 메타데이터(크기, 권한, 수정 시각 등)를 가져옵니다.
 * buf에 struct stat 구조체로 정보가 채워집니다.
 */
void Stat(const char *filename, struct stat *buf)
{
    if (stat(filename, buf) < 0)
	unix_error("Stat error");
}

/*
 * Fstat - fstat() 래퍼
 *
 * Stat과 동일하나, 파일 경로 대신 파일 디스크립터(fd)를 사용합니다.
 */
void Fstat(int fd, struct stat *buf)
{
    if (fstat(fd, buf) < 0)
	unix_error("Fstat error");
}

/*********************************
 * Wrappers for directory function
 * 디렉토리 함수 래퍼들
 *********************************/

/*
 * Opendir - opendir() 래퍼
 *
 * 디렉토리를 열고 DIR* 포인터를 반환합니다.
 * Readdir()로 내부 파일 목록을 순회할 수 있습니다.
 */
DIR *Opendir(const char *name)
{
    DIR *dirp = opendir(name);

    if (!dirp)
        unix_error("opendir error");
    return dirp;
}

/*
 * Readdir - readdir() 래퍼
 *
 * 디렉토리에서 다음 항목(파일 또는 하위 디렉토리)을 읽습니다.
 * 모든 항목을 읽으면 NULL을 반환합니다.
 * errno를 0으로 초기화해 EOF와 에러를 구분합니다.
 */
struct dirent *Readdir(DIR *dirp)
{
    struct dirent *dep;

    errno = 0;
    dep = readdir(dirp);
    if ((dep == NULL) && (errno != 0))
        unix_error("readdir error");
    return dep;
}

/*
 * Closedir - closedir() 래퍼
 *
 * Opendir()로 연 디렉토리를 닫아 자원을 해제합니다.
 */
int Closedir(DIR *dirp)
{
    int rc;

    if ((rc = closedir(dirp)) < 0)
        unix_error("closedir error");
    return rc;
}

/***************************************
 * Wrappers for memory mapping functions
 * 메모리 매핑 함수 래퍼들
 ***************************************/

/*
 * Mmap - mmap() 래퍼
 *
 * [mmap이란?]
 *   파일의 내용을 메모리 주소 공간에 직접 매핑합니다.
 *   이를 통해 파일을 read()/write() 없이 메모리 접근처럼 사용할 수 있습니다.
 *   웹 서버에서 정적 파일을 빠르게 전송할 때 유용합니다.
 *
 *   addr   : 매핑할 주소 (NULL이면 OS가 결정)
 *   len    : 매핑할 크기
 *   prot   : 접근 권한 (PROT_READ, PROT_WRITE 등)
 *   flags  : MAP_PRIVATE(개인 복사), MAP_SHARED(공유) 등
 *   fd     : 매핑할 파일의 디스크립터
 *   offset : 파일 내 시작 오프셋
 */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    void *ptr;

    // 실패 시, -1이 아니라 MAP_FAILED를 리턴 (그 값이 ((void *) -1)임.)
    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1)) 
	unix_error("mmap error");
    return(ptr);
}

/*
 * Munmap - munmap() 래퍼
 *
 * Mmap()으로 만든 메모리 매핑을 해제합니다.
 */
void Munmap(void *start, size_t length)
{
    if (munmap(start, length) < 0)
	unix_error("munmap error");
}

/***************************************************
 * Wrappers for dynamic storage allocation functions
 * 동적 메모리 할당 함수 래퍼들
 ***************************************************/

/*
 * Malloc - malloc() 래퍼
 *
 * 힙(heap)에서 size 바이트를 할당합니다.
 * 실패 시(메모리 부족) 에러 출력 후 종료합니다.
 * 사용 후 반드시 Free()로 해제해야 합니다.
 */
void *Malloc(size_t size)
{
    void *p;

    if ((p  = malloc(size)) == NULL)
	unix_error("Malloc error");
    return p;
}

/*
 * Realloc - realloc() 래퍼
 *
 * 기존 할당된 메모리 블록의 크기를 size로 재조정합니다.
 * 크기를 늘릴 경우 기존 내용은 보존되고 새 공간은 초기화되지 않습니다.
 */
void *Realloc(void *ptr, size_t size)
{
    void *p;

    if ((p  = realloc(ptr, size)) == NULL)
	unix_error("Realloc error");
    return p;
}

/*
 * Calloc - calloc() 래퍼
 *
 * nmemb개의 size 바이트 항목을 할당하고 모두 0으로 초기화합니다.
 * malloc + memset(0)과 동일하지만 더 안전합니다.
 */
void *Calloc(size_t nmemb, size_t size)
{
    void *p;

    if ((p = calloc(nmemb, size)) == NULL)
	unix_error("Calloc error");
    return p;
}

/*
 * Free - free() 래퍼
 *
 * Malloc/Calloc/Realloc으로 할당한 메모리를 해제합니다.
 * 해제하지 않으면 메모리 누수(memory leak)가 발생합니다.
 */
void Free(void *ptr)
{
    free(ptr);
}

/******************************************
 * Wrappers for the Standard I/O functions.
 * 표준 I/O 함수 래퍼들 (FILE* 스트림 기반)
 *
 * [FILE*과 fd의 차이]
 *   fd(파일 디스크립터)는 커널 수준의 정수 핸들입니다.
 *   FILE*는 그 위에 C 라이브러리가 버퍼링을 추가한 스트림 객체입니다.
 ******************************************/

/*
 * Fclose - fclose() 래퍼
 *
 * FILE* 스트림을 닫고 버퍼를 플러시(flush)합니다.
 */
void Fclose(FILE *fp)
{
    if (fclose(fp) != 0)
	unix_error("Fclose error");
}

/*
 * Fdopen - fdopen() 래퍼
 *
 * 이미 열린 파일 디스크립터(fd)를 FILE* 스트림으로 변환합니다.
 *   type : "r"(읽기), "w"(쓰기), "a"(추가) 등
 */
FILE *Fdopen(int fd, const char *type)
{
    FILE *fp;

    if ((fp = fdopen(fd, type)) == NULL)
	unix_error("Fdopen error");

    return fp;
}

/*
 * Fgets - fgets() 래퍼
 *
 * 스트림에서 최대 n-1 바이트를 읽어 문자열로 저장합니다.
 * 개행('\n') 또는 EOF를 만나면 멈춥니다.
 * EOF이면 NULL 반환 (에러는 아님).
 */
char *Fgets(char *ptr, int n, FILE *stream)
{
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
	app_error("Fgets error");

    return rptr;
}

/*
 * Fopen - fopen() 래퍼
 *
 * 파일을 열어 FILE* 스트림을 반환합니다.
 *   mode : "r"(읽기), "w"(쓰기/생성), "a"(추가), "rb"(바이너리 읽기) 등
 */
FILE *Fopen(const char *filename, const char *mode)
{
    FILE *fp;

    if ((fp = fopen(filename, mode)) == NULL)
	unix_error("Fopen error");

    return fp;
}

/*
 * Fputs - fputs() 래퍼
 *
 * 문자열 ptr을 스트림에 씁니다 (개행 문자는 자동으로 추가되지 않음).
 */
void Fputs(const char *ptr, FILE *stream)
{
    if (fputs(ptr, stream) == EOF)
	unix_error("Fputs error");
}

/*
 * Fread - fread() 래퍼
 *
 * 스트림에서 nmemb개의 size 바이트 항목을 ptr에 읽습니다.
 * 실제 읽은 항목 수를 반환합니다.
 */
size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t n;

    if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream))
	unix_error("Fread error");
    return n;
}

/*
 * Fwrite - fwrite() 래퍼
 *
 * ptr의 내용 nmemb개(각 size 바이트)를 스트림에 씁니다.
 */
void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (fwrite(ptr, size, nmemb, stream) < nmemb)
	unix_error("Fwrite error");
}


/****************************
 * Sockets interface wrappers
 * 소켓 인터페이스 래퍼들
 *
 * [소켓이란?]
 *   네트워크 통신의 끝점(endpoint)입니다.
 *   전화기에 비유하면: 소켓=전화기, bind=전화번호 부여,
 *   listen=전화 대기, accept=전화 받기, connect=전화 걸기
 ****************************/

/*
 * Socket - socket() 래퍼
 *
 * 소켓을 생성하고 파일 디스크립터를 반환합니다.
 *   domain   : AF_INET(IPv4), AF_INET6(IPv6)
 *   type     : SOCK_STREAM(TCP, 연결 지향), SOCK_DGRAM(UDP, 비연결)
 *   protocol : 보통 0 (자동 선택)
 */
int Socket(int domain, int type, int protocol)
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
	unix_error("Socket error");
    return rc;
}

/*
 * Setsockopt - setsockopt() 래퍼
 *
 * 소켓의 옵션을 설정합니다.
 * 서버에서 자주 사용하는 옵션:
 *   SO_REUSEADDR : 이전 프로세스가 사용하던 포트를 바로 재사용 가능
 *                  ("Address already in use" 에러 방지)
 */
void Setsockopt(int s, int level, int optname, const void *optval, int optlen)
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
	unix_error("Setsockopt error");
}

/*
 * Bind - bind() 래퍼
 *
 * 소켓에 IP 주소와 포트 번호를 부여합니다(서버에서 사용).
 * 마치 전화기에 전화번호를 배정하는 것과 같습니다.
 */
void Bind(int sockfd, struct sockaddr *my_addr, int addrlen)
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
	unix_error("Bind error");
}

/*
 * Listen - listen() 래퍼
 *
 * 소켓을 수동 대기(listening) 상태로 전환합니다(서버에서 사용).
 *   backlog : 연결 대기 큐의 최대 크기 (접속 폭주 시 대기열)
 */
void Listen(int s, int backlog)
{
    int rc;

    if ((rc = listen(s,  backlog)) < 0)
	unix_error("Listen error");
}

/*
 * Accept - accept() 래퍼
 *
 * 클라이언트의 연결 요청을 수락합니다(서버에서 사용).
 * 클라이언트가 연결할 때까지 블로킹(blocking)됩니다.
 * 성공하면 클라이언트와 통신할 새 소켓 fd를 반환합니다.
 *   addr    : 연결된 클라이언트의 주소 정보가 채워짐
 *   addrlen : addr 구조체의 크기
 */
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
	unix_error("Accept error");
    return rc;
}

/*
 * Connect - connect() 래퍼
 *
 * 서버에 연결을 요청합니다(클라이언트에서 사용).
 * 3-way handshake가 완료될 때까지 블로킹됩니다.
 *   serv_addr : 접속할 서버의 IP 주소와 포트 번호
 */
void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
	unix_error("Connect error");
}

/*******************************
 * Protocol-independent wrappers
 * 프로토콜 독립적인 주소 변환 함수 래퍼들
 *
 * [IPv4 vs IPv6]
 *   이 함수들은 IPv4/IPv6 구분 없이 동작합니다.
 *   getaddrinfo()가 내부적으로 적절한 프로토콜을 선택합니다.
 *******************************/

/*
 * Getaddrinfo - getaddrinfo() 래퍼
 *
 * 호스트 이름(또는 IP 문자열)과 포트/서비스 이름을 받아
 * 사용 가능한 주소 정보 목록(linked list)을 반환합니다.
 *
 * 예: "www.google.com", "80" → IP 주소 구조체 리스트
 *
 *   node    : 호스트 이름 또는 IP 주소 문자열
 *   service : 포트 번호("80") 또는 서비스명("http")
 *   hints   : 원하는 소켓 타입 등 필터 힌트
 *   res     : 결과 주소 리스트 (사용 후 freeaddrinfo로 해제)
 */
/* $begin getaddrinfo */
void Getaddrinfo(const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;

    if ((rc = getaddrinfo(node, service, hints, res)) != 0)
        gai_error(rc, "Getaddrinfo error");
}
/* $end getaddrinfo */

/*
 * Getnameinfo - getnameinfo() 래퍼
 *
 * Getaddrinfo의 반대 방향: 소켓 주소 구조체를 호스트 이름과 서비스 이름으로 변환합니다.
 *   host    : 변환된 호스트 이름이 저장될 버퍼
 *   serv    : 변환된 서비스 이름(포트)이 저장될 버퍼
 */
void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
                 size_t hostlen, char *serv, size_t servlen, int flags)
{
    int rc;

    if ((rc = getnameinfo(sa, salen, host, hostlen, serv,
                          servlen, flags)) != 0)
        gai_error(rc, "Getnameinfo error");
}

/*
 * Freeaddrinfo - freeaddrinfo() 래퍼
 *
 * Getaddrinfo로 할당된 주소 리스트 메모리를 해제합니다.
 * 반드시 호출해야 메모리 누수를 방지할 수 있습니다.
 */
void Freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}

/*
 * Inet_ntop - inet_ntop() 래퍼
 *
 * 바이너리 형태의 IP 주소를 사람이 읽을 수 있는 문자열로 변환합니다.
 * 예: 바이너리 IPv4 → "192.168.1.1"
 *   af  : AF_INET(IPv4) 또는 AF_INET6(IPv6)
 *   dst : 변환된 문자열이 저장될 버퍼
 */
void Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (!inet_ntop(af, src, dst, size))
        unix_error("Inet_ntop error");
}

/*
 * Inet_pton - inet_pton() 래퍼
 *
 * 문자열 형태의 IP 주소를 바이너리로 변환합니다. (ntop의 반대)
 * 예: "192.168.1.1" → 바이너리 IPv4
 */
void Inet_pton(int af, const char *src, void *dst)
{
    int rc;

    rc = inet_pton(af, src, dst);
    if (rc == 0)
	app_error("inet_pton error: invalid dotted-decimal address");
    else if (rc < 0)
        unix_error("Inet_pton error");
}

/*******************************************
 * DNS interface wrappers. (구식 DNS 함수들)
 *
 * 주의: 아래 함수들은 스레드 안전하지 않아 사용을 권장하지 않습니다.
 *       getaddrinfo()와 getnameinfo()를 사용하세요.
 ***********************************/

/* $begin gethostbyname */
/*
 * Gethostbyname - gethostbyname() 래퍼 (구식, 비권장)
 *
 * 호스트 이름을 IP 주소로 변환합니다.
 * 내부 정적 버퍼를 사용해 스레드 안전하지 않습니다.
 */
struct hostent *Gethostbyname(const char *name)
{
    struct hostent *p;

    if ((p = gethostbyname(name)) == NULL)
	dns_error("Gethostbyname error");
    return p;
}
/* $end gethostbyname */

/*
 * Gethostbyaddr - gethostbyaddr() 래퍼 (구식, 비권장)
 *
 * IP 주소를 호스트 이름으로 역변환합니다.
 */
struct hostent *Gethostbyaddr(const char *addr, int len, int type)
{
    struct hostent *p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
	dns_error("Gethostbyaddr error");
    return p;
}

/************************************************
 * Wrappers for Pthreads thread control functions
 * POSIX 스레드(Pthreads) 제어 함수 래퍼들
 *
 * [스레드란?]
 *   같은 프로세스 안에서 메모리를 공유하며 독립적으로 실행되는 실행 단위입니다.
 *   프록시 서버에서 여러 클라이언트를 동시에 처리할 때 사용합니다.
 ************************************************/

/*
 * Pthread_create - pthread_create() 래퍼
 *
 * 새 스레드를 생성합니다.
 *   tidp    : 생성된 스레드의 ID가 저장될 포인터
 *   attrp   : 스레드 속성 (NULL이면 기본값)
 *   routine : 스레드가 실행할 함수
 *   argp    : routine에 전달할 인자
 */
void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp,
		    void * (*routine)(void *), void *argp)
{
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
	posix_error(rc, "Pthread_create error");
}

/*
 * Pthread_cancel - pthread_cancel() 래퍼
 *
 * 특정 스레드에 취소 요청을 보냅니다.
 * 스레드가 취소 가능 상태일 때 종료됩니다.
 */
void Pthread_cancel(pthread_t tid) {
    int rc;

    if ((rc = pthread_cancel(tid)) != 0)
	posix_error(rc, "Pthread_cancel error");
}

/*
 * Pthread_join - pthread_join() 래퍼
 *
 * 특정 스레드가 종료될 때까지 기다립니다.
 *   thread_return : 스레드의 반환값이 저장될 포인터
 */
void Pthread_join(pthread_t tid, void **thread_return) {
    int rc;

    if ((rc = pthread_join(tid, thread_return)) != 0)
	posix_error(rc, "Pthread_join error");
}

/*
 * Pthread_detach - pthread_detach() 래퍼
 *
 * 스레드를 분리(detach) 상태로 만듭니다.
 * 분리된 스레드는 종료 시 자동으로 자원을 해제합니다.
 * join이 필요 없는 스레드에 사용합니다.
 */
/* $begin detach */
void Pthread_detach(pthread_t tid) {
    int rc;

    if ((rc = pthread_detach(tid)) != 0)
	posix_error(rc, "Pthread_detach error");
}
/* $end detach */

/*
 * Pthread_exit - pthread_exit() 래퍼
 *
 * 현재 스레드를 종료합니다.
 * retval은 Pthread_join으로 회수할 수 있는 반환값입니다.
 */
void Pthread_exit(void *retval) {
    pthread_exit(retval);
}

/*
 * Pthread_self - pthread_self() 래퍼
 *
 * 현재 스레드의 ID(tid)를 반환합니다.
 */
pthread_t Pthread_self(void) {
    return pthread_self();
}

/*
 * Pthread_once - pthread_once() 래퍼
 *
 * init_function을 딱 한 번만 실행되도록 보장합니다.
 * 여러 스레드가 동시에 호출해도 init_function은 한 번만 실행됩니다.
 */
void Pthread_once(pthread_once_t *once_control, void (*init_function)()) {
    pthread_once(once_control, init_function);
}

/*******************************
 * Wrappers for Posix semaphores
 * POSIX 세마포어 래퍼들
 *
 * [세마포어란?]
 *   공유 자원에 대한 동시 접근을 제어하는 동기화 도구입니다.
 *   P(sem) : 세마포어 값을 1 감소. 0이면 대기(잠금).
 *   V(sem) : 세마포어 값을 1 증가. 대기 중인 스레드를 깨움(해제).
 *
 *   P와 V는 네덜란드어 Proberen(시도)과 Verhogen(증가)의 약자입니다.
 *******************************/

/*
 * Sem_init - sem_init() 래퍼
 *
 * 세마포어를 초기화합니다.
 *   pshared : 0이면 같은 프로세스 내 스레드 간 공유
 *   value   : 초기값 (1이면 뮤텍스처럼 사용 가능)
 */
void Sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (sem_init(sem, pshared, value) < 0)
	unix_error("Sem_init error");
}

/*
 * P - sem_wait() 래퍼 (세마포어 획득/잠금)
 *
 * 세마포어 값이 0이면 0보다 커질 때까지 대기합니다.
 * 0보다 크면 1 감소하고 진행합니다.
 * 임계 구역(critical section) 진입 시 사용합니다.
 */
void P(sem_t *sem)
{
    if (sem_wait(sem) < 0)
	unix_error("P error");
}

/*
 * V - sem_post() 래퍼 (세마포어 해제)
 *
 * 세마포어 값을 1 증가시키고, 대기 중인 스레드를 깨웁니다.
 * 임계 구역을 벗어날 때 사용합니다.
 */
void V(sem_t *sem)
{
    if (sem_post(sem) < 0)
	unix_error("V error");
}

/****************************************
 * The Rio package - Robust I/O functions
 * RIO 패키지 - 안정적(Robust) I/O 함수들
 *
 * [왜 RIO가 필요한가?]
 *   Unix의 read()/write()는 요청한 바이트보다 적게 읽거나 쓸 수 있습니다.
 *   이를 short count라고 합니다.
 *   예: read(fd, buf, 1000)이 실제로 500바이트만 읽을 수 있음
 *
 *   RIO는 요청한 바이트를 모두 읽거나 쓸 때까지 반복 호출합니다.
 *
 * [두 가지 읽기 방식]
 *   1. 언버퍼드(unbuffered): rio_readn, rio_writen
 *      - 내부 버퍼 없이 직접 커널에서 사용자 버퍼로 복사
 *      - 바이너리 파일, 네트워크 데이터 전송에 적합
 *
 *   2. 버퍼드(buffered): rio_readlineb, rio_readnb
 *      - 내부 버퍼(8192바이트)에서 데이터를 채워두고 조금씩 제공
 *      - 텍스트 한 줄씩 읽기에 적합 (HTTP 헤더 파싱 등)
 ****************************************/

/*
 * rio_readn - n 바이트를 안정적으로 읽습니다 (언버퍼드)
 *
 * 요청한 n 바이트를 모두 읽을 때까지 read()를 반복 호출합니다.
 * EINTR(시그널 인터럽트)로 중단된 경우 재시도합니다.
 * EOF이면 실제 읽은 바이트 수를 반환합니다.
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;       /* 아직 읽어야 할 바이트 수 */
    ssize_t nread;
    char *bufp = usrbuf;    /* 현재 버퍼 쓰기 위치 */

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* 시그널 핸들러가 중단시킨 경우 */
		nread = 0;      /* read()를 다시 호출 */
	    else
		return -1;      /* 실제 에러 */
	}
	else if (nread == 0)
	    break;              /* EOF 도달 */
	nleft -= nread;         /* 남은 바이트 수 갱신 */
	bufp += nread;          /* 버퍼 포인터 이동 */
    }
    return (n - nleft);         /* 실제 읽은 바이트 수 반환 */
}
/* $end rio_readn */

/*
 * rio_writen - n 바이트를 안정적으로 씁니다 (언버퍼드)
 *
 * 요청한 n 바이트를 모두 쓸 때까지 write()를 반복 호출합니다.
 * EINTR로 중단된 경우 재시도합니다.
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;       /* 아직 써야 할 바이트 수 */
    ssize_t nwritten;
    char *bufp = usrbuf;    /* 현재 버퍼 읽기 위치 */

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR) /* 시그널 핸들러가 중단시킨 경우 */
		nwritten = 0;   /* write()를 다시 호출 */
	    else
		return -1;      /* 실제 에러 */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/*
 * rio_read - rio_t 내부 버퍼에서 사용자 버퍼로 데이터를 복사합니다 (내부용)
 *
 * 내부 버퍼(rp->rio_buf)가 비어 있으면 read()로 다시 채웁니다.
 * min(n, 내부버퍼의남은데이터) 바이트를 usrbuf로 복사합니다.
 *
 * [rio_t 구조체 설명]
 *   rio_fd     : 읽을 파일 디스크립터
 *   rio_buf    : 내부 버퍼 (8192바이트)
 *   rio_bufptr : 내부 버퍼에서 다음에 읽을 위치
 *   rio_cnt    : 내부 버퍼에 남은 바이트 수
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* 내부 버퍼가 비었으면 다시 채움 */
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR) /* EINTR이 아닌 실제 에러 */
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF */
	    return 0;
	else
	    rp->rio_bufptr = rp->rio_buf; /* 버퍼 포인터를 처음으로 리셋 */
    }

    /* min(n, 남은 데이터) 바이트를 사용자 버퍼로 복사 */
    cnt = n;
    if (rp->rio_cnt < n)
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;   /* 내부 버퍼 읽기 위치 이동 */
    rp->rio_cnt -= cnt;      /* 남은 바이트 수 감소 */
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - rio_t 구조체를 초기화합니다
 *
 * 버퍼드 읽기를 사용하기 전에 반드시 호출해야 합니다.
 *   rp : 초기화할 rio_t 구조체 포인터
 *   fd : 읽을 파일 디스크립터 (소켓 등)
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;          /* 파일 디스크립터 저장 */
    rp->rio_cnt = 0;          /* 내부 버퍼를 비운 상태로 초기화 */
    rp->rio_bufptr = rp->rio_buf; /* 버퍼 포인터를 처음으로 */
}
/* $end rio_readinitb */

/*
 * rio_readnb - n 바이트를 안정적으로 읽습니다 (버퍼드)
 *
 * rio_readn과 동일하지만 내부 버퍼를 사용합니다.
 * rio_readinitb()로 초기화된 rio_t와 함께 사용합니다.
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = rio_read(rp, bufp, nleft)) < 0)
            return -1;          /* 에러 */
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* 실제 읽은 바이트 수 반환 */
}
/* $end rio_readnb */

/*
 * rio_readlineb - 텍스트 한 줄을 안정적으로 읽습니다 (버퍼드)
 *
 * '\n'(개행)을 만날 때까지 또는 maxlen-1 바이트까지 읽습니다.
 * 결과 문자열은 null-terminated('\0')입니다.
 * HTTP 요청/응답 헤더를 한 줄씩 파싱할 때 주로 사용합니다.
 *
 * 반환값:
 *   >0 : 읽은 바이트 수 (개행 포함)
 *    0 : EOF (데이터 없이)
 *   -1 : 에러
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {   /* 최대 maxlen-1 문자 읽기 */
        if ((rc = rio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;           /* 문자를 버퍼에 저장 */
	    if (c == '\n') {       /* 개행이면 줄 끝 */
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0;          /* EOF, 아무것도 읽지 못함 */
	    else
		break;             /* EOF, 일부 읽음 */
	} else
	    return -1;		   /* 에러 */
    }
    *bufp = 0;                     /* null terminator 추가 */
    return n-1;                    /* 읽은 바이트 수 반환 (null 제외) */
}
/* $end rio_readlineb */

/**********************************
 * Wrappers for robust I/O routines
 * RIO 함수들의 래퍼 (에러 처리 포함)
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
	unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
	unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
	unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	unix_error("Rio_readlineb error");
    return rc;
}

/********************************
 * Client/server helper functions
 * 클라이언트/서버 헬퍼 함수들
 *
 * [TCP 연결 과정 요약]
 *
 *   클라이언트                        서버
 *   ─────────                        ─────
 *   open_clientfd()                  open_listenfd()
 *      socket()                         socket()
 *      getaddrinfo()                    bind()
 *      connect()  ──── 연결 요청 ───►  listen()
 *                                       accept()  ◄── 연결 수락
 *   read()/write()  ◄──── 통신 ────►  read()/write()
 *   close()                          close()
 ********************************/

/*
 * open_clientfd - 서버에 연결하고 통신 가능한 소켓 fd를 반환합니다
 *
 * 클라이언트 측 함수입니다.
 * 내부적으로 socket() + getaddrinfo() + connect()를 수행합니다.
 * 프로토콜 독립적(IPv4/IPv6 모두 지원)이며 재진입 가능합니다.
 *
 *   hostname : 접속할 서버 주소 (예: "www.google.com", "127.0.0.1")
 *   port     : 접속할 포트 번호 문자열 (예: "80", "8080")
 *
 * 반환값:
 *   >0 : 연결된 소켓의 fd (읽기/쓰기 가능)
 *   -2 : getaddrinfo 실패
 *   -1 : 기타 에러
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, char *port) {
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    /* 서버의 가능한 주소 목록을 가져옴 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* TCP 연결 */
    hints.ai_flags = AI_NUMERICSERV;  /* 포트를 숫자로 해석 */
    hints.ai_flags |= AI_ADDRCONFIG;  /* 시스템이 지원하는 프로토콜만 반환 */
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }

    /* 주소 목록을 순회하며 연결 가능한 것을 찾음 */
    for (p = listp; p; p = p->ai_next) {
        /* 소켓 생성 시도 */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* 실패하면 다음 주소 시도 */

        /* 서버에 연결 시도 */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break; /* 성공! 루프 탈출 */
        if (close(clientfd) < 0) { /* 연결 실패 시 소켓 닫고 다음 시도 */
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    /* 메모리 해제 */
    freeaddrinfo(listp);
    if (!p) /* 모든 주소 연결 실패 */
        return -1;
    else    /* 연결 성공한 소켓 반환 */
        return clientfd;
}
/* $end open_clientfd */

/*
 * open_listenfd - 지정한 포트에서 연결을 기다리는 소켓 fd를 반환합니다
 *
 * 서버 측 함수입니다.
 * 내부적으로 socket() + bind() + listen()을 수행합니다.
 * 프로토콜 독립적이며 재진입 가능합니다.
 *
 *   port : 사용할 포트 번호 문자열 (예: "80", "8080")
 *
 * 반환값:
 *   >0 : 리스닝 소켓의 fd (클라이언트 연결 대기 중)
 *   -2 : getaddrinfo 실패
 *   -1 : 기타 에러
 */
/* $begin open_listenfd */
int open_listenfd(char *port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    /* 바인딩할 주소 목록을 가져옴 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* TCP */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* 모든 IP에서 수신 대기 */
    hints.ai_flags |= AI_NUMERICSERV;            /* 포트를 숫자로 해석 */
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    /* 바인딩 가능한 주소를 찾아 순회 */
    for (p = listp; p; p = p->ai_next) {
        /* 소켓 생성 시도 */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;  /* 실패하면 다음 주소 시도 */

        /* 서버 재시작 시 "Address already in use" 에러를 방지 */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int));

        /* 소켓에 IP 주소와 포트 바인딩 */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* 바인딩 성공 */
        if (close(listenfd) < 0) { /* 실패하면 닫고 다음 시도 */
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    /* 메모리 해제 */
    freeaddrinfo(listp);
    if (!p) /* 바인딩 가능한 주소 없음 */
        return -1;

    /* 소켓을 리스닝 상태로 전환 (클라이언트 연결 대기 시작) */
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
	return -1;
    }
    return listenfd;
}
/* $end open_listenfd */

/****************************************************
 * Wrappers for reentrant protocol-independent helpers
 * open_clientfd / open_listenfd 래퍼들 (에러 처리 포함)
 ****************************************************/

/*
 * Open_clientfd - open_clientfd()의 래퍼
 * 연결 실패 시 에러를 출력하고 종료합니다.
 */
int Open_clientfd(char *hostname, char *port)
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0)
	unix_error("Open_clientfd error");
    return rc;
}

/*
 * Open_listenfd - open_listenfd()의 래퍼
 * 리스닝 소켓 생성 실패 시 에러를 출력하고 종료합니다.
 */
int Open_listenfd(char *port)
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
	unix_error("Open_listenfd error");
    return rc;
}

/* $end csapp.c */
