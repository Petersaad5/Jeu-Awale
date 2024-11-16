#ifndef SERVER2_H
#define SERVER2_H

#ifdef WIN32
#include <winsock2.h>
#elif defined(linux)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
#else
#error Platform not supported
#endif

#define CRLF "\r\n"
#define PORT 1977
#define MAX_CLIENTS 100
#define BUF_SIZE 1024

#include "client2.h"

// Function declarations
static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
void handle_challenge(const char *target_name, Client *challenger, Client *clients, int actual);
static void send_online_clients_list(Client *clients, int actual, SOCKET sock);
void update_client_status(Client *client, const char *status);  
const char* get_status_string(ClientStatus status); // Function to get the status as a string
void handle_challenge_response(Client *clients, int actual, int i, const char *buffer);

#endif /* SERVER2_H */