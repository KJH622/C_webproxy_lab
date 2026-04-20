#include <stdio.h> // printf, fprintf
#include <stdlib.h> // exit
#include <string.h> // strcpy, strlen
#include <unistd.h> // read, write, close
#include <sys/socket.h> // socket, bind, listen, accept
#include <netinet/in.h> // sockaddr_in, htons
#include <arpa/inet.h> // inet_pton, inet_ntop

#define MAXLINE 1024

int main(int argc, char **argv) {// argc : 인자 개수 argv : 인자 목록

    // 1. 인자 확인
    if (argc != 2) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    // 2. 소켓 생성 - 생성이 안되면 return -1
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    // 3. 서버 주소 설정
    struct sockaddr_in serveraddr;

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[1]));
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    // 4. bind - bind(소켓fd, 주소구조체포인터, 구조체크기)
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    // 5. listen - listen(소켓fd, backlog)
    if (listen(listenfd, 10) < 0) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    // 6. 반복해서 accept
    struct sockaddr_in clientaddr;

    socklen_t clientlen = sizeof(clientaddr);

    int connfd;
    while ((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen)) >= 0) {

        // 7. read
        char buf[MAXLINE];

        int n = read(connfd, buf, MAXLINE);

        // 8. write
        write(connfd, buf, n);

        // 9. close
        close(connfd);
    }
}