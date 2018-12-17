#include "common.h"

struct player players[NUMPLAYERS];
struct player npcs[NUMNPCS];
int sfd = -1; // socket file descriptor

int diceRoll(int max)
{
    return rand()%max+1;
}

void printNPCs(struct player* npcs, int num)
{
    int i = 0;
    for (i = 0; i < num;i++)
    {
        printf("%d: ",i);
        printPlayer(&npcs[i]);
    }
}

void createNPC(struct player* p, struct player* npc, int type)
{
    switch(type)
    {
        case GOLLUM:
            sprintf(npc->name,"Gollum");
            npc->arm = 0; // get armor
            npc->wep = 0; // get weapon
            npc->lvl = 1;
            break;
        case SAURON:
            sprintf(npc->name,"Sauron");
            npc->arm = 4; // get armor
            npc->wep = 4; // get weapon
            npc->lvl = 20;
            break;
        case PLAYER:
            return;
        default:
            sprintf(npc->name,"Orc %d",type);
            npc->arm = diceRoll(5)-1; // get armor
            npc->wep = diceRoll(5)-1; // get weapon
            npc->lvl = diceRoll(p->lvl);
    }
    npc->type = type;
    npc->xp = 2000*(1 << (npc->lvl - 1));
    if (type == GOLLUM)
        npc->hp = 10;
    else
        npc->hp = 20 + (npc->lvl - 1) * 5;

}

void generateNPCs(struct player* p, struct player* npcs, int num)
{
    createNPC(p, &npcs[0], SAURON); // create SAURON
    createNPC(p, &npcs[num-1], GOLLUM); // create GOLLUM
    int i;
    for (i = 1 ;i < num-1; i++)
        createNPC(p, &npcs[i],i); // CREATE ORCS
}

void createPlayer(struct player* p)
{

    int i = 0;
    printf("List of available armors:\n");
    for ( i=0; i < 5; i++)
        printf("%d: %s (AC=%d)\n",i,armors[i],acs[i]);

    int readSuccess = 0;
    char buff[100];
    do { // read in armor
        printf("Choose Player One Armor (0-4): ");
        fgets(buff,100,stdin);
        readSuccess = sscanf(buff,"%d",&p->arm);
    } while (readSuccess != 1 || p->arm < 0 || p->arm > 4);

    printf("\nList of available weapons:\n");
    for ( i=0; i < 5; i++)
        printf("%d: %s (%dd=%d)\n",i,weapons[i],rol[i],dmg[i]);

    do { // read in weapon
        printf("Choose Player One Weapon (0-4): ");
        fgets(buff,100,stdin);
        readSuccess = sscanf(buff,"%d",&p->wep);
    } while (readSuccess != 1 || p->wep < 0 || p->wep > 4);

    p->type = PLAYER;
    p->lvl = 1;
    p->xp = 2000*(1 << (p->lvl - 1));
    p->hp = 20 + (p->lvl - 1) * 5;
    printf("\nPlayer setting complete:\n");
}
void resetPlayer(struct player* p)
{
    printf("Respawning %s...\n",p->name);
    p->hp = 20 + (p->lvl - 1) * 5;
    p->xp = 2000*(1 << (p->lvl - 1));
    printPlayer(p);
}

void attackPlayer(struct player* p, struct player* enemy)
{
    int attack = diceRoll(20); // attack roll
    if (attack >= acs[enemy->arm]) // if greater than AC
    {
        int j, damageRoll=0; // set up damage roll
        for (j = 0;j < rol[p->wep];j++)
            damageRoll += diceRoll(dmg[p->wep] - 1);
        printf("%s hits %s for %d damage ",p->name,enemy->name,damageRoll);
        enemy->hp -= damageRoll; // subtract the health
    }
    else
        printf("%s misses %s ",p->name,enemy->name);
    printf("(attack roll %d)\n",attack);
}


