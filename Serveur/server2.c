#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h> // For waitpid
#include "server2.h"
#include "awale.h"

#define MAX_CLIENTS 100

static void init(void) {
    // Initialization code (if needed)
}

static void end(void) {
    // Cleanup code (if needed)
}

static int init_connection(void) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sin = { 0 };

    if (sock == INVALID_SOCKET) {
        perror("socket()");
        exit(errno);
    }

    int optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt()");
        exit(errno);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR) {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR) {
        perror("listen()");
        exit(errno);
    }

    return sock;
}

static void end_connection(int sock) {
    close(sock);
}

int read_client(SOCKET sock, char *buffer) {
    int n = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (n < 0) {
        perror("recv()");
        exit(errno);
    }
    buffer[n] = '\0';
    return n;
}

void write_client(SOCKET sock, const char *buffer) {
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("send()");
        exit(errno);
    }
}

void remove_client(Client *clients, int to_remove, int *actual, GameProcess *game_processes, int *game_count) {
    // Close the client's socket
    close(clients[to_remove].sock);

    // Mark the client as inactive and update status
    clients[to_remove].is_active = 0;
    clients[to_remove].status = AVAILABLE;

    // Check if the client is involved in any game
    for (int g = 0; g < *game_count; g++) {
        if (game_processes[g].player1 == &clients[to_remove] || game_processes[g].player2 == &clients[to_remove]) {
            // Notify the other player
            Client *other_player = (game_processes[g].player1 == &clients[to_remove]) ? game_processes[g].player2 : game_processes[g].player1;
            if (other_player != NULL) {
                write_client(other_player->sock, "Your opponent has disconnected. You win!\n");
                other_player->status = AVAILABLE;
            }

            // Remove the game process
            game_processes[g] = game_processes[*game_count - 1];
            (*game_count)--;

            // Adjust index after removal
            g--;
        }
    }

    // No need to shift clients array since we're marking client as inactive
}

