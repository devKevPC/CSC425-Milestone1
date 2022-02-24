all: client server

client:   client.c
	gcc -g -Wall -std=c99 client.c -o client

server:   server.c
	gcc -g -Wall -std=c99 server.c -o server