void lootNPC(struct player* p, struct player* enemy)
{
    char buff[100];
    char c;

    printf("Exchange %s's %s for %s's %s (y/n)? ",enemy->name,armors[enemy->arm],p->name,armors[p->arm]);
    fgets(buff,100,stdin);
    int readSuccess = sscanf(buff,"%c",&c);
    while(readSuccess != 1 || (c != 'y' && c != 'n' && c != 'Y' && c != 'n'))
    {
        printf("Error, enter (y/n)\n");
        printf("Exchange %s's %s for %s's %s (y/n)? ",enemy->name,armors[enemy->arm],p->name,armors[p->arm]);
        fgets(buff,100,stdin);
        readSuccess = sscanf(buff,"%c",&c);
    }
    if (c == 'y' || c == 'Y')
    {
        p->arm = enemy->arm;
        printf("%s picked up %s.\n",p->name,armors[p->arm]);
    }
    else
        printf("%s left the %s behind.\n",p->name,armors[enemy->arm]);

    printf("Exchange %s's %s for %s's %s (y/n)? ",enemy->name,weapons[enemy->wep],p->name,weapons[p->wep]);
    fgets(buff,100,stdin);
    readSuccess = sscanf(buff,"%c",&c);
    while(readSuccess != 1 || (c != 'y' && c != 'n' && c != 'Y' && c != 'n'))
    {
        printf("Error, enter (y/n)\n");
        printf("Exchange %s's %s for %s's %s (y/n)? ",enemy->name,weapons[enemy->wep],p->name,weapons[p->wep]);
        fgets(buff,100,stdin);
        readSuccess = sscanf(buff,"%c",&c);
    }
    if (c == 'y' || c == 'Y')
    {
        p->wep = enemy->wep;
        printf("%s picked up %s.\n",p->name,weapons[p->wep]);
    }
    else
        printf("%s left the %s behind.\n",p->name,weapons[enemy->wep]);
    printPlayer(p);

}


void fightPlayer(struct player* p, struct player* enemy) // returns 0 for loss, 1 for win and 2 for tie
{
    do {
        attackPlayer(p,enemy);
        attackPlayer(enemy,p);
    } while (p->hp > 0 && enemy->hp > 0);
    printf("\n");
    if (p->hp < 0 && enemy->hp < 0)
    {
        printf("%s and %s have died.\n\n",p->name,enemy->name);
        resetPlayer(p); // respawn player
        createNPC(p,enemy,enemy->type); // recreate the npc
    }
    else if (p->hp > 0) // player killed enemy
    {
        printf("%s killed %s.\n",p->name,enemy->name);
        p->xp += enemy->lvl * 2000;
        while (p->xp >= 2000 *( 1 << p->lvl))
        {
            p->lvl += 1; // LEVEL UP!
            printf("%s leveled up to level %d!\n",p->name,p->lvl);
        }
        lootNPC(p,enemy);
        createNPC(p,enemy,enemy->type); // recreate the npc
        printf("Respawning %s...\n",enemy->name);
        printPlayer(enemy);
    }
    else // player killed by enemy
    {
        resetPlayer(p); // respawn player
    }
}

void damageNPCs(struct player* p, struct player* npcs, int numNPCs)
{
    int i;
    for (i = 0; i < numNPCs; i++)
    {
        printf("%s suffers -20 damage ",npcs[i].name);
        npcs[i].hp -= 20;
        if (npcs[i].hp <= 0)
        {
            printf(" and dies. Respawning...\n");
            createNPC(p,&npcs[i],npcs[i].type);
            printPlayer(&npcs[i]);
        }
        else printf(" but survives.\n");
    }
    printf("%s suffers -20 damage",p->name);
    p->hp -= 20;
    if (p->hp <= 0)
    {
        printf(" and dies. Respawning...\n");
        resetPlayer(p);
    }
    else printf(", but survives.\n");
}

void listEquipment()
{
    int i;
    printf("Armors:\n");
    for ( i=0; i < 5; i++)
        printf("%d: %s (AC=%d)\n",i,armors[i],acs[i]);

    printf("\nWeapons:\n");
    for ( i=0; i < 5; i++)
        printf("%d: %s (%dd=%d)\n",i,weapons[i],rol[i],dmg[i]);
}

void quit(int code)
{
    switch(code)
    {
        case 0:
            printf("Quitting...\n");
            break;
        case 1:
            printf("Server shutdown...\n");
            break;
        default:
            printf("Quitting due to error\n");
    }
}

