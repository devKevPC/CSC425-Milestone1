#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
//#include <winsock2.h>

int main(int argc, char* argv[]){
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[1024];

    if (argc < 3){
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        fprintf(stderr, "ERROR, bad socket\n");
        exit(1);
    }

    server = gethostbyname(argv[1]);
    if (server == NULL){
        fprintf(stderr, "ERROR, bad server\n");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(&serv_addr));

    serv_addr.sin_family = AF_INET;

    bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, &serv_addr, sizeof(serv_addr))<0){
        fprintf(stderr, "ERROR, bad connection\n");
        exit(1);
    }

    int len = 0;
    while(fgets(buffer, 1023, stdin)) {
        buffer[1024-1] = '\0';
        len = strlen(buffer) + 1;
        send(sockfd, buffer, len, 0);
    }

return 0;
}