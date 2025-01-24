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

#define MAX_CONNECT_NUMBER 5

// Server-side user information and status management data structure
typedef struct user_info {
  int status;             // 1: online, 0: offline
  int sock;               // client socket
  pthread_t tid;          // Thread ID handling the user
  char username[STR_LEN]; // Username
  char password[STR_LEN]; // Password
  struct user_info *next; // Traverse users
  struct user_info *head; // Head of the user linked list
} USER_INFO;

// Initialize TCP server
int init_server(int port) {
  // Create TCP socket resource
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("create socket error");
    goto fail;
  }

  // Enable reuse address option to avoid address binding failure after process exit
  int value = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) != 0) {
    perror("set socket option fail");
    goto fail;
  }

  // Bind server address 0.0.0.0:5566
  struct sockaddr_in server_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind socket error");
    goto fail;
  }

  // Listen to the number of users, modify to control the number
  if (listen(sock, MAX_CONNECT_NUMBER) < 0) {
    perror("listen socket error");
    goto fail;
  }

  return sock;

fail:
  if (sock > 0) {
    close(sock);
  }
  return -1;
}

// Load user list, read usernames and passwords for login verification
USER_INFO *load_users(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("open file error");
    return NULL;
  }

  USER_INFO *head = NULL;
  while (1) {
    char username[STR_LEN];
    char password[STR_LEN];
    if (2 != fscanf(fp, "%s %s", username, password)) {
      break;
    }
    clear_str(username);
    clear_str(password);

    // Allocate user management data structure
    USER_INFO *temp = malloc(sizeof(USER_INFO));
    memset(temp, 0, sizeof(USER_INFO));

    // Initialize fields
    temp->status = 0;
    temp->sock = -1;
    temp->next = head;
    strcpy(temp->username, username);
    strcpy(temp->password, password);
    printf("load %s:%s successful\n", username, password);

    head = temp; // Insert into the head of the linked list
  }

  fclose(fp);

  // Update the head field for all users, allowing each user to traverse all user information
  for (USER_INFO *tmp = head; tmp != NULL; tmp = tmp->next) {
    tmp->head = head;
  }
  return head;
}

int users_online_number(USER_INFO *head) {
  int couter = 0;
  for (USER_INFO *tmp = head; tmp != NULL; tmp = tmp->next) {
    if (tmp->status) {
      couter++; // Accumulate if user status is online
    }
  }
  return couter;
}

// When a socket request is established, it will first verify login information. If successful, it will continue to receive messages.
// If login verification fails, close the socket and stop message processing.
USER_INFO *login_msg(USER_INFO *head, int sock) {
  MESSAGE msg;
  USER_INFO *usr = NULL;

  memset(&msg, 0, sizeof(MESSAGE));
  if (recv_message(sock, &msg) != 0) {
    return NULL;
  }
  if (msg.type != LOGIN_MSG) {
    printf("bad request type");
    return NULL;
  }
  ACK_MESSAGE ack;
  LOGIN_MESSAGE *lm = &msg.data.login;

  sprintf(ack.info, "login %s not exist", lm->username);

  // Verify the number of logged-in users, limit the maximum number of logged-in users
  if (users_online_number(head) >= MAX_CONNECT_NUMBER) {
    sprintf(ack.info, "login users too much");
  } else {
    // Verify login authentication information
    for (USER_INFO *tmp = head; tmp != NULL; tmp = tmp->next) {
      if (strcmp(tmp->username, lm->username) == 0 &&
          strcmp(tmp->password, lm->password) == 0) {
        if (tmp->status) {
          // If user status is online, consider it a duplicate login, return failure
          sprintf(ack.info, "login %s repeat", lm->username);
        } else {
          // Login successful
          sprintf(ack.info, "login %s successful", lm->username);
          usr = tmp;
        }
        break;
      }
    }
  }

  msg.type = ACK_MSG;
  if (usr) {
    // usr obtained successfully
    usr->sock = sock;
    usr->status = 1;
    ack.error = 0;
  } else {
    // usr obtained failed, indicating login failure
    ack.error = 1;
  }
  memcpy(&msg.data.ack, &ack, sizeof(ack));
  send_message(sock, &msg);

  return usr;
}

// Send message to a specific user
int send_message_to_user(USER_INFO *usr, MESSAGE *msg) {
  SINGLE_MESSAGE *smsg = &msg->data.single;
  for (USER_INFO *tmp = usr->head; tmp != NULL; tmp = tmp->next) {
    if (strcmp(tmp->username, smsg->to_user) == 0) {
      if (tmp->status) {
        return send_message(tmp->sock, msg);
      } else {
        // User offline, send failed
        printf("forward message from %s to %s fail, offline\n", smsg->from_user,
               smsg->to_user);
        return -1;
      }
    }
  }
  // User does not exist, return failure
  printf("forward message from %s to %s fail, no user\n", smsg->from_user,
         smsg->to_user);
  return -1;
}