int comm(char *s)
{

    if (strcmp(s,"quit") == 0 || strcmp(s,"q") == 0)
        return QUIT;
    else if ( strcmp(s,"look") == 0 || strcmp(s,"l") == 0)
        return LOOK;
    else if (strcmp(s,"stats") == 0 || strcmp(s,"s") == 0)
        return STATS;
    else if (strcmp(s,"fight") == 0 || strcmp(s,"f") == 0)
        return FIGHT;
    else if (strcmp(s,"equipment") == 0 || strcmp(s,"eq") == 0)
        return EQUIP;
    else if (strcmp(s,"endserver") == 0 || strcmp(s,"es") == 0)
        return ENDSERVER;
    return INVALID;
}

void getCommand(char *command, int *playerToFight)
{
    int readSuccess = -1;
    char buff[100];
    do {
        printf("command >> ");
        fgets(buff,100,stdin);
        readSuccess = sscanf(buff,"%s %d\n",command,playerToFight);
    } while (readSuccess < 1);

    while (comm(command) == INVALID || (comm(command) == FIGHT && (readSuccess != 2 || *playerToFight < 0 || *playerToFight >= NUMNPCS)))
    {
        if (comm(command) == FIGHT && readSuccess == 2 && (*playerToFight < 0 || *playerToFight >= NUMNPCS))
            printf("invalid player ''%d'', try again\n",*playerToFight);
        else
            printf("invalid command ''%s'', try again\n",command);
        do {
            printf("command >> ");
            fgets(buff,100,stdin);
            readSuccess = sscanf(buff,"%s %d",command,playerToFight);
        } while (readSuccess < 1);
    }
}

void sigtermint_handler(int sig)
{
    printf("\ninterrupt detected, cleaning up\n");
    if (sfd != -1)
    {
        struct data_header header;
        createHeader(&header,QUIT,0);
        sendHeader(sfd,&header);
        close(sfd);
    }
    exit(0);
}

