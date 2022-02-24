#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <winsock2.h>

int main(int argc, char* argv[]){
    int sockfd, newsockfd, portno, clilen,n;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[1024];
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "ERROR, bad socket\n");
        exit(1);
    }
    bzero((char *) &serv_addr,sizeof(serv_addr));

    portno = atoi(argv[1]);

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=INADDR_ANY;
    serv_addr.sin_port=htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr))<0)
    {
        error("binding");
    }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    size_t len;

    int buf_len;

    while(1){
        if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0) {
            return 0;
        }
        while(buf_len = recv(newsockfd, buffer, 1024, 0)){
            len = strlen(buffer);
            printf("%zu\n", len-1);
            fputs(buffer, stdout);
        }
        close(newsockfd);
        return 0;
    }

return 0;
}