// Broadcast message to all users
void send_message_to_group(USER_INFO *usr, MESSAGE *msg) {
  for (USER_INFO *tmp = usr->head; tmp != NULL; tmp = tmp->next) {
    // Send to other online users except oneself
    if (strcmp(usr->username, tmp->username) != 0 && tmp->status) {
      send_message(tmp->sock, msg);
    }
  }
}

// Traverse all user status information
void dump_other_user_status(USER_INFO *usr, MESSAGE *msg) {
  int add = 0;
  int i = 0;
  for (USER_INFO *tmp = usr->head; tmp != NULL; tmp = tmp->next) {
    // \e[32m marks green characters
    // \e[31m marks red characters
    add +=
        sprintf(&msg->data.ack.info[add], "\n%d\t%s\t%s", i++, tmp->username,
                tmp->status ? "\e[32m online \e[0m" : "\e[31m offline \e[0m");
  }
}

// Thread handling each user
void *proc_task(void *arg) {
  USER_INFO *usr = (USER_INFO *)arg;
  usr->tid = pthread_self();
  printf("login %s successful\n", usr->username);

  MESSAGE msg;
  while (1) {
    // Receive message
    memset(&msg, 0, sizeof(MESSAGE));
    if (recv_message(usr->sock, &msg) != 0) {
      break;
    }
    // If it's a logout message, break the loop
    if (msg.type == LOGOUT_MSG) {
      printf("user %s logout\n", usr->username);
      break;
    }
    ACK_MESSAGE *ack = &msg.data.ack;
    switch (msg.type) {
    case SINGLE_MSG: {
      // Handle single message
      SINGLE_MESSAGE *slm = &msg.data.single;
      printf("single message from %s to %s\n", slm->from_user, slm->to_user);
      if (send_message_to_user(usr, &msg) != 0) {
        ack->error = 1;
        sprintf(ack->info, "send fail");
      } else {
        ack->error = 0;
      }
      msg.type = ACK_MSG;
      send_message(usr->sock, &msg);
    } break;
    case GROUP_MSG: {
      // Handle group message
      GROUP_MESSAGE *grm = &msg.data.group;
      printf("group message from %s to all user\n", grm->from_user);
      send_message_to_group(usr, &msg);
      msg.type = ACK_MSG;
      ack->error = 0;
      send_message(usr->sock, &msg);
    } break;
    case QUERY_MSG: {
      // Handle query message
      printf("query message from %s\n", usr->username);
      msg.type = ACK_MSG;
      ack->error = 0;
      dump_other_user_status(usr, &msg);
      send_message(usr->sock, &msg);
    } break;

    default:
      break;
    }
  }
  // Release socket resource
  if (usr->sock > 0) {
    close(usr->sock);
    usr->sock = -1;
  }
  // Update status to offline
  usr->status = 0;
  printf("logout %s successful\n", usr->username);

  return NULL;
}

int main(int argc, char **argv) {
  USER_INFO *users;

  if (argc != 2) {
    printf("Usage: ./userdata.txt\n");
    return -1;
  }
  // Load user information based on the input filename
  users = load_users(argv[1]);
  if (users == NULL) {
    printf("load users fail\n");
    return -1;
  }
  // Initialize server socket
  int sock = init_server(SERVER_PORT);
  if (sock < 0) {
    return -1;
  }

  printf("init server successful\n");
  // Accept client connection requests
  while (1) {
    pthread_t tid;
    int sock_cli = accept(sock, NULL, NULL);
    if (sock_cli < 0) {
      perror("accept error\n");
      continue;
    }
    // Verify user login information
    USER_INFO *usr = login_msg(users, sock_cli);
    if (usr == NULL) {
      // If failed, destroy the socket
      close(sock_cli);
      continue;
    }
    // Create thread to handle user requests
    if (pthread_create(&tid, NULL, proc_task, usr) != 0) {
      perror("create task fail\n");
      break;
    }
  }
  // Close all user resources, server-side resource cleanup
  for (USER_INFO *tmp = users; tmp != NULL; tmp = tmp->next) {
    if (tmp->sock > 0) {
      close(tmp->sock);
      pthread_join(tmp->tid, NULL);
    }
  }
  // Close server socket
  close(sock);
  return 0;
}