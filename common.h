#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PLAYER 0 // types of characters
#define GOLLUM -2
#define SAURON -3

#define NUMNPCS 10 // number of npcs (including sauron and gollum)
#define NUMPLAYERS 10 // max number of players

#define START  0 // commands
#define QUIT  1 // commands
#define STATS 2 // show player stats, could be an npc
#define LOOK  3 // list npcs
#define FIGHT 4 // fight npc
#define EQUIP 5 // list equipment
#define ROLL 6 // for rolls
#define CREATE 7 // for new players
#define ERROR 8 // for new players
#define LOOT 9 // for looting NPCS
#define DONE 10 // for looting NPCS
#define ENDSERVER 11 // for looting NPCS
#define INVALID -1 // invalid input


struct player {
    char name[100]; // name
    int hp; // health points
    int arm; // armor index
    int wep; // weapon index
    int lvl; // level
    int xp; // current xp
    int type; //player or npc type
};

extern const char *armors[5];
const char *weapons[5];
extern const int acs[5]; // armor classes
extern const int dmg[5]; // weapon damages
extern const int rol[5]; // attack rolls per weapon

struct data_header {
    int responseType;
    int length;
};

void createHeader(struct data_header* header, int type, int length);
// SENDING AND RECIEVING FUNCTIONS

void readBytes(int sock, void* buffer, unsigned int numBytes);
void readHeader(int sock, struct data_header* h);
void readPlayer(int sock, struct player* p);
void readCharBuffer(int sock, char * b, int length);
void readPlayerBuffer(int sock, struct player * players, int numPlayers);

void sendHeader(int sock, struct data_header* h);
void sendPlayer(int sock, struct player* p);
void sendCharBuffer(int sock, char * b, int length);
void sendPlayerBuffer(int sock, struct player * players, int numPlayers);

void copyPlayer(struct player* src, struct player* dst);
void printPlayer(struct player* p);
/* COMMAND:Response Types
 * PLAYERLOOKUP: boolean isFound
 * QUIT: n/a
 * STATS: player struct
 * LOOK: player struct[]
 * FIGHT: boolean couldFight, int number of rounds, int[] rolls
 * GETLOOT: player struct
 */
#endif
