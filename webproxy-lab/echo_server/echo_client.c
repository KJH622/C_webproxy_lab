#include <stdio.h> // printf, fprintf
#include <stdlib.h> // exit
#include <string.h> // strcpy, strlen
#include <unistd.h> // read, write, close
#include <sys/socket.h> // socket, bind, listen, accept
#include <netinet/in.h> // sockaddr_in, htons
#include <arpa/inet.h> // inet_pton, inet_ntop
#include <sys/types.h> // 데이터 타입들

#define MAXLINE 1024

int main(int argc, char **argv) { // argc : 인자 개수 argv : 인자 목록
    char buf[MAXLINE]; // 버퍼 변수 선언

    // 예외 조건 - 인자 개수는 3개 (어디로, ip 주소, 포트번호)
    if (argc != 3) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    // socket() - 소켓 생성
    int socket_client = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_client < 0) {
        fprintf(stderr, "socket error\n");
        return -1;
    }
    
    // 서버 주소 구조체
    struct sockaddr_in serveraddr;

    serveraddr.sin_family = AF_INET; // 주소 체계
    serveraddr.sin_port = htons(atoi(argv[2])); // 포트 문자열 -> 정수 -> 네트워크 바이트 순서로 변환

    // 문자열 -> 바이너리
    inet_pton(AF_INET, argv[1], &serveraddr.sin_addr); // inet_pton()은 반환값 x, 3번째 인자에 직접 저장

    // connect(소켓, 서버주소, 주소크기)
    int socket_connect = connect(socket_client, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

    // 예외 처리 - connect 실패할 경우
    if (socket_connect < 0) {
        fprintf(stderr, "connect error\n");
        exit(1);
    }

    // 보낼 내용 입력 - fgets(저장할곳, 최대길이, 입력소스)
    fgets(buf, MAXLINE, stdin);

    // 보내기 - write(소켓, 보낼데이터, 데이터크기)
    write(socket_client, buf, strlen(buf));

    // 받기 - read(소켓, 저장할곳, 최대크기)
    read(socket_client, buf, MAXLINE);

    // 출력 - fprintf(출력대상, 형식, 값)
    fprintf(stdout, "%s\n", buf);

    // 소켓 닫는 함수 - close(소켓)
    close(socket_client);

    // 정상 종료
    exit(0);
}