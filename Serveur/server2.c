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

#define ELO_FILE "elo_ratings.txt"
#define MAX_CLIENTS 100
// Declare the array of game processes
//GameProcess *game_processes[MAX_GAMES];
//int game_count = 0;
static void init(void) {
    
}

static void end(void) {
 
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

void start_game(Client *player1, Client *player2,GameProcess *game) {
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

        // Notify spectators
        
        for (int s = 0; s < game->spectator_count; s++) {
            
            write_client(game->spectators[s]->sock, board_state);
        }   

        // Prompt current player for move
        write_client(current_player->sock, "Your move (or type 'FORFEIT' to forfeit): ");

        // Read move from current player
        if (read_client(current_player->sock, buffer) <= 0) {
            // Handle disconnection
            update_elo_ratings(other_player, current_player);
            write_client(other_player->sock, "Your opponent has disconnected. You win!\n");
            snprintf(buffer, sizeof(buffer), "Game over. Your new Elo rating is %d.\n", other_player->elo_rating);
            write_client(other_player->sock, buffer);
            snprintf(buffer, sizeof(buffer), "Game over. Your new Elo rating is %d.\n", current_player->elo_rating);
            write_client(current_player->sock, buffer);
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

        if (strcmp(buffer, "FORFEIT") == 0) {
            // Current player forfeits
            update_elo_ratings(other_player, current_player);
            write_client(current_player->sock, "You have forfeited the game.\n");
            write_client(other_player->sock, "Your opponent has forfeited. You win!\n");
            // Notify players of the new ratings
            snprintf(buffer, sizeof(buffer), "Game over. Your new Elo rating is %d.\n", other_player->elo_rating);
            write_client(other_player->sock, buffer);
            snprintf(buffer, sizeof(buffer), "Game over. Your new Elo rating is %d.\n", current_player->elo_rating);
            write_client(current_player->sock, buffer);
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
            update_elo_ratings(other_player, current_player);
             snprintf(buffer, sizeof(buffer), "Game over. Your new Elo rating is %d.\n", other_player->elo_rating);
            write_client(other_player->sock, buffer);
            snprintf(buffer, sizeof(buffer), "Game over. Your new Elo rating is %d.\n", current_player->elo_rating);
            write_client(current_player->sock, buffer);
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
                    
                    start_game(&clients[j], &clients[i],&game_processes[*game_count]);
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

                    //Set game spectate mode
                    if (clients[j].spectate_mode == 1 || clients[i].spectate_mode == 1) {
                        game_processes[*game_count].friends_only = 1;
                    } else {
                        game_processes[*game_count].friends_only = 0;
                    }

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
            case SPECTATING:
                strcpy(status, "SPECTATING");
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
void handle_set_bio(Client *client) {
    char buffer[BUF_SIZE];
    int lines = 0;
    client->bio_line_count = 0;

    write_client(client->sock, "Enter your bio (up to 10 lines). Type 'END_BIO' on a new line to finish.\n");

    while (lines < MAX_BIO_LINES) {
        int n = read_client(client->sock, buffer);
        if (n <= 0) {
            // Handle client disconnection if necessary
            break;
        }
        buffer[strcspn(buffer, "\r\n")] = '\0'; // Remove newline characters

        if (strcmp(buffer, "END_BIO") == 0) {
            break;
        }
        strncpy(client->bio[lines], buffer, MAX_LINE_LENGTH - 1);
        client->bio[lines][MAX_LINE_LENGTH - 1] = '\0'; // Ensure null termination
        lines++;
    }
    client->bio_line_count = lines;
    write_client(client->sock, "Your bio has been updated.\n");
}
void handle_view_bio(Client *clients, int actual, int requestor_index, const char *target_name) {
    int found = 0;
    for (int j = 0; j < actual; j++) {
        if (clients[j].is_active && strcmp(clients[j].name, target_name) == 0) {
            found = 1;
            write_client(clients[requestor_index].sock, "Bio of ");
            write_client(clients[requestor_index].sock, clients[j].name);
            write_client(clients[requestor_index].sock, ":\n");
            if (clients[j].bio_line_count == 0) {
                write_client(clients[requestor_index].sock, "This user has not set a bio.\n");
            } else {
                for (int k = 0; k < clients[j].bio_line_count; k++) {
                    write_client(clients[requestor_index].sock, clients[j].bio[k]);
                    write_client(clients[requestor_index].sock, "\n");
                }
            }
            break;
        }
    }
    if (!found) {
        write_client(clients[requestor_index].sock, "User not found.\n");
    }
}
void send_ongoing_games_list(GameProcess *game_processes, int game_count, int client_sock) {
    char game_list[BUF_SIZE];
    strcpy(game_list, "Ongoing games:\n");
    int games_found = 0;

    for (int g = 0; g < game_count; g++) {
        if (game_processes[g].player1 != NULL && game_processes[g].player2 != NULL) {
            char game_info[128];
            snprintf(game_info, sizeof(game_info), "Game %d: %s vs %s\n",
                     g + 1, game_processes[g].player1->name, game_processes[g].player2->name);
            strcat(game_list, game_info);
            games_found = 1;
        }
    }

    if (!games_found) {
        strcat(game_list, "No ongoing games.\n");
    }

    write_client(client_sock, game_list);
}

void handle_spectate_request(Client *clients, int client_count, Client *spectator, char *target_name,GameProcess *game_processes, int game_count) {
    // Find the game where target_name is participating
    GameProcess *target_game = NULL;
    int games_found = 0;
    for (int g = 0; g < game_count; g++) {
        GameProcess *game = &game_processes[g];
        if ((game->player1 && strcmp(game->player1->name, target_name) == 0) ||
            (game->player2 && strcmp(game->player2->name, target_name) == 0)) {
            target_game = game;
            break;
        }
    }

    if (!target_game) {
        write_client(spectator->sock, "Game not found.\n");
        return;
    }

    // Check spectate permissions
    if (target_game->friends_only) {
        int is_friend = 0;
        // Check if spectator is a friend of either player
        for (int f = 0; f < target_game->player1->friend_count; f++) {
            if (strcmp(target_game->player1->friends[f], spectator->name) == 0) {
                is_friend = 1;
                break;
            }
        }
        if (!is_friend) {
            for (int f = 0; f < target_game->player2->friend_count; f++) {
                if (strcmp(target_game->player2->friends[f], spectator->name) == 0) {
                    is_friend = 1;
                    break;
                }
            }
        }
        if (!is_friend) {
            write_client(spectator->sock, "Spectating is restricted to friends.\n");
            return;
        }
    }

    // Add spectator to the game
    if (target_game->spectator_count >= MAX_SPECTATORS) {
        write_client(spectator->sock, "Maximum number of spectators reached.\n");
        return;
    }

    target_game->spectators[target_game->spectator_count++] = spectator;
    spectator->is_spectating = 1;
    spectator->spectating_game = target_game;
    spectator->status = SPECTATING;

    // Send the current game state
    char board_state[BUF_SIZE];
    get_board_state(&target_game->plateau, board_state, sizeof(board_state));
    write_client(spectator->sock, "You are now spectating the game.\n");
    write_client(spectator->sock, board_state);
}
int is_friend(Client *client, Client *other) {
    for (int f = 0; f < client->friend_count; f++) {
        if (strcmp(client->friends[f], other->name) == 0) {
            return 1;
        }
    }
    return 0;
}
void update_elo_ratings(Client *winner, Client *loser) {
    int K = 32; // K-factor, determines the sensitivity of rating changes

    // Read current Elo ratings from file
    int winner_elo = read_elo_rating(winner->name);
    int loser_elo = read_elo_rating(loser->name);

    // Calculate expected scores
    double expected_winner = 1.0 / (1.0 + pow(10, (loser_elo - winner_elo) / 400.0));
    double expected_loser = 1.0 / (1.0 + pow(10, (winner_elo - loser_elo) / 400.0));

    // Update ratings
    winner_elo += (int)(K * (1 - expected_winner));
    loser_elo += (int)(K * (0 - expected_loser));

    // Write updated Elo ratings to file
    write_elo_rating(winner->name, winner_elo);
    write_elo_rating(loser->name, loser_elo);

    // Update client structures
    winner->elo_rating = winner_elo;
    loser->elo_rating = loser_elo;
}
void handle_view_elo(Client *clients, int client_count, Client *requesting_client, const char *player_name) {
    // Read Elo rating from file
    int elo_rating = read_elo_rating(player_name);

    // Send Elo rating to the requesting client
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "Player %s has an Elo rating of %d.\n", player_name, elo_rating);
    write_client(requesting_client->sock, buffer);
}
int read_elo_rating(const char *name) {
    FILE *file = fopen(ELO_FILE, "r");
    if (file == NULL) {
        return 1200; // Default Elo rating if file does not exist
    }
    char line[BUF_SIZE];
    char stored_name[BUF_SIZE];
    int stored_elo;
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%s %d", stored_name, &stored_elo);
        if (strcmp(name, stored_name) == 0) {
            fclose(file);
            return stored_elo;
        }
    }
    fclose(file);
    return 1200; // Default Elo rating if user not found
}

// Function to write Elo rating to file
void write_elo_rating(const char *name, int elo_rating) {
    FILE *file = fopen(ELO_FILE, "r+");
    if (file == NULL) {
        file = fopen(ELO_FILE, "w");
    }
    char line[BUF_SIZE];
    char stored_name[BUF_SIZE];
    int stored_elo;
    long pos;
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        pos = ftell(file);
        sscanf(line, "%s %d", stored_name, &stored_elo);
        if (strcmp(name, stored_name) == 0) {
            fseek(file, pos - strlen(line), SEEK_SET);
            fprintf(file, "%s %d\n", name, elo_rating);
            found = 1;
            break;
        }
    }
    if (!found) {
        fprintf(file, "%s %d\n", name, elo_rating);
    }
    fclose(file);
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
            c.spectate_mode = 0;
            // Check if the player's name is already in the file and get the Elo rating
            c.elo_rating = read_elo_rating(c.name);

            // If the player is new, add them to the file with the default Elo rating
            if (c.elo_rating == 1200) {
                FILE *file = fopen(ELO_FILE, "r");
                if (file != NULL) {
                    char line[BUF_SIZE];
                    char stored_name[BUF_SIZE];
                    int stored_elo;
                    int found = 0;
                    while (fgets(line, sizeof(line), file)) {
                        sscanf(line, "%s %d", stored_name, &stored_elo);
                        if (strcmp(c.name, stored_name) == 0) {
                            found = 1;
                            break;
                        }
                    }
                    fclose(file);
                    if (!found) {
                        write_elo_rating(c.name, c.elo_rating);
                    }
                } else {
                    write_elo_rating(c.name, c.elo_rating);
                }
            } 
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
                
                // Initialize bio fields
                c.bio_line_count = 0;
                for (int i = 0; i < MAX_BIO_LINES; i++) {
                    memset(c.bio[i], 0, MAX_LINE_LENGTH);
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
    " - 'FORFEIT' to forfeit a game\n"
    " - 'CHAT_ALL <message>' to send a message to all users\n"
    " - 'CHAT <name> <message>' to send a private message\n"
    " - 'SET_BIO' to set your bio (up to 10 lines)\n"
    " - 'VIEW_BIO <name>' to view a user's bio\n"
    " - 'VIEW_ELO <name>' to view a user's elo \n"
    " - 'SET_SPECTATE_MODE <ALL/FRIENDS>' to set spectate mode\n"
    " - 'LIST_GAMES' to list ongoing games\n"
    " - 'SPECTATE <player_name>' to spectate a game\n"
    " - 'EXIT_SPECTATE' to exit spectating mode\n",c.name);
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
                            
                    }else if (strcmp(buffer, "SET_BIO") == 0) {
                        handle_set_bio(&clients[i]);
                    } else if (strncmp(buffer, "VIEW_BIO ", 9) == 0) {
                        char *target_name = buffer + 9;
                        // Trim leading and trailing whitespace
                        while (*target_name == ' ') target_name++;
                        char *end = target_name + strlen(target_name) - 1;
                        while (end > target_name && isspace((unsigned char)*end)) end--;
                        *(end + 1) = '\0';
                        handle_view_bio(clients, actual, i, target_name);
                        

                    }else if (strcmp(buffer, "LIST_GAMES") == 0) {
                        send_ongoing_games_list(game_processes, game_count, clients[i].sock);
                    }
                    else if (strncmp(buffer, "FRIEND_REQUEST ", 15) == 0) {
                        handle_friend_request(buffer, clients, i, actual);
                    }else if (strncmp(buffer, "VIEW_ELO ", 9) == 0) {
                       handle_view_elo(clients, actual, &clients[i], buffer + 9);
                    } 
                    else if (strcmp(buffer, "ACCEPT_FRIEND") == 0) {
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
                        }else if (strncmp(buffer, "SPECTATE ", 9) == 0) {
                            char *target_name = buffer + 9;
                            while (*target_name == ' ') target_name++;
                            handle_spectate_request(clients, actual, &clients[i], target_name,game_processes, game_count);
                        } else if (strncmp(buffer, "SET_SPECTATE_MODE ", 18) == 0) {
                            char *mode_str = buffer + 18;
                            while (*mode_str == ' ') mode_str++;
                            if (strcmp(mode_str, "ALL") == 0) {
                                clients[i].spectate_mode = 0;  // Allow all to spectate
                                write_client(clients[i].sock, "Spectate mode set to ALL.\n");
                            } else if (strcmp(mode_str, "FRIENDS") == 0) {
                                clients[i].spectate_mode = 1;  // Friends only
                                write_client(clients[i].sock, "Spectate mode set to FRIENDS.\n");
                            } else {
                                write_client(clients[i].sock, "Invalid spectate mode. Use ALL or FRIENDS.\n");
                            }
                    } 
                    else if (strncmp(buffer, "CHAT_ALL ", 9) == 0) {
                        // **Implementing 'CHAT_ALL' Command**
                        char *message = buffer + 9; // Extract the message
                        char full_message[BUF_SIZE];
                        snprintf(full_message, BUF_SIZE, "%s says to everyone: %s\n", clients[i].name, message);
                        // Send the message to all connected clients except the sender
                        for (int j = 0; j < actual; j++) {
                            if (clients[j].is_active && j != i) {
                                write_client(clients[j].sock, full_message);
                            }
                        }
                    } else if (strncmp(buffer, "CHAT ", 5) == 0) {
                        // **Implementing 'CHAT' Command**
                        char *rest = buffer + 5;
                        char *recipient_name = strtok(rest, " ");
                        char *message = strtok(NULL, "\0"); // Get the rest of the message
                        if (recipient_name != NULL && message != NULL) {
                            int recipient_found = 0;
                            for (int j = 0; j < actual; j++) {
                                if (clients[j].is_active && strcmp(clients[j].name, recipient_name) == 0) {
                                    char full_message[BUF_SIZE];
                                    snprintf(full_message, BUF_SIZE, "%s says to you: %s\n", clients[i].name, message);
                                    write_client(clients[j].sock, full_message);
                                    recipient_found = 1;
                                    break;
                                }
                            }
                            if (!recipient_found) {
                                write_client(clients[i].sock, "Recipient not found or not online.\n");
                            }
                        } else {
                            write_client(clients[i].sock, "Invalid command format. Use: 'CHAT <recipient_name> <message>'.\n");
                        }
                    } else {
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
