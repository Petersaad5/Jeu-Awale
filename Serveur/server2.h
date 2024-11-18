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
#endif

#include <sys/types.h>   // For pid_t

#define CRLF "\r\n"
#define PORT 1977
#define MAX_CLIENTS 100
#define BUF_SIZE 1024

#include "client2.h"
#include "awale.h"

// Structure to represent a game
typedef struct {
    PlateauAwale plateau;
    Client *player1;
    Client *player2;
    int current_player;
} Game;

// Define the GameProcess structure
typedef struct {
    pid_t pid;           // Process ID of the game (child process)
    Client *player1;     // Pointer to player 1
    Client *player2;     // Pointer to player 2
} GameProcess;

// Function declarations
static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients,int actual, const char *buffer, char from_server);
static void clear_clients(Client *clients, int actual);
void handle_challenge(const char *target_name, Client *challenger, Client *clients, int actual);
void handle_challenge_response(Client *clients, int actual, int i, const char *buffer, GameProcess *game_processes, int *game_count);
void start_game(Client *player1, Client *player2);
void handle_move(Game *game, Client *player, int case_selectionnee);
static void send_online_clients_list(Client *clients, int actual, SOCKET sock);
const char* get_status_string(ClientStatus status);
void get_board_state(const PlateauAwale *plateau, char *buffer, size_t buffer_size);// Declare the get_board_state function

void start_game(Client *player1, Client *player2);
void remove_client(Client *clients, int to_remove, int *actual, GameProcess *game_processes, int *game_count);
void write_client(SOCKET sock, const char *buffer);
int read_client(SOCKET sock, char *buffer);

#endif /* SERVER2_H */
