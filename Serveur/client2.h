#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

// Enum to represent client status
typedef enum {
    AVAILABLE,
    IN_GAME,
    SPECTATING,
    BUSY
} ClientStatus;

// Struct to represent a client
typedef struct {
    SOCKET sock;
    char name[BUF_SIZE];
    ClientStatus status; // Add client status
} Client;

#endif /* guard */
