#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char* argv[]) {
////////////////////////////////////////////////////////
//      Send/Recieve (Cproxy <-> Sproxy) Socket       //
////////////////////////////////////////////////////////

    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
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

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

////////////////////////////////////////////////////////
//   Send/Recieve (Sproxy <-> Telnet Daemon) Socket    //
////////////////////////////////////////////////////////

    struct sockaddr_in daemon_address;
    int d_sock = socket(AF_INET, SOCK_STREAM, 0);

    daemon_address.sin_family=AF_INET;
    daemon_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    daemon_address.sin_port=htons(23);

    if (connect(d_sock, (struct sockaddr *) &daemon_address, sizeof(serv_addr))<0){
        perror("Connection Failed");
        exit(0);
    }

////////////////////////////////////////////////////////
//              Select where data will go             //
////////////////////////////////////////////////////////
    fd_set readfds;
    int sock1_telnet = d_sock;
    int sock2_sproxy = newsockfd;

    FD_ZERO(&readfds);
    FD_SET(sock1_telnet, &readfds);
    FD_SET(sock2_sproxy, &readfds);
    int n;
    if (sock1_telnet > sock2_sproxy)
        n = sock1_telnet + 1;
    else
        n = sock2_sproxy + 1;

    struct timeval tv;
    char cmd_buffer[1024], reply_buffer[1024];
    int len1, len2, rv;
    tv.tv_sec = 10; 
    tv.tv_usec = 500000;
    rv = select(n, &readfds, NULL, NULL, &tv);

    if (rv == -1) {
        perror("select");
    } else if (rv == 0) {
        printf("Timeout occurred!  No data after 10.5 seconds.\n");
    } else {
        if (FD_ISSET(sock1_telnet, &readfds)) {
            fputs("Sent to proxy\n", stdout);
            len1 = recv(sock1_telnet, cmd_buffer, 1024, 0);
            send(sock2_sproxy, cmd_buffer, 1024, 0);
        }
        if (FD_ISSET(sock2_sproxy, &readfds)) {
            fputs("Sent to telnet\n", stdout);
            len2 = recv(sock2_sproxy, reply_buffer, 1024, 0);
            send(sock1_telnet, reply_buffer, 1024, 0);
        }
    }
    close(sockfd);
    close(d_sock);
return 0;
}