int main (int argc, char **argv)
{
    signal(SIGINT, sigtermint_handler);
    signal(SIGTERM, sigtermint_handler);
    if (argc != 2)
    {
        printf("Usage: ./rpg_client port-num\n");
        return 0;
    }
    int portNum = atoi(argv[1]);
    if (portNum < 0)
    {
        printf("Usage: ./rpg_client port-num\n");
        printf("Please use a valid portnumber.\n");
        return 0;
    }

    // #### SETTING UP NETWORKING
    struct sockaddr_in addr;
    sfd = socket(PF_INET,SOCK_STREAM,0);
    if (sfd == -1)
    {
        printf("Error creating socket\n");
        return 0;
    }
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portNum); // my range is 52260 <= x <= 52279
    addr.sin_addr.s_addr = INADDR_ANY; // automatically find IP

    // ### CONNECTING
    printf("Connecting to server...\n");
    if(connect(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Connection failed.\n");
        return 0;
    }
    int connfd = sfd; // as client connection = socket

    struct data_header inHeader;
    readHeader(sfd,&inHeader); // basically wait for start message
    if (inHeader.responseType != START)
    {
        printf("Error with connection\n");
        close(connfd);
        return 0;
    }

    struct data_header outHeader;
    struct player currPlayer;
    char nameBuff[100];
    printf("Please enter your name: ");
    fgets(nameBuff,100,stdin);
    sscanf(nameBuff,"%[^\n]\n",currPlayer.name); // read in the whole line

    createHeader(&outHeader,START,strnlen(nameBuff,99)+1);
    sendHeader(connfd,&outHeader);
    sendCharBuffer(connfd,currPlayer.name,strnlen(nameBuff,99)+1);

    readHeader(sfd,&inHeader); // read in response
    if (inHeader.responseType == CREATE)
    {
        printf("Player not found in our database\n");
        createPlayer(&currPlayer);
        createHeader(&outHeader,CREATE,0);
        sendHeader(connfd,&outHeader);
        sendPlayer(connfd,&currPlayer);
        readHeader(connfd,&inHeader);
    }
    else
        printf("'%s' user found.\n",currPlayer.name);

    if (inHeader.responseType != LOOK)
    {
        printf("response type is %d\n",inHeader.responseType);
        printf("ERROR reading in NPCS, exiting\n");
        return 0;
    }

    int numPlayers = inHeader.length-NUMNPCS;
    readPlayerBuffer(connfd,npcs,NUMNPCS);
    readPlayerBuffer(connfd,players,numPlayers);

    printf("The following options are available:\n\
1) f[ight] enemy-number : fight an enemy\n\
2) l[ook] : see the list of current enemies and adventurers\n\
3) s[tats] : see the list of current enemies and adventurers\n\
4) q[uit] : quit the game (progress will be saved as long as the server is running)\n");

    /* CLIENT
     * Does all of the requests, look, fight, stats, and listens
     */
    printf("\nAll is peaceful in the land of Mordor.\nSauron and his minions are blissfully going about their business:\n");
    printNPCs(npcs,NUMNPCS);
    printf("\nAlso at the scene are some adventurers looking for trouble:\n");
    printNPCs(players,numPlayers);

    char command[100];
    int playerToFight = -1;
    int readSuccess = 0;
    while (1)
    {
        getCommand(command,&playerToFight);
        createHeader(&outHeader,comm(command),playerToFight);
        sendHeader(connfd,&outHeader);
        readHeader(connfd,&inHeader);
        switch(inHeader.responseType)
        {
            case FIGHT:
                readHeader(connfd,&inHeader); // read in the next header
                char buff[200];
                struct player somePlayer;
                while (inHeader.responseType != DONE)
                {
                    switch (inHeader.responseType)
                    {
                        case ROLL:
                            readCharBuffer(connfd,buff,200);
                            printf("%s",buff);
                            break;
                        case STATS:
                            readPlayer(connfd,&somePlayer);
                            if (inHeader.length == 2) // if it's a player update
                                copyPlayer(&somePlayer,&currPlayer);
                            printPlayer(&somePlayer);
                            break;
                        case LOOT:
                            readCharBuffer(connfd,buff,200);
                            printf("%s",buff);
                            char c;
                            char inBuff[20];
                            fgets(inBuff,20,stdin);
                            int readSuccess = sscanf(inBuff,"%c",&c);
                            while(readSuccess != 1 ||
                                    (c != 'y' && c != 'n' &&
                                     c != 'Y' && c != 'n'))
                            {
                                printf("Error, enter (y/n)\n");
                                printf("%s",buff);
                                fgets(inBuff,100,stdin);
                                readSuccess = sscanf(inBuff,"%c",&c);
                            }
                            if (c == 'y' || c == 'Y')
                            {
                                createHeader(&outHeader,LOOT,1);
                                sendHeader(connfd,&outHeader);
                            }
                            else
                            {
                                createHeader(&outHeader,LOOT,0);
                                sendHeader(connfd,&outHeader);
                            }
                            break;
                        case QUIT:
                            quit(inHeader.length);
                            close(connfd);
                            return 0;
                        default:
                        case ERROR:
                            printf("ERROR INTERPRETING COMMAND\n");
                            close(connfd);
                            return -1;
                            break;
                    }
                    readHeader(connfd,&inHeader); // read in the next header
                }
                break;
            case LOOK:
                numPlayers = inHeader.length-NUMNPCS;
                readPlayerBuffer(connfd,npcs,NUMNPCS);
                readPlayerBuffer(connfd,players,numPlayers);
                printf("\n\nAll is peaceful in the land of Mordor.\nSauron and his minions are blissfully going about their business:\n");
                printNPCs(npcs,NUMNPCS);
                printf("\nAlso at the scene are some adventurers looking for trouble:\n");
                printNPCs(players,numPlayers);
                break;
            case STATS:
                readPlayer(connfd, &somePlayer);
                printPlayer(&somePlayer);
                break;
            case EQUIP:
                listEquipment();
                break;
            case ENDSERVER:
                printf("Server ended...\n");
            case QUIT:
                quit(inHeader.length);
                close(connfd);
                return 0;
            case ERROR:
                printf("Error executing command, try again\n");
                break;
            default:
                return 0;
        }
    }
    return 0;
}
