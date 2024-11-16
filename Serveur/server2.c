#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "server2.h"
#include "client2.h"
#include "awale.h" // Include the header file for Awale functions
#include <sys/select.h>



static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed!");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   int actual = 0;
   int max = sock;
   Client clients[MAX_CLIENTS];

   fd_set rdfs;

   while (1)
   {
      int i = 0;
      FD_ZERO(&rdfs);
      FD_SET(STDIN_FILENO, &rdfs);
      FD_SET(sock, &rdfs);

      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         SOCKADDR_IN csin = {0};
         socklen_t sinsize = sizeof(csin);
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         if (read_client(csock, buffer) == -1)
         {
            continue;
         }

         max = csock > max ? csock : max;
         FD_SET(csock, &rdfs);

         Client c = {csock};
         strncpy(c.name, buffer, BUF_SIZE - 1);
         c.name[BUF_SIZE - 1] = '\0'; // Ensure null-termination
         c.status = AVAILABLE; // Default status
         clients[actual] = c;
         actual++;

         char welcome_message[BUF_SIZE];
         snprintf(welcome_message, BUF_SIZE, "Welcome %.50s! Type 'LIST' for online users or 'CHALLENGE:<pseudo>' to challenge someone.", c.name);
         write_client(csock, welcome_message);
      }
      else
      {
         for (i = 0; i < actual; i++)
         {
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);

               if (c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  snprintf(buffer, BUF_SIZE, "%.50s disconnected!", client.name);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else if (strncmp(buffer, "LIST", 4) == 0)
               {
                  send_online_clients_list(clients, actual, client.sock);
               }
               else if (strncmp(buffer, "CHALLENGE:", 10) == 0)
               {
                  // Challenge handling
                  char opponent_name[BUF_SIZE];
                  strncpy(opponent_name, buffer + 10, BUF_SIZE - 10); // Correct the offset to start after "CHALLENGE:"
                  opponent_name[BUF_SIZE - 10] = '\0'; // Ensure null-termination

                  handle_challenge(opponent_name, &clients[i], clients, actual);
               }
               else
               {
                  handle_challenge_response(clients, actual, i, buffer);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, 10) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = recv(sock, buffer, BUF_SIZE - 1, 0);
   if (n < 0)
   {
      perror("recv()");
      exit(errno);
   }
   buffer[n] = '\0';
   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   char message[BUF_SIZE];
   message[0] = 0;
   for (int i = 0; i < actual; i++)
   {
      if (sender.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            snprintf(message, BUF_SIZE, "%.50s: %s", sender.name, buffer);
         }
         else
         {
            snprintf(message, BUF_SIZE, "%s", buffer);
         }
         write_client(clients[i].sock, message);
      }
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   closesocket(clients[to_remove].sock);
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   (*actual)--;
}

static void clear_clients(Client *clients, int actual)
{
   for (int i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

void handle_challenge(const char *target_name, Client *challenger, Client *clients, int actual) {
    int found = 0;
    for (int i = 0; i < actual; i++) {
        if (strcmp(clients[i].name, target_name) == 0) {
            found = 1;

            // Store the challenger's name in the challenged client's structure
            clients[i].is_challenged = 1;
            strncpy(clients[i].challenger, challenger->name, BUF_SIZE);

            // Notify the challenged client
            char message[BUF_SIZE];
            snprintf(message, BUF_SIZE, "You have been challenged by %.996s.\n", challenger->name);
            write_client(clients[i].sock, message);

            // Notify the challenger
            write_client(challenger->sock, "Challenge sent.\n");

            break;
        }
    }

    if (!found) {
        write_client(challenger->sock, "Target not found.\n");
    }
}

void handle_challenge_response(Client *clients, int actual, int i, const char *buffer) {
    if (strncmp(buffer, "ACCEPT", 6) == 0) {
        // Challenge acceptance
        char challenger_name[BUF_SIZE];
        strncpy(challenger_name, clients[i].challenger, BUF_SIZE);

        int found = 0;
        for (int j = 0; j < actual; j++) {
            if (strcmp(clients[j].name, challenger_name) == 0) {
                found = 1;

                // Notify both players that the challenge is accepted
                write_client(clients[i].sock, "Challenge accepted.\n");
                write_client(clients[j].sock, "Challenge accepted.\n");

                // Start the game
                start_game(&clients[j], &clients[i]);

                break;
            }
        }

        if (!found) {
            write_client(clients[i].sock, "Challenger not found.\n");
        }
    } else if (strncmp(buffer, "DENY", 4) == 0) {
        // Challenge denial
        char challenger_name[BUF_SIZE];
        strncpy(challenger_name, clients[i].challenger, BUF_SIZE);

        int found = 0;
        for (int j = 0; j < actual; j++) {
            if (strcmp(clients[j].name, challenger_name) == 0) {
                found = 1;

                // Notify both players that the challenge is denied
                write_client(clients[i].sock, "Challenge denied.\n");
                write_client(clients[j].sock, "Challenge denied.\n");

                break;
            }
        }

        if (!found) {
            write_client(clients[i].sock, "Challenger not found.\n");
        }
    }
}

void start_game(Client *player1, Client *player2) {
    // Initialize the game
    Game game;
    initialiser_plateau(&game.plateau);

    // Randomly decide who is player 1 and who is player 2
    srand(time(NULL));
    if (rand() % 2 == 0) {
        game.player1 = player1;
        game.player2 = player2;
    } else {
        game.player1 = player2;
        game.player2 = player1;
    }

    game.current_player = 1;

    // Notify players of their roles
    write_client(game.player1->sock, "You are Player 1.\n");
    write_client(game.player2->sock, "You are Player 2.\n");

    // Show the initial board to both players
    char board_state[BUF_SIZE];
    get_board_state(&game.plateau, board_state, BUF_SIZE);
    write_client(game.player1->sock, board_state);
    write_client(game.player2->sock, board_state);

    // Start the game loop
    while (1) {
        char buffer[BUF_SIZE];
        int case_selectionnee;

        // Get the move from the current player
        if (game.current_player == 1) {
            write_client(game.player1->sock, "Your turn. Select a case: ");
            read_client(game.player1->sock, buffer);
            case_selectionnee = atoi(buffer);
            handle_move(&game, game.player1, case_selectionnee);
        } else {
            write_client(game.player2->sock, "Your turn. Select a case: ");
            read_client(game.player2->sock, buffer);
            case_selectionnee = atoi(buffer);
            handle_move(&game, game.player2, case_selectionnee);
        }

        // Switch player
        game.current_player = (game.current_player == 1) ? 2 : 1;

        // Check if the game is over
        if (est_fin_de_partie(&game.plateau)) {
            char message[BUF_SIZE];
            snprintf(message, BUF_SIZE, "Game over. Score - Player 1: %d, Player 2: %d\n", game.plateau.score_joueur1, game.plateau.score_joueur2);
            write_client(game.player1->sock, message);
            write_client(game.player2->sock, message);

            // Set players' status back to available
            game.player1->status = AVAILABLE;
            game.player2->status = AVAILABLE;

            break;
        }
    }
}

void handle_move(Game *game, Client *player, int case_selectionnee) {
    // Show the board state before the move
    char board_state[BUF_SIZE];
    get_board_state(&game->plateau, board_state, BUF_SIZE);
    write_client(game->player1->sock, board_state);
    write_client(game->player2->sock, board_state);

    int result = jouer_coup(&game->plateau, game->current_player, case_selectionnee);
    if (result == -1) {
        write_client(player->sock, "Illegal move. Try again.\n");
    } else {
        // Show the board state after the move
        get_board_state(&game->plateau, board_state, BUF_SIZE);
        write_client(game->player1->sock, board_state);
        write_client(game->player2->sock, board_state);
    }
}

static void send_online_clients_list(Client *clients, int actual, SOCKET sock) {
    char list[BUF_SIZE] = "Online users:\n";
    for (int i = 0; i < actual; i++) {
        char client_info[BUF_SIZE];
        snprintf(client_info, BUF_SIZE, "%.50s (%s)\n", clients[i].name, get_status_string(clients[i].status));
        strncat(list, client_info, BUF_SIZE - strlen(list) - 1);
    }
    write_client(sock, list);
}

const char* get_status_string(ClientStatus status) {
    switch (status) {
        case AVAILABLE:
            return "Available";
        case IN_GAME:
            return "In Game";
        case CHALLENGED:
            return "Challenged";
        case SPECTATING:
            return "Spectating";
        case BUSY:
            return "Busy";
        default:
            return "Unknown";
    }
}

int main(void) {
    init();
    app();
    end();
    return 0;
}
