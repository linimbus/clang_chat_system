#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int send_message(int sock, MESSAGE *msg) {
  if (sizeof(MESSAGE) != send(sock, (void *)msg, sizeof(MESSAGE), 0)) {
    printf("send message error\n");
    return -1;
  }
  return 0;
}

int recv_message(int sock, MESSAGE *msg) {
  size_t length = recv(sock, msg, sizeof(MESSAGE), 0);
  if (length != sizeof(MESSAGE)) {
    return -1;
  }
  return 0;
}

void clear_str(char *s) {
  for (; *s != '\0'; s++) {
    if (*s == '\r' || *s == '\n') {
      *s = '\0';
    }
  }
}