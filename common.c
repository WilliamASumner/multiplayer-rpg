#include "common.h"

void createHeader(struct data_header* header, int type, int length)
{
    header->responseType = type;
    header->length = length;
}
void readBytes(int sock, void* buffer, unsigned int numBytes)
{
    int bytes = 0;
    int hold = 0;
    while (bytes < numBytes)
    {
        hold = read(sock,buffer+bytes,numBytes-bytes);
        if (hold < -1)
            printf("Error reading from socket: %s\n",strerror(errno));
        bytes += hold;
    }
}


void readHeader(int sock, struct data_header* h)
{
    readBytes(sock,h,sizeof(struct data_header));
}

void readPlayer(int sock, struct player* p)
{
    readBytes(sock,p,sizeof(struct player));
}

void readCharBuffer(int sock, char * b, int length)
{
    readBytes(sock,b,length);
}

void readPlayerBuffer(int sock, struct player * players, int numPlayers)
{
    readBytes(sock,players,sizeof(struct player)*numPlayers);
}

void sendHeader(int sock, struct data_header* h)
{
    send(sock,h,sizeof(struct data_header),0);
}

void sendPlayer(int sock, struct player* p)
{
    send(sock,p,sizeof(struct player),0);
}

void sendCharBuffer(int sock, char* b, int length)
{
    send(sock,b,length,0);
}

void sendPlayerBuffer(int sock, struct player * players, int numPlayers)
{
    send(sock,players,sizeof(struct player)*numPlayers,0);
}

void copyPlayer(struct player* src, struct player* dst)
{
    strncpy(dst->name,(const char*)src->name,100); // name
    dst->hp = src->hp; // health points
    dst->arm = src->arm; // armor index
    dst->wep = src->wep; // weapon index
    dst->lvl = src->lvl; // level
    dst->xp = src->xp; // current xp
    dst->type = src->type; //player or npc type
}

void printPlayer(struct player* p)
{
    printf("[%s: hp=%d, armor=%s, weapon=%s, level=%d, xp=%d]\n",p->name, p->hp,armors[p->arm], weapons[p->wep],p->lvl,p->xp);
}

const char *armors[5] = {"Cloth", "Studded Leather","Ring Mail", "Chain Mail", "Plate"}; // armor and weapon lists
const char *weapons[5] = {"Dagger", "Short Sword","Long Sword", "Great Sword", "Great Axe"};
const int acs[5] = {10,12,14,16,18}; // armor classes
const int dmg[5] = {4,6,8,6,12}; // weapon damages
const int rol[5] = {1,1,1,2,1}; // attack rolls per weapon
