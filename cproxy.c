#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char* argv[]){
    //structs to recieve from telnet
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr, serv_addrProxy;
    // structs to send from here to client
    int cliSockfd, cliPortno;
    struct hostent *server;
    char buffer[1024];
    
    if (argc < 4){
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
////////////////////////////////////////////////////////
//           Send/Recieve (Telnet <-> Cproxy) Socket        //
////////////////////////////////////////////////////////

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0){fprintf(stderr, "ERROR, bad socket\n");exit(1);}
    
    bzero((char *) &serv_addr,sizeof(serv_addr));

    portno = atoi(argv[1]);
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=INADDR_ANY;
    serv_addr.sin_port=htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr))<0){error("binding");}
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    size_t len;
    int buf_len;

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

  ////////////////////////////////////////////////////////
 //              Send/Recieve (Cproxy <-> Sproxy) Socket        //
  ////////////////////////////////////////////////////////

    cliPortno = atoi(argv[3]);

    cliSockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (cliSockfd < 0){fprintf(stderr, "ERROR, bad socket\n");exit(1);}

    server = gethostbyname(argv[2]);
    if (server == NULL){fprintf(stderr, "ERROR, bad server\n");exit(1);}

    bzero((char *) &serv_addrProxy, sizeof(&serv_addrProxy));
    serv_addrProxy.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *)&serv_addrProxy.sin_addr.s_addr, server->h_length);
    serv_addrProxy.sin_port = htons(cliPortno);

    if (connect(cliSockfd, &serv_addrProxy, sizeof(serv_addrProxy))<0){fprintf(stderr, "ERROR, bad connection\n");exit(1);}
    fd_set readfds;

////////////////////////////////////////////////////////
//              Select where data will go             //
////////////////////////////////////////////////////////
    int sock1_telnet = newsockfd;
    int sock2_sproxy = cliSockfd;

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
            fputs("Sent to proxy", stdout);
            len1 = recv(sock1_telnet, cmd_buffer, 1024, 0);
            send(sock2_sproxy, cmd_buffer, 1024, 0);
        }
        if (FD_ISSET(sock2_sproxy, &readfds)) {
            fputs("Sent to telnet", stdout);
            len2 = recv(sock2_sproxy, reply_buffer, 1024, 0);
            send(sock1_telnet, reply_buffer, 1024, 0);
        }
    }
    close(newsockfd);
return 0;
}