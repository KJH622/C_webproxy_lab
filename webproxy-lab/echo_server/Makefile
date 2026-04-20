CC = gcc
CFLAGS = -g -Wall

all: echo_server echo_client

echo_server: echo_server.c
	$(CC) $(CFLAGS) echo_server.c -o echo_server

echo_client: echo_client.c
	$(CC) $(CFLAGS) echo_client.c -o echo_client

clean:
	rm -f echo_server echo_client
