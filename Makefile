all: client server

client:   client.c
	gcc -g -Wall client.c -o client

server:   server.c
	gcc -g -Wall server.c -o server