void start_game(Client *player1, Client *player2) {
    PlateauAwale plateau;
    initialiser_plateau(&plateau);

    Client *current_player = player1;
    Client *other_player = player2;
    char buffer[BUF_SIZE];

    while (1) {
        // Send the board state to both players
        char board_state[BUF_SIZE];
        get_board_state(&plateau, board_state, BUF_SIZE);
        write_client(current_player->sock, board_state);
        write_client(other_player->sock, board_state);

        // Prompt current player for move
        write_client(current_player->sock, "Your move (or type 'FORFEIT' to forfeit): ");

        // Read move from current player
        if (read_client(current_player->sock, buffer) <= 0) {
            // Handle disconnection
            write_client(other_player->sock, "Your opponent has disconnected. You win!\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

        if (strcmp(buffer, "FORFEIT") == 0) {
            // Current player forfeits
            write_client(current_player->sock, "You have forfeited the game.\n");
            write_client(other_player->sock, "Your opponent has forfeited. You win!\n");
            break;
        }

        int case_selectionnee = atoi(buffer);

        // Play the move
        int game_over = jouer_coup(&plateau, (current_player == player1) ? 1 : 2, case_selectionnee);

        if (game_over) {
            // Game over
            char end_message[BUF_SIZE];
            snprintf(end_message, BUF_SIZE, "Game over. Final scores - Player 1: %d, Player 2: %d\n",
                     plateau.score_joueur1, plateau.score_joueur2);
            write_client(current_player->sock, end_message);
            write_client(other_player->sock, end_message);
            break;
        }

        // Swap players
        Client *temp = current_player;
        current_player = other_player;
        other_player = temp;
    }

    // Child process exits
    exit(0);
}

int main(void) {
    init();

    SOCKET sock = init_connection();
    printf("Server started on port %d\n", PORT);

    fd_set rdfs;

    Client clients[MAX_CLIENTS];
    int actual = 0;
    GameProcess game_processes[MAX_CLIENTS / 2];
    int game_count = 0;

    while (1) {
        FD_ZERO(&rdfs);

        // Add listening socket
        FD_SET(sock, &rdfs);

        // Add clients sockets
        int max = sock;
        for (int i = 0; i < actual; i++) {
            if (!clients[i].is_active)
                continue;

            FD_SET(clients[i].sock, &rdfs);
            if (clients[i].sock > max)
                max = clients[i].sock;
        }

        // Add stdin
        FD_SET(STDIN_FILENO, &rdfs);
        if (STDIN_FILENO > max)
            max = STDIN_FILENO;

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if (select(max + 1, &rdfs, NULL, NULL, &timeout) < 0) {
            perror("select()");
            exit(errno);
        }

        // Handle new connection
        if (FD_ISSET(sock, &rdfs)) {
            struct sockaddr_in csin = { 0 };
            socklen_t sinsize = sizeof(csin);
            int csock = accept(sock, (struct sockaddr *)&csin, &sinsize);
            if (csock == -1) {
                perror("accept()");
                continue;
            }

            // Ask for client's name
            write_client(csock, "Enter your name: ");
            char buffer[BUF_SIZE];
            if (read_client(csock, buffer) <= 0) {
                close(csock);
                continue;
            }
            buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

            // Initialize new client
            Client c;
            c.sock = csock;
            strncpy(c.name, buffer, BUF_SIZE - 1);
            c.name[BUF_SIZE - 1] = '\0';
            c.status = AVAILABLE;
            c.is_active = 1;
            c.is_challenged = 0;
            memset(c.challenger, 0, BUF_SIZE);

            clients[actual++] = c;

            // Send welcome message
            char welcome_message[BUF_SIZE];
            snprintf(welcome_message, BUF_SIZE,
                     "Welcome %s! Type 'LIST' for online users or 'CHALLENGE:<name>' to challenge someone.\n", c.name);
            write_client(csock, welcome_message);
        }

        // Handle clients messages
        for (int i = 0; i < actual; i++) {
            if (!clients[i].is_active)
                continue;

            if (FD_ISSET(clients[i].sock, &rdfs)) {
                char buffer[BUF_SIZE];
                int n = read_client(clients[i].sock, buffer);
                if (n <= 0) {
                    // Client disconnected
                    remove_client(clients, i, &actual, game_processes, &game_count);
                    continue;
                }
                buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

                // Handle commands
                if (strcasecmp(buffer, "LIST") == 0) {
                    char list_message[BUF_SIZE] = "Online users:\n";
                    for (int j = 0; j < actual; j++) {
                        if (!clients[j].is_active)
                            continue;

                        char status[20];
                        switch (clients[j].status) {
                            case AVAILABLE:
                                strcpy(status, "AVAILABLE");
                                break;
                            case IN_GAME:
                                strcpy(status, "IN_GAME");
                                break;
                            case CHALLENGED:
                                strcpy(status, "CHALLENGED");
                                break;
                            default:
                                strcpy(status, "BUSY");
                                break;
                        }
                        snprintf(list_message + strlen(list_message), BUF_SIZE - strlen(list_message),
                                 "%s [%s]\n", clients[j].name, status);
                    }
                    write_client(clients[i].sock, list_message);
                } else if (strncasecmp(buffer, "CHALLENGE:", 10) == 0) {
                    char *challenged_name = buffer + 10;
                    // Find the challenged client
                    int found = 0;
                    for (int j = 0; j < actual; j++) {
                        if (!clients[j].is_active)
                            continue;

                        if (strcmp(clients[j].name, challenged_name) == 0) {
                            found = 1;
                            if (clients[j].status == AVAILABLE) {
                                // Send challenge request
                                char challenge_message[BUF_SIZE];
                                snprintf(challenge_message, BUF_SIZE, "%s has challenged you! Type 'ACCEPT' to accept.\n", clients[i].name);
                                write_client(clients[j].sock, challenge_message);

                                // Update statuses
                                clients[i].status = BUSY;
                                clients[i].is_challenged = 0;
                                clients[j].status = CHALLENGED;
                                clients[j].is_challenged = 1;
                                strncpy(clients[j].challenger, clients[i].name, BUF_SIZE - 1);
                                clients[j].challenger[BUF_SIZE - 1] = '\0';
                            } else {
                                write_client(clients[i].sock, "The player is not available.\n");
                            }
                            break;
                        }
                    }
                    if (!found) {
                        write_client(clients[i].sock, "Player not found.\n");
                    }
                } else if (strcasecmp(buffer, "ACCEPT") == 0 && clients[i].status == CHALLENGED) {
                    // Find the challenger
                    Client *challenger = NULL;
                    for (int j = 0; j < actual; j++) {
                        if (!clients[j].is_active)
                            continue;

                        if (strcmp(clients[j].name, clients[i].challenger) == 0) {
                            challenger = &clients[j];
                            break;
                        }
                    }
                    if (challenger != NULL) {
                        // Start game
                        pid_t pid = fork();
                        if (pid == 0) {
                            // Child process
                            start_game(challenger, &clients[i]);
                            // Should not reach here
                            exit(0);
                        } else if (pid > 0) {
                            // Parent process
                            // Update statuses
                            challenger->status = IN_GAME;
                            clients[i].status = IN_GAME;

                            // Store game process
                            game_processes[game_count].pid = pid;
                            game_processes[game_count].player1 = challenger;
                            game_processes[game_count].player2 = &clients[i];
                            game_count++;
                        } else {
                            perror("fork()");
                        }
                    } else {
                        write_client(clients[i].sock, "Challenger not found.\n");
                        clients[i].status = AVAILABLE;
                        clients[i].is_challenged = 0;
                        memset(clients[i].challenger, 0, BUF_SIZE);
                    }
                } else {
                    write_client(clients[i].sock, "Unknown command.\n");
                }
            }
        }

        // Handle terminated game processes
        pid_t pid;
        int status;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            for (int g = 0; g < game_count; g++) {
                if (game_processes[g].pid == pid) {
                    // Update client statuses
                    if (game_processes[g].player1 != NULL) {
                        game_processes[g].player1->status = AVAILABLE;
                    }
                    if (game_processes[g].player2 != NULL) {
                        game_processes[g].player2->status = AVAILABLE;
                    }
                    // Remove game process
                    game_processes[g] = game_processes[game_count - 1];
                    game_count--;
                    break;
                }
            }
        }
    }

    end_connection(sock);
    end();

    return EXIT_SUCCESS;
}
