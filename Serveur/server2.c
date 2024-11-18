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

    // Free the friends list
    for (int i = 0; i < clients[to_remove].friend_count; i++) {
        if (clients[to_remove].friends[i] != NULL) {
            free(clients[to_remove].friends[i]);
        }
    }
    free(clients[to_remove].friends);

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

void handle_challenge(const char *target_name, Client *challenger, Client *clients, int actual) {
    int found = 0;
    for (int i = 0; i < actual; i++) {
        if (strcmp(clients[i].name, target_name) == 0) {
            found = 1;

            if (clients[i].status != AVAILABLE) {
                write_client(challenger->sock, "Player is not available.\n");
                return;
            }

            // Store the challenger's name in the challenged client's structure
            clients[i].is_challenged = 1;
            strncpy(clients[i].challenger, challenger->name, BUF_SIZE);

            // Notify the challenged client
            char message[BUF_SIZE];
            snprintf(message, BUF_SIZE, "You have been challenged by %s.\nType 'ACCEPT' to accept or 'DENY' to deny.\n", challenger->name);
            write_client(clients[i].sock, message);

            // Notify the challenger
            write_client(challenger->sock, "Challenge sent.\n");

            break;
        }
    }

    if (!found) {
        write_client(challenger->sock, "Player not found.\n");
    }
}

void handle_challenge_response(Client *clients, int actual, int i, const char *buffer, GameProcess *game_processes, int *game_count) {
    if (strncmp(buffer, "ACCEPT", 6) == 0) {
        char challenger_name[BUF_SIZE];
        strncpy(challenger_name, clients[i].challenger, BUF_SIZE);

        int found = 0;
        for (int j = 0; j < actual; j++) {
            if (strcmp(clients[j].name, challenger_name) == 0) {
                found = 1;

                // Notify both players that the challenge is accepted
                write_client(clients[i].sock, "Challenge accepted.\n");
                write_client(clients[j].sock, "Challenge accepted.\n");

                // Fork a new process for the game
                pid_t pid = fork();
                if (pid == 0) {
                    // Child process
                    start_game(&clients[j], &clients[i]);
                    exit(0);
                } else if (pid > 0) {
                    // Parent process
                    // Mark the clients as in-game
                    clients[j].status = IN_GAME;
                    clients[i].status = IN_GAME;

                    // Store the game process info
                    game_processes[*game_count].pid = pid;
                    game_processes[*game_count].player1 = &clients[j];
                    game_processes[*game_count].player2 = &clients[i];
                    (*game_count)++;
                } else {
                    perror("fork()");
                }

                break;
            }
        }

        if (!found) {
            write_client(clients[i].sock, "Challenger not found.\n");
        }
    } else if (strncmp(buffer, "DENY", 4) == 0) {
        // Handle challenge denial
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
    } else {
        write_client(clients[i].sock, "Invalid response. Type 'ACCEPT' or 'DENY'.\n");
    }
}
void handle_friend_request(char *buffer, Client *clients, int sender_index, int actual) {
    char *target_name = buffer + strlen("FRIEND_REQUEST ");
    // Trim leading and trailing whitespace
    while (*target_name == ' ') target_name++;
    char *end = target_name + strlen(target_name) - 1;
    while (end > target_name && *end == ' ') end--;
    *(end + 1) = '\0';

    // Find the target client
    int target_index = -1;
    for (int i = 0; i < actual; i++) {
        if (clients[i].is_active && strcmp(clients[i].name, target_name) == 0) {
            target_index = i;
            break;
        }
    }

    if (target_index == -1) {
        write_client(clients[sender_index].sock, "User not found.\n");
        return;
    }

    Client *sender = &clients[sender_index];
    Client *target = &clients[target_index];

    // Check if already friends
    for (int i = 0; i < sender->friend_count; i++) {
        if (strcmp(sender->friends[i], target->name) == 0) {
            write_client(sender->sock, "You are already friends with this user.\n");
            return;
        }
    }

    // Check if target is already being requested
    if (target->is_friend_requested && strcmp(target->friend_requester, sender->name) == 0) {
        write_client(sender->sock, "You have already sent a friend request to this user.\n");
        return;
    }

    // Send friend request
    target->is_friend_requested = 1;
    strncpy(target->friend_requester, sender->name, BUF_SIZE - 1);
    target->friend_requester[BUF_SIZE - 1] = '\0';

    // Notify the target client
    char message[BUF_SIZE];
    snprintf(message, BUF_SIZE, "%s has sent you a friend request.\nType 'ACCEPT_FRIEND' to accept or 'REJECT_FRIEND' to reject.\n", sender->name);
    write_client(target->sock, message);

    // Notify the sender
    write_client(sender->sock, "Friend request sent.\n");
}
void handle_friend_response(Client *clients, int responder_index, int actual, int accept) {
    Client *responder = &clients[responder_index];

    if (!responder->is_friend_requested) {
        write_client(responder->sock, "You have no pending friend requests.\n");
        return;
    }

    // Find the requester
    int requester_index = -1;
    for (int i = 0; i < actual; i++) {
        if (clients[i].is_active && strcmp(clients[i].name, responder->friend_requester) == 0) {
            requester_index = i;
            break;
        }
    }

    if (requester_index == -1) {
        // Requester is no longer online
        responder->is_friend_requested = 0;
        memset(responder->friend_requester, 0, BUF_SIZE);
        write_client(responder->sock, "Friend requester is no longer online.\n");
        return;
    }

    Client *requester = &clients[requester_index];

    if (accept) {
        // Add each other as friends
        if (responder->friend_count < MAX_FRIENDS && requester->friend_count < MAX_FRIENDS) {
            // Add to responder's friends
            responder->friends[responder->friend_count] = malloc(BUF_SIZE * sizeof(char));
            if (responder->friends[responder->friend_count] != NULL) {
                strncpy(responder->friends[responder->friend_count], requester->name, BUF_SIZE - 1);
                responder->friends[responder->friend_count][BUF_SIZE - 1] = '\0';
                responder->friend_count++;
            }

            // Add to requester's friends
            requester->friends[requester->friend_count] = malloc(BUF_SIZE * sizeof(char));
            if (requester->friends[requester->friend_count] != NULL) {
                strncpy(requester->friends[requester->friend_count], responder->name, BUF_SIZE - 1);
                requester->friends[requester->friend_count][BUF_SIZE - 1] = '\0';
                requester->friend_count++;
            }

            // Notify both clients
            write_client(responder->sock, "Friend request accepted. You are now friends.\n");
            write_client(requester->sock, "Your friend request has been accepted.\n");
        } else {
            write_client(responder->sock, "Friend list is full. Cannot add new friend.\n");
            write_client(requester->sock, "Friend request accepted but friend list is full.\n");
        }
    } else {
        // Reject the friend request
        write_client(responder->sock, "Friend request rejected.\n");
        write_client(requester->sock, "Your friend request has been rejected.\n");
    }

    // Clear the pending friend request
    responder->is_friend_requested = 0;
    memset(responder->friend_requester, 0, BUF_SIZE);
}

