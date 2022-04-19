#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <sys/select.h>
#include <stdbool.h>

typedef struct Message {
    char* content;
    struct Message* next;
}Message;

void run(char const *ip, int sPort, int tPort)
{

    int n = 0, rv = 0;
    fd_set readfds;
    struct timeval tv;
    char cmd_buf[9999], reply_buf[9999];

    struct sockaddr_in t_address;
    
    int sock_telnet = socket(AF_INET, SOCK_STREAM, 0);
    memset(&t_address, '0', sizeof(t_address));
    t_address.sin_family = AF_INET;
    t_address.sin_port = htons(tPort);
    t_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(sock_telnet, (struct sockaddr *) &t_address, sizeof(t_address));
    listen(sock_telnet,5);
    
    int addr_len = sizeof(t_address);
    int s1_telnet = accept(sock_telnet, (struct sockaddr*) &t_address, (socklen_t *) & addr_len);
    int sessionID = rand();
    int seqID = -1;
    Message *queue = (Message*)malloc(sizeof(Message));
    queue->next = NULL;
    int ackID = -1;

    int len1 = 0, len2 = 0;
    while (1) {
        struct sockaddr_in sAddress;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&sAddress, '0', sizeof(sAddress));
        sAddress.sin_family = AF_INET;
        sAddress.sin_port = htons(sPort);
        sAddress.sin_addr.s_addr = inet_addr(ip);

        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        connect(sock, (struct sockaddr *) &sAddress, sizeof(sAddress));
        int s2_sproxy = sock;

        struct timeval last, now, hb_time;
        gettimeofday(&last, NULL);
        int hb_sent = 0;
        int hb_recv = 0;
        int last_hb = -1;
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(s1_telnet, &readfds);
            FD_SET(s2_sproxy, &readfds);

            if (s1_telnet > s2_sproxy)
                n = s1_telnet + 1;
            else
                n = s2_sproxy + 1;

            tv.tv_sec = 1;
            tv.tv_usec = 500000;
            rv = select(n, &readfds, NULL, NULL, &tv);

            gettimeofday(&now, NULL);
            double diff = (now.tv_sec - last.tv_sec) + ((now.tv_usec - last.tv_usec) / 1000000.0);
            if (diff >= 1) {
                //send
                char Message[9999] = { 0 };
                char *payload_HB = " ";
                Message[0] = (((long) 2) >> 24) & 0xFF;
                Message[1] = (((long) 2) >> 16) & 0xFF;
                Message[2] = (((long) 2) >> 8) & 0xFF;
                Message[3] = ((long) 2) & 0xFF;
                //AckID
                Message[4] = (((long) hb_sent) >> 24) & 0xFF;
                Message[5] = (((long) hb_sent) >> 16) & 0xFF;
                Message[6] = (((long) hb_sent) >> 8) & 0xFF;
                Message[7] = ((long) hb_sent) & 0xFF;
                //sessionID
                Message[8] = (((long) sessionID) >> 24) & 0xFF;
                Message[9] = (((long) sessionID) >> 16) & 0xFF;
                Message[10] = (((long) sessionID) >> 8) & 0xFF;
                Message[11] = ((long) sessionID) & 0xFF;
                //Paylen
                Message[12] = (((long) sizeof(payload_HB)) >> 24) & 0xFF;
                Message[13] = (((long) sizeof(payload_HB)) >> 16) & 0xFF;
                Message[14] = (((long) sizeof(payload_HB)) >> 8) & 0xFF;
                Message[15] = ((long) sizeof(payload_HB)) & 0xFF;
                //SeqID
                Message[16] = (((long) 2020) >> 24) & 0xFF;
                Message[17] = (((long) 2020) >> 16) & 0xFF;
                Message[18] = (((long) 2020) >> 8) & 0xFF;
                Message[19] = ((long) 2020) & 0xFF;

                memcpy(&Message[20], payload_HB, sizeof(*payload_HB));
                int Message_len_HB = 20 + sizeof(payload_HB);
                send(s2_sproxy, Message, Message_len_HB, 0);
                gettimeofday(&last, NULL);
                hb_sent++;
            }
            // Detect if timeout by heartbeat
            if (last_hb >= 0) {
                gettimeofday(&now, NULL);
                double diff_hb = (now.tv_sec - hb_time.tv_sec) + ((now.tv_usec - hb_time.tv_usec) / 1000000.0);
                if (diff_hb >= 3) {
                    close(s2_sproxy);

                    struct sockaddr_in n_address;
                    s2_sproxy = socket(AF_INET, SOCK_STREAM, 0);
                    n_address.sin_family = AF_INET;
                    n_address.sin_addr.s_addr = INADDR_ANY;
                    n_address.sin_port = htons(sPort);
                    int try = 1;
                    do {
                        try = connect(s2_sproxy, (struct sockaddr *) &n_address, sizeof(n_address));

                    } while (try == 0);
                        gettimeofday(&hb_time, NULL);
                    }
            }
                if (rv == -1) {
                    perror("select");
                } else if (rv == 0) {
                    ;
                } else {
        if (FD_ISSET(s1_telnet, &readfds)) {
            len1 = recv(s1_telnet, cmd_buf, sizeof(cmd_buf), 0);
            if (len1 > 0) {
                seqID++;
                Message *node = NULL;
                char Message_t[9999] = { 0 };
                Message_t[0] = (((long) 1) >> 24) & 0xFF;
                Message_t[1] = (((long) 1) >> 16) & 0xFF;
                Message_t[2] = (((long) 1) >> 8) & 0xFF;
                Message_t[3] = ((long) 1) & 0xFF;
                //AckID
                Message_t[4] = (((long) ackID) >> 24) & 0xFF;
                Message_t[5] = (((long) ackID) >> 16) & 0xFF;
                Message_t[6] = (((long) ackID) >> 8) & 0xFF;
                Message_t[7] = ((long) ackID) & 0xFF;
                //sessionID
                Message_t[8] = (((long) sessionID) >> 24) & 0xFF;
                Message_t[9] = (((long) sessionID) >> 16) & 0xFF;
                Message_t[10] = (((long) sessionID) >> 8) & 0xFF;
                Message_t[11] = ((long) sessionID) & 0xFF;
                //Paylen
                Message_t[12] = (((long) len1) >> 24) & 0xFF;
                Message_t[13] = (((long) len1) >> 16) & 0xFF;
                Message_t[14] = (((long) len1) >> 8) & 0xFF;
                Message_t[15] = ((long) len1) & 0xFF;
                //SeqID
                Message_t[16] = (((long) cmd_buf) >> 24) & 0xFF;
                Message_t[17] = (((long) cmd_buf) >> 16) & 0xFF;
                Message_t[18] = (((long) cmd_buf) >> 8) & 0xFF;
                Message_t[19] = ((long) cmd_buf) & 0xFF;
                memcpy(&Message_t[20], cmd_buf, sizeof(cmd_buf));
                int Message_len_t = 20 + len1;
                node = (Message*)malloc(sizeof(Message));
                node->content = (char*)malloc(sizeof(char) * Message_len_t);
                strncpy(node->content, Message_t, Message_len_t);
                node->next = NULL;
                Message *cur = queue;
                while (cur->next != NULL)
                    cur = cur->next;
                    cur->next = node;
                    send(s2_sproxy, Message_t, Message_len_t, 0);
                    memset(cmd_buf, 0, sizeof(cmd_buf));
                } else {    // len < 1
                    close(s1_telnet);
                    close(s2_sproxy);
                    close(sock_telnet);
                return;
                }
            }
        if (FD_ISSET(s2_sproxy, &readfds)) {
            len2 = recv(s2_sproxy, reply_buf, sizeof(reply_buf), 0);
            if (len2 > 0) {

            char *buffer = reply_buf;
            int pkt_len = len2;
            int go_again = 0;
            
            do {
                int type = -1;
                int ackID_S = -1;
                char payload_s[9999] = { 0 };
                
                type = buffer[0] << 24 | (buffer[1] & 0xFF) << 16 | (buffer[2] & 0xFF) << 8 | (buffer[3] & 0xFF);
                ackID_S = buffer[4] << 24 | (buffer[5] & 0xFF) << 16 | (buffer[6] & 0xFF) << 8 | (buffer[7] & 0xFF);
                int paylen_s = buffer[12] << 24 | (buffer[13] & 0xFF) << 16 | (buffer[14] & 0xFF) << 8 | (buffer[15] & 0xFF);
                memcpy(payload_s, &buffer[20], paylen_s);
            if (type == 2) {
                gettimeofday(&hb_time, NULL);
                hb_recv++;
                last_hb++;
            }
        
            else if (type == 1) {
                if(seqID >= ackID_S){
                    ;
                }
            else{
                Message *curr = queue->next;
                if (curr != NULL)
                   queue->next = queue->next->next;

            }

                ackID++;

            send(s1_telnet, payload_s, paylen_s, 0);
            } else {
                ;
            }
            int hdr_and_pay = 20 + paylen_s;
            if (hdr_and_pay < pkt_len) {
                go_again = 1;
                pkt_len = pkt_len - hdr_and_pay;
                buffer += hdr_and_pay;
            } else {
                go_again = 0;
            }


        } while (go_again);

            memset(reply_buf, 0, sizeof(reply_buf));
            } else {
                close(s2_sproxy);
                s2_sproxy = socket(AF_INET, SOCK_STREAM, 0);
                memset(&sAddress, '0', sizeof(sAddress));
                sAddress.sin_family = AF_INET;
                sAddress.sin_port = htons(sPort);
                sAddress.sin_addr.s_addr = inet_addr(ip);
                connect(sock, (struct sockaddr *) &sAddress, sizeof(sAddress));
            }
        }
    }
    }
        close(sock);
        close(s2_sproxy);
    }
    close(s1_telnet);
    close(sock_telnet);
    return;
    }

int main(int argc, char const *argv[]) {
    int tPort = atoi(argv[1]);
    char const *ip = argv[2];
    int sPort = atoi(argv[3]);
    

    while (1) {
        run(ip, sPort, tPort);
        sleep(1);
    }
}
