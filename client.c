#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

// Create a TCP client socket and connect to the server:port
int init_client(const char *addr, int port) {
  struct sockaddr_in addr_in = {0};
  int sock = -1;
  // Create an IPv4 TCP socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("create socket error");
    goto fail;
  }
  // Convert string address to network byte order
  if (inet_pton(AF_INET, addr, &(addr_in.sin_addr)) <= 0) {
    perror("inet pton error");
    goto fail;
  }
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons(port);
  // Connect to the server
  int ret =
      connect(sock, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));
  if (ret < 0) {
    perror("connect error");
    goto fail;
  }
  return sock;

fail: // If failed, release socket resources
  if (sock > 0) {
    close(sock);
  }
  return -1;
}

// Attempt to log in to the server using username and password
int login(int sock, char *username, char *password) {
  MESSAGE msg;
  memset(&msg, 0, sizeof(MESSAGE));
  // Construct login message
  msg.type = LOGIN_MSG;
  strcpy(msg.data.login.username, username);
  strcpy(msg.data.login.password, password);
  if (send_message(sock, &msg) != 0) {
    return -1;
  }
  // Receive server response
  if (recv_message(sock, &msg) != 0) {
    return -1;
  }
  // Check if the response message indicates success
  if (msg.type == ACK_MSG && msg.data.ack.error == 0) {
    printf("login %s successful\n", username);
    return 0;
  }
  // Login failed, print error message
  printf("login fail, %s\n", msg.data.ack.info);
  return -1;
}

// Message receiving thread
void *recv_task(void *arg) {
  int sock = *(int *)arg;
  MESSAGE msg;

  while (1) {
    memset(&msg, 0, sizeof(MESSAGE));
    if (recv_message(sock, &msg) < 0) {
      break;
    }
    // Handle messages based on their type
    switch (msg.type) {
    case GROUP_MSG: {
      // Received a group message, print the content
      GROUP_MESSAGE *gm = &msg.data.group;
      printf("[RECV GROUP MSG FROM %s : %s]\n", gm->from_user, gm->message);
    } break;
    case SINGLE_MSG: {
      // Received a single message, print the content
      SINGLE_MESSAGE *sm = &msg.data.single;
      printf("[RECV SINGLE MSG FROM %s : %s]\n", sm->from_user, sm->message);
    } break;
    case ACK_MSG: {
      // Other messages, such as status queries
      ACK_MESSAGE *am = &msg.data.ack;
      if (am->error) {
        printf("[ACK ERR: %d INFO: %s]\n", am->error, am->info);
      } else if (strlen(am->info)) {
        printf("[ACK INFO: %s]\n", am->info);
      }
    } break;
    default:
      break;
    }
  }
  return NULL;
}

// Print help information, supported command-line options
void help_usage() {
  printf("-> ---------------HELP---------------\n");
  printf("-> [?,h,help] show usage\n");
  printf("-> [g,group] send message to all user\n");
  printf("-> [s,single] send message to user\n");
  printf("-> [q,query] query all user status\n");
  printf("-> [e,exit] logout\n");
  printf("-> ----------------------------------\n");
}

// Check if a string matches any in a list
int is_mapping(const char *p, const char **ss) {
  for (size_t i = 0; strlen(ss[i]); i++) {
    if (strcmp(p, ss[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

// Check if the command is a help command
int is_help(const char *p) {
  const char *ss[] = {"?", "h", "help", ""};
  return is_mapping(p, ss);
}

// Check if the command is an exit command
int is_exit(const char *p) {
  const char *ss[] = {"e", "exit", ""};
  return is_mapping(p, ss);
}

// Check if the command is a query command
int is_query(const char *p) {
  const char *ss[] = {"q", "query", ""};
  return is_mapping(p, ss);
}

// Check if the command is a group message command
int is_group(const char *p) {
  const char *ss[] = {"g", "group", ""};
  return is_mapping(p, ss);
}

// Check if the command is a single message command
int is_single(const char *p) {
  const char *ss[] = {"s", "single", ""};
  return is_mapping(p, ss);
}

// Read input string from the user
void get_input(char *out, size_t len) {
  printf("-> ");
  memset(out, 0, len);
  fgets(out, len, stdin);
  clear_str(out);
}

int main(int argc, char **argv) {
  int sock;
  pthread_t tid;
  if (argc != 3) {
    printf("Usage: username password\n");
    return -1;
  }
  sock = init_client(SERVER_IP, SERVER_PORT);
  if (sock < 0) {
    return -1;
  }
  printf("init client successful\n");

  // Input parameters include username and password
  if (login(sock, argv[1], argv[2])) {
    close(sock);
    return -1;
  }

  // Create a message receiving task to avoid blocking message reception during stdin reading
  if (pthread_create(&tid, NULL, recv_task, &sock) != 0) {
    perror("create task fail\n");
    close(sock);
    return -1;
  }

  // Print command-line help information
  help_usage();

  while (1) {
    MESSAGE msg;
    memset(&msg, 0, sizeof(MESSAGE));

    // Read command input
    char input[MSG_LEN];
    get_input(input, MSG_LEN);
    if (strlen(input) == 0) {
      continue;
    }

    if (is_help(input)) {
      help_usage();
      continue;
    }

    if (is_exit(input)) {
      // Send logout message
      msg.type = LOGOUT_MSG;
      send_message(sock, &msg);
      break;
    }

    if (is_query(input)) {
      // Send status query message
      msg.type = QUERY_MSG;
      send_message(sock, &msg);
      continue;
    }

    if (is_group(input)) {
      // Read the message to be sent
      printf("[INPUT MESSAGE]\n");
      char message[MSG_LEN];
      get_input(message, MSG_LEN);
      // Send group message
      msg.type = GROUP_MSG;
      strcpy(msg.data.group.from_user, argv[1]);
      strcpy(msg.data.group.message, message);
      send_message(sock, &msg);
      continue;
    }

    if (is_single(input)) {
      char username[MSG_LEN];
      char message[MSG_LEN];
      // Read username and message separately, then send the message
      printf("[INPUT USRNAME]\n");
      get_input(username, MSG_LEN);
      printf("[INPUT MESSAGE]\n");
      get_input(message, MSG_LEN);
      msg.type = SINGLE_MSG;
      strcpy(msg.data.single.from_user, argv[1]);
      strcpy(msg.data.single.to_user, username);
      strcpy(msg.data.single.message, message);
      send_message(sock, &msg);
      continue;
    }
    printf("-> [UNKOWN CMD]\n");
  }
  // Close the socket and wait for the thread to exit
  close(sock);
  pthread_join(tid, NULL);
  return 0;
}