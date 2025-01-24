#ifndef COMMON_H
#define COMMON_H

// String length configuration
#define STR_LEN 128
#define MSG_LEN 1024

// Server address, can be modified according to actual situation
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5566

// Message types, used to distinguish processing
enum type_message {
  ACK_MSG = 0,
  LOGIN_MSG = 1,
  LOGOUT_MSG = 2,
  GROUP_MSG = 3,
  SINGLE_MSG = 4,
  QUERY_MSG = 5,
};

// Login message format
typedef struct login_message {
  char username[STR_LEN];
  char password[STR_LEN];
} LOGIN_MESSAGE;

// Logout message format
typedef struct logout_message {
  char username[STR_LEN];
} LOGOUT_MESSAGE;

// Group message format
typedef struct group_message {
  char from_user[STR_LEN];
  char message[MSG_LEN];
} GROUP_MESSAGE;

// Single message format
typedef struct single_message {
  char from_user[STR_LEN];
  char to_user[STR_LEN];
  char message[MSG_LEN];
} SINGLE_MESSAGE;

// Query message format
typedef struct query_message {
  char username[STR_LEN];
} QUERY_MESSAGE;

// Server response message format
typedef struct ack_message {
  int error;
  char info[MSG_LEN];
} ACK_MESSAGE;

// Unified message format, distinguished by type
typedef struct message {
  int type; // enum type_message
  union {
    struct login_message login;
    struct logout_message logout;
    struct group_message group;
    struct single_message single;
    struct query_message query;
    struct ack_message ack;
  } data;
} MESSAGE;

// Unified interface for sending messages
int send_message(int sock, MESSAGE *msg);
// Unified interface for receiving messages
int recv_message(int sock, MESSAGE *msg);
// Clean up string '\r' '\n' characters
void clear_str(char *s);

#endif