void send_online_clients_list(Client *clients, int actual, SOCKET sock) {
    char list[BUF_SIZE] = "Online users:\n";
    for (int i = 0; i < actual; i++) {
        if (!clients[i].is_active)
            continue;

        char status[20];
        switch (clients[i].status) {
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
        snprintf(list + strlen(list), BUF_SIZE - strlen(list),
                 "%s [%s]\n", clients[i].name, status);
    }
    write_client(sock, list);
}

static void app(void) {
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    int actual = 0;
    Client clients[MAX_CLIENTS];
    GameProcess game_processes[MAX_CLIENTS / 2];
    int game_count = 0;

    fd_set rdfs;

    while (1) {
        int i = 0;
        FD_ZERO(&rdfs);

        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        int max_fd = sock;
        for (i = 0; i < actual; i++) {
            FD_SET(clients[i].sock, &rdfs);
            if (clients[i].sock > max_fd) {
                max_fd = clients[i].sock;
            }
        }

        // Check for terminated child processes
        pid_t pid;
        int status;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            // Find the corresponding game
            for (int g = 0; g < game_count; g++) {
                if (game_processes[g].pid == pid) {
                    // Update player statuses
                    if (game_processes[g].player1 != NULL) {
                        game_processes[g].player1->status = AVAILABLE;
                    }
                    if (game_processes[g].player2 != NULL) {
                        game_processes[g].player2->status = AVAILABLE;
                    }

                    // Remove the game from the list
                    game_processes[g] = game_processes[game_count - 1];
                    game_count--;
                    break;
                }
            }
        }

        if (select(max_fd + 1, &rdfs, NULL, NULL, NULL) == -1) {
            perror("select()");
            exit(errno);
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs)) {
            // Handle server input
            fgets(buffer, BUF_SIZE - 1, stdin);
            buffer[strlen(buffer) - 1] = '\0';
            if (strcmp(buffer, "QUIT") == 0) {
                break;
            } else if (strcmp(buffer, "LIST") == 0) {
                send_online_clients_list(clients, actual, STDOUT_FILENO);
            }
        } else if (FD_ISSET(sock, &rdfs)) {
            // Handle new connection
            struct sockaddr_in csin = { 0 };
            socklen_t sinsize = sizeof(csin);
            int csock = accept(sock, (struct sockaddr *)&csin, &sinsize);
            if (csock == -1) {
                perror("accept()");
                continue;
            }

            if (actual >= MAX_CLIENTS) {
                write_client(csock, "Server full\n");
                close(csock);
                continue;
            }

            // Ask for the client's name
            write_client(csock, "Enter your name: ");
            if (read_client(csock, buffer) <= 0) {
                close(csock);
                continue;
            }
            buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

            Client c;
            c.sock = csock;
            strncpy(c.name, buffer, BUF_SIZE - 1);
            c.name[BUF_SIZE - 1] = '\0';
            c.status = AVAILABLE;
            c.is_active = 1;
            c.is_challenged = 0;
            memset(c.challenger, 0, BUF_SIZE);
            // Initialize friend system fields
                c.is_friend_requested = 0;
                memset(c.friend_requester, 0, BUF_SIZE);
                c.friend_count = 0;

                // Allocate memory for the friends array
                c.friends = malloc(MAX_FRIENDS * sizeof(char *));
                if (c.friends == NULL) {
                    perror("malloc");
                    // Handle allocation failure
                }

                // Initialize friend pointers to NULL
                for (int i = 0; i < MAX_FRIENDS; i++) {
                    c.friends[i] = NULL;
                }

            clients[actual] = c;
            actual++;

            char welcome_message[BUF_SIZE];
                snprintf(welcome_message, BUF_SIZE, 
                    "Welcome %s!\n"
                    "Available commands:\n"
                    " - 'LIST' to see online users\n"
                    " - 'CHALLENGE:<name>' to challenge someone\n"
                    " - 'FRIEND_REQUEST <name>' to send a friend request\n"
                    " - 'ACCEPT_FRIEND' to accept a friend request\n"
                    " - 'REJECT_FRIEND' to reject a friend request\n"
                    " - 'LIST_FRIENDS' to list your friends\n"
                    " - 'FORFEIT' to forfeit a game\n", c.name);
                write_client(csock, welcome_message);
        } else {
            for (i = 0; i < actual; i++) {
                if (FD_ISSET(clients[i].sock, &rdfs)) {
                    int c = read_client(clients[i].sock, buffer);
                    if (c <= 0) {
                        // Client disconnected
                        remove_client(clients, i, &actual, game_processes, &game_count);
                        continue;
                    }   

                    // Handle client input
                    if (strncmp(buffer, "CHALLENGE:", 10) == 0) {
                        handle_challenge(buffer + 10, &clients[i], clients, actual);
                    } else if (strcmp(buffer, "LIST") == 0) {
                        send_online_clients_list(clients, actual, clients[i].sock);
                    } else if (clients[i].is_challenged && strncmp(buffer, "ACCEPT", 6) == 0) {
                        handle_challenge_response(clients, actual, i, buffer, game_processes, &game_count);
                        clients[i].is_challenged = 0;
                        memset(clients[i].challenger, 0, BUF_SIZE);
                    } else if (clients[i].is_challenged && strncmp(buffer, "DENY", 4) == 0) {
                        handle_challenge_response(clients, actual, i, buffer, game_processes, &game_count);
                        clients[i].is_challenged = 0;
                        memset(clients[i].challenger, 0, BUF_SIZE);
                    } else if (strncmp(buffer, "FORFEIT", 7) == 0) {
                        // Handle forfeit
                        for (int g = 0; g < game_count; g++) {
                            if (game_processes[g].player1 == &clients[i] || game_processes[g].player2 == &clients[i]) {
                                Client *other_player = (game_processes[g].player1 == &clients[i]) ? game_processes[g].player2 : game_processes[g].player1;
                                write_client(clients[i].sock, "You have forfeited the game.\n");
                                write_client(other_player->sock, "Your opponent has forfeited the game. You win!\n");

                                // Update statuses
                                clients[i].status = AVAILABLE;
                                other_player->status = AVAILABLE;

                                // Remove the game process
                                game_processes[g] = game_processes[game_count - 1];
                                game_count--;
                                break;
                            }
                        }
                    } else if (strncmp(buffer, "FRIEND_REQUEST ", 15) == 0) {
                        handle_friend_request(buffer, clients, i, actual);
                    } else if (strcmp(buffer, "ACCEPT_FRIEND") == 0) {
                        handle_friend_response(clients, i, actual, 1);
                    } else if (strcmp(buffer, "REJECT_FRIEND") == 0) {
                        handle_friend_response(clients, i, actual, 0);
                    } else if (strcmp(buffer, "LIST_FRIENDS") == 0) {
                        Client *client = &clients[i];
                        char message[BUF_SIZE];
                        if (client->friend_count == 0) {
                            snprintf(message, BUF_SIZE, "You have no friends added.\n");
                        } else {
                            snprintf(message, BUF_SIZE, "Your friends:\n");
                            for (int j = 0; j < client->friend_count; j++) {
                                if (client->friends[j] != NULL) {
                                    strncat(message, client->friends[j], BUF_SIZE - strlen(message) - 1);
                                    strncat(message, "\n", BUF_SIZE - strlen(message) - 1);
                                }
                            }
                        }
                        write_client(client->sock, message); 
                    }else {
                        write_client(clients[i].sock, "Unknown command.\n");
                    }
                }
            }
        }
    }

    for (int i = 0; i < actual; i++) {
        close(clients[i].sock);
        // Free the friends list
    for (int j = 0; j < clients[i].friend_count; j++) {
        if (clients[i].friends[j] != NULL) {
            free(clients[i].friends[j]);
        }
    }
    free(clients[i].friends);
    }
    end_connection(sock);
}

int main(void) {
    init();
    app();
    end();
    return 0;
}
