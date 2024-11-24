#ifndef CLIENT2_H
#define CLIENT2_H

#define BUF_SIZE 1024
#define MAX_FRIENDS 100
#define MAX_BIO_LINES 10
#define MAX_LINE_LENGTH 256


#include "server2.h"

// Define the possible client statuses
typedef enum {
    AVAILABLE,   // Client is available for a game
    IN_GAME,     // Client is in an active game
    CHALLENGED ,  // Client is waiting for a challenge response
    SPECTATING ,  // Client is spectating a game
    BUSY         // Client is busy 
} ClientStatus;

// Define the Client structure
typedef struct Client {
    SOCKET sock;                  // Client's socket
    char name[BUF_SIZE];          // Client's name
    ClientStatus status;          // Client's status (AVAILABLE, IN_GAME, etc.)
    int is_active;                // 1 if active, 0 if not

    // Elo rating fields
    int elo_rating; 
    
     // Bio field
    char bio[MAX_BIO_LINES][MAX_LINE_LENGTH];
    int bio_line_count;

    // Spectating fields
    int is_spectating; // 0: not spectating, 1: spectating
    int spectate_mode; // 0: open to all, 1: friends only
    struct GameProcess *spectating_game; // Pointer to the game being spectated

    // Friend system fields
    int is_friend_requested;
    char friend_requester[BUF_SIZE];
    char **friends;       // Pointer to array of friend names
    int friend_count;

    // New fields for challenge system
    int is_challenged;            // 0: no challenge, 1: being challenged
    char challenger[BUF_SIZE];    // Name of the player who challenged the client
} Client;

#endif // CLIENT2_H
