#include <sys/time.h>
#include <time.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>

typedef struct Message {
    char* content;
    struct Message* next;
}Message;

void run(int port)
{
int s1_cproxy = 0, s2_daemon = 0, n = 0, rv = 0, sessionID = -1;
fd_set readfds;
struct timeval tv;
char cmd_buf[9999], reply_buf[9999];
struct sockaddr_in daemon_address;

int server_teldaemon = socket(AF_INET, SOCK_STREAM, 0);
memset(&daemon_address, '0', sizeof(daemon_address));
daemon_address.sin_family = AF_INET;
daemon_address.sin_port = htons(23);
daemon_address.sin_addr.s_addr = inet_addr("127.0.0.1");

connect(server_teldaemon, (struct sockaddr *) &daemon_address, sizeof(daemon_address));

int seqID = -1;
int ackID = -1;
Message* queue = (Message*)malloc(sizeof(Message));
queue->next = NULL;

while (1) {
struct sockaddr_in address;
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(port);

int opt = 1;
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
}

bind(server_fd, (struct sockaddr *) &address, sizeof(address));
listen(server_fd,5);
int addrlen = sizeof(address);
s1_cproxy = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) & addrlen);

s2_daemon = server_teldaemon;
printf("- Accepted\n");
int len1 = 0, len2 = 0;

struct timeval last, now, hb_time;
gettimeofday(&last, NULL);
int hb_sent = 0;
int hb_recv = 0;
int last_hb = -1;

while (1) {
FD_ZERO(&readfds);
FD_SET(s1_cproxy, &readfds);
FD_SET(s2_daemon, &readfds);
if (s1_cproxy > s2_daemon)
    n = s1_cproxy + 1;
else
    n = s2_daemon + 1;
    
tv.tv_sec = 1;      //10;
tv.tv_usec = 5;     //500000;
rv = select(n, &readfds, NULL, NULL, &tv);

gettimeofday(&now, NULL);
double diff = (now.tv_sec - last.tv_sec) +
((now.tv_usec - last.tv_usec) / 1000000.0);
if (diff >= 1) {
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
send(s1_cproxy, Message, Message_len_HB, 0);
gettimeofday(&last, NULL);
hb_sent++;
}
    
if (last_hb >= 0) {
gettimeofday(&now, NULL);
double diff_hb =
(now.tv_sec - hb_time.tv_sec) +
((now.tv_usec - hb_time.tv_usec) / 1000000.0);
if (diff_hb >= 3  ) {
close(s1_cproxy);
listen(server_fd, 5);

    int addrlen = sizeof(address);
    s1_cproxy = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) & addrlen);
}
}

if (rv == -1) {
    perror("select");
} else if (rv == 0) {
;
} else {
if (FD_ISSET(s1_cproxy, &readfds)) {
len1 = recv(s1_cproxy, cmd_buf, sizeof(cmd_buf), 0);
if (len1 > 0) {
char *buffer = cmd_buf;
int pkt_len = len1;
int go_again = 0;
do {
int type = -1;
int ackID_C = -1;
int sessionID_C = -1;
char payload_c[9999] = { 0 };
type = buffer[0] << 24 | (buffer[1] & 0xFF) << 16 | (buffer[2] & 0xFF) << 8 | (buffer[3] & 0xFF);
ackID_C = buffer[4] << 24 | (buffer[5] & 0xFF) << 16 | (buffer[6] & 0xFF) << 8 | (buffer[7] & 0xFF);
sessionID_C = buffer[8] << 24 | (buffer[9] & 0xFF) << 16 | (buffer[10] & 0xFF) << 8 | (buffer[11] & 0xFF);
int paylen_c = buffer[12] << 24 | (buffer[13] & 0xFF) << 16 | (buffer[14] & 0xFF) << 8 | (buffer[15] & 0xFF);
memcpy(payload_c, &buffer[20], paylen_c);
if (sessionID != sessionID_C) {
sessionID = sessionID_C;
}
    
if (type == 2) {
gettimeofday(&hb_time, NULL);
hb_recv++;
last_hb++;
} else if (type == 1) {
if(seqID >= ackID_C){
;
}
else{ // <
Message *cur = queue->next;
               if (cur != NULL)
                       queue->next = queue->next->next;
               }
               ackID++;
               send(s2_daemon, payload_c, paylen_c, 0);

           } else {
               ;
           }
           int hdr_and_pay = 20 + paylen_c;
           if (hdr_and_pay < pkt_len) {
               go_again = 1;
               
               pkt_len = pkt_len - hdr_and_pay;
               
               buffer += hdr_and_pay;
               
           } else {
               go_again = 0;
           }

       } while (go_again);

       memset(cmd_buf, 0, sizeof(cmd_buf));
   } else {
       close(s1_cproxy);
       close(s2_daemon);
       close(server_fd);
       return;
   }
}
if (FD_ISSET(s2_daemon, &readfds)) {
   len2 = recv(s2_daemon, reply_buf, sizeof(reply_buf), 0);
   if (len2 > 0) {
       seqID++;
       Message *node = NULL;
       char Message_d[9999] = { 0 };
        Message_d[0] = (((long) 1) >> 24) & 0xFF;
        Message_d[1] = (((long) 1) >> 16) & 0xFF;
        Message_d[2] = (((long) 1) >> 8) & 0xFF;
        Message_d[3] = ((long) 1) & 0xFF;
       //AckID
       Message_d[4] = (((long) ackID) >> 24) & 0xFF;
       Message_d[5] = (((long) ackID) >> 16) & 0xFF;
       Message_d[6] = (((long) ackID) >> 8) & 0xFF;
       Message_d[7] = ((long) ackID) & 0xFF;
       //sessionID
       Message_d[8] = (((long) sessionID) >> 24) & 0xFF;
       Message_d[9] = (((long) sessionID) >> 16) & 0xFF;
       Message_d[10] = (((long) sessionID) >> 8) & 0xFF;
       Message_d[11] = ((long) sessionID) & 0xFF;
       //Paylen
       Message_d[12] = (((long) len2) >> 24) & 0xFF;
       Message_d[13] = (((long) len2) >> 16) & 0xFF;
       Message_d[14] = (((long) len2) >> 8) & 0xFF;
       Message_d[15] = ((long) len2) & 0xFF;
       //SeqID
       Message_d[16] = (((long) reply_buf) >> 24) & 0xFF;
       Message_d[17] = (((long) reply_buf) >> 16) & 0xFF;
       Message_d[18] = (((long) reply_buf) >> 8) & 0xFF;
       Message_d[19] = ((long) reply_buf) & 0xFF;
       memcpy(&Message_d[20], reply_buf, sizeof(reply_buf));
       int Message_len_d = 20 + len2;
       node = (Message*)malloc(sizeof(Message));
       node->content = (char*)malloc(sizeof(char) * Message_len_d);
       strncpy(node->content, Message_d, Message_len_d);
       node->next = NULL;
       Message *curr = queue;
       while (curr->next != NULL)
               curr = curr->next;
       curr->next = node;
       send(s1_cproxy, Message_d, Message_len_d, 0);
       memset(reply_buf, 0, sizeof(reply_buf));
   } else {
       usleep(50000);
   }
}
          }
      }
      close(s1_cproxy);
  }
  close(s2_daemon);
  close(server_teldaemon);
  return;
}
int main(int argc, char const *argv[])
{
      int port = atoi(argv[1]);
      while (1) {
          run(port);
          sleep(1);
      }

}

