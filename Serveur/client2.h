#ifndef CLIENT2_H
#define CLIENT2_H

#define BUF_SIZE 1024

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
    SOCKET sock;                 // Client's socket
    char name[BUF_SIZE];         // Client's name
    ClientStatus status;         // Client's status (AVAILABLE, IN_GAME, etc.)
    int is_active;               // 1 if active, 0 if not

    // Existing fields for challenge system
    int is_challenged;           // 0: no challenge, 1: being challenged
    char challenger[BUF_SIZE];   // Name of the player who challenged the client
} Client;

#endif // CLIENT2_H
