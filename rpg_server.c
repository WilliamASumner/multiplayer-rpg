#include "common.h"
#ifdef _REENTRANT
#include <pthread.h>
pthread_mutex_t playersMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t npcsMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t threadMut = PTHREAD_MUTEX_INITIALIZER;


pthread_t tids[NUMPLAYERS+1];
int connfds[NUMPLAYERS];
int shuttingDown = 0;
#endif

struct player npcs[NUMNPCS];
struct player players[NUMPLAYERS];

int didStart = 0;
int playersConnected = 0;
int sfd;

int diceRoll(int max)
{
    return rand()%max+1;
}

void createNPC(struct player* p, struct player* npc, int type)
{
#ifdef _REENTRANT
    pthread_mutex_lock(&npcsMut);
#endif
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
#ifdef _REENTRANT
    pthread_mutex_unlock(&npcsMut);
#endif

}

void generateNPCs(struct player* p, struct player* npcs, int num)
{
    createNPC(p, &npcs[0], SAURON); // create SAURON
    createNPC(p, &npcs[num-1], GOLLUM); // create GOLLUM
    int i;
    for (i = 1 ;i < num-1; i++)
       createNPC(p, &npcs[i],i); // CREATE ORCS
}

void resetPlayer(int connfd,struct player* p)
{
    char buff[200];
    struct data_header outHeader;
    createHeader(&outHeader,ROLL,200);
    sendHeader(connfd,&outHeader);
    sprintf(buff,"Respawning %s...\n",p->name);
    sendCharBuffer(connfd,buff,200);

#ifdef _REENTRANT
    pthread_mutex_lock(&playersMut);
#endif
    p->hp = 20 + (p->lvl - 1) * 5;
    p->xp = 2000*(1 << (p->lvl - 1));
#ifdef _REENTRANT
    pthread_mutex_unlock(&playersMut);
#endif

    createHeader(&outHeader,STATS,2);
    sendHeader(connfd,&outHeader);
    sendPlayer(connfd,p);
}

void attackPlayer(int connfd, struct player* p, struct player* enemy)
{
    char buff[200];
    struct data_header outHeader;
#ifdef _REENTRANT
    pthread_mutex_lock(&playersMut);
    pthread_mutex_lock(&npcsMut);
#endif
    int attack = diceRoll(20); // attack roll
    if (attack >= acs[enemy->arm]) // if greater than AC
    {
        int j, damageRoll=0; // set up damage roll
        for (j = 0;j < rol[p->wep];j++)
            damageRoll += diceRoll(dmg[p->wep] - 1);
        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"%s hits %s for %d damage ",p->name,enemy->name,damageRoll);
        sendCharBuffer(connfd,buff,200);
        enemy->hp -= damageRoll; // subtract the health
    }
    else
    {
        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"%s misses %s ",p->name,enemy->name);
        sendCharBuffer(connfd,buff,200);
    }
    createHeader(&outHeader,ROLL,200);
    sendHeader(connfd,&outHeader);
    sprintf(buff,"(attack roll %d)\n",attack);
    sendCharBuffer(connfd,buff,200);
#ifdef _REENTRANT
    pthread_mutex_unlock(&npcsMut);
    pthread_mutex_unlock(&playersMut);
#endif
}


void lootNPC(int connfd, struct player* p, struct player* enemy)
{
    char c;
    struct data_header outHeader;
    struct data_header inHeader;
    char buff[200];

    createHeader(&outHeader,LOOT,200);
    sendHeader(connfd,&outHeader);
    sprintf(buff,"Exchange %s's %s for %s's %s (y/n)? ",enemy->name,armors[enemy->arm],p->name,armors[p->arm]);
    sendCharBuffer(connfd,buff,200);

    readHeader(connfd, &inHeader);
    if (inHeader.length == 1)
    {
#ifdef _REENTRANT
        pthread_mutex_lock(&playersMut);
#endif
        p->arm = enemy->arm;
#ifdef _REENTRANT
        pthread_mutex_unlock(&playersMut);
#endif
        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"%s picked up %s.\n",p->name,armors[p->arm]);
        sendCharBuffer(connfd,buff,200);
    }
    else
    {
        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"%s left the %s behind.\n",p->name,armors[enemy->arm]);
        sendCharBuffer(connfd,buff,200);
    }


    createHeader(&outHeader,LOOT,200);
    sendHeader(connfd,&outHeader);
    sprintf(buff,"Exchange %s's %s for %s's %s (y/n)? ",enemy->name,weapons[enemy->wep],p->name,weapons[p->wep]);
    sendCharBuffer(connfd,buff,200);

    readHeader(connfd,&inHeader);
    if (inHeader.length == 1)
    {
#ifdef _REENTRANT
        pthread_mutex_lock(&playersMut);
#endif
        p->wep = enemy->wep;
#ifdef _REENTRANT
        pthread_mutex_unlock(&playersMut);
#endif
        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"%s picked up %s.\n",p->name,weapons[p->wep]);
        sendCharBuffer(connfd,buff,200);
    }
    else
    {
        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"%s left the %s behind.\n",p->name,weapons[enemy->wep]);
        sendCharBuffer(connfd,buff,200);
    }
    createHeader(&outHeader,STATS,2);
    sendHeader(connfd,&outHeader);
    sendPlayer(connfd,p);
}


void fightPlayer(int connfd, struct player* p, struct player* enemy) // returns 0 for loss, 1 for win and 2 for tie
{
    struct data_header outHeader;
    char buff[200];
        printf("attacking in progress...\n");
    do {
        attackPlayer(connfd, p,enemy);
        attackPlayer(connfd, enemy,p);
    } while (p->hp > 0 && enemy->hp > 0);
    printf("attacking done\n");

    if (p->hp <= 0 && enemy->hp <= 0)
    {
        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"\n%s and %s have died.\n\n",p->name,enemy->name);
        sendCharBuffer(connfd,buff,200);

        resetPlayer(connfd,p); // respawn player
        createNPC(p,enemy,enemy->type); // recreate the npc
        createHeader(&outHeader,STATS,1);
        sendHeader(connfd,&outHeader);
        sendPlayer(connfd,enemy);
    }
    else if (p->hp > 0) // player killed enemy
    {
        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"%s killed %s.\n",p->name,enemy->name);
        sendCharBuffer(connfd,buff,200);

#ifdef _REENTRANT
        pthread_mutex_lock(&playersMut);
#endif
        p->xp += enemy->lvl * 2000;
        while (p->xp >= 2000 *( 1 << p->lvl))
        {
            p->lvl += 1; // LEVEL UP!
            createHeader(&outHeader,ROLL,200);
            sendHeader(connfd,&outHeader);
            sprintf(buff,"%s leveled up to level %d!\n",p->name,p->lvl);
            sendCharBuffer(connfd,buff,200);
        }
#ifdef _REENTRANT
        pthread_mutex_unlock(&playersMut);
#endif
        lootNPC(connfd, p,enemy); // loop npc
        createNPC(p,enemy,enemy->type); // recreate the npc

        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"Respawning %s...\n",enemy->name);
        sendCharBuffer(connfd,buff,200);

        createHeader(&outHeader,STATS,1);
        sendHeader(connfd,&outHeader);
        sendPlayer(connfd,enemy);
    }
    else // player killed by enemy
    {
        createHeader(&outHeader,ROLL,200);
        sendHeader(connfd,&outHeader);
        sprintf(buff,"%s killed %s...\n",enemy->name, p->name);
        sendCharBuffer(connfd,buff,200);

        printf("respawning player\n");
        resetPlayer(connfd,p); // respawn player
    }

    createHeader(&outHeader,DONE,0);
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
void quit()
{
    printf("Quitting...\n");
}

int playerID(const char* buff)
{
    int i;
    for (i = 0; i < NUMPLAYERS; i++)
    {
        if (strncmp(buff,(const char*)&players[i].name,100) == 0)
            return i;
    }
    return -1;
}

int getNewPlayerID()
{
    int i;
    for (i=0;i< NUMPLAYERS;i++)
        if (strcmp("empty",(const char*)&players[i].name) == 0)
            return i;
    return -1;
}

#ifdef _REENTRANT
int getThreadID(pthread_t tid)
{
    int i;
    for (i = 0; i < NUMPLAYERS+1;i++)
        if (pthread_equal(tid,tids[i]))
            return i;
    return -1;
}
#endif

void sigtermint_handler(int sig)
{
    printf("\ninterrupt detected, cleaning up\n");
    signal(SIGINT,sigtermint_handler);
    signal(SIGTERM,sigtermint_handler);
#ifdef _REENTRANT
    int threadID = getThreadID(pthread_self());
    struct data_header outHeader;
    if (shuttingDown == 0)
    {
        printf("Waiting for threads to end\n");
        int i;
        int tidReturn;
        shuttingDown = 1;
        pthread_mutex_lock(&threadMut);
        for (i = 1; i < NUMPLAYERS+1;i++)
        {
            if (tids[i] != NULL && tids[i] != pthread_self())
            {
                if((tidReturn = pthread_kill(tids[i],SIGINT)) == 0)
                    printf("pthread killed");
                else if (tidReturn == ESRCH)
                    printf("thread %lld could not be found\n",(long long)tids[i]);
            }
        }
        for (i = 0; i < NUMPLAYERS;i++) // send a shutdown on each connfd
        {
            createHeader(&outHeader,QUIT,1);
            sendHeader(connfds[i],&outHeader);
            close(connfds[i]);
            connfds[i] = (int)NULL;
        }
    }
    else
    {
        tids[threadID] = NULL;
        pthread_exit(0);
    }
#endif
    close(sfd);
    exit(0);
}

void *thread_func(void * arg)
{
    int connfd = *(int *)arg;

#ifdef _REENTRANT
    int threadID = getThreadID(pthread_self());
    connfds[threadID] = connfd;
    tids[threadID] = pthread_self();
#endif

    // START OF MULTITHREADING

    int numNPCs = NUMNPCS;
    struct data_header inHeader; // send a start message

    struct data_header outHeader; // send a start message
    createHeader(&outHeader,START,0);
    sendHeader(connfd,&outHeader);

    readHeader(connfd,&inHeader);
    if (inHeader.responseType != START || inHeader.length > 100 || inHeader.length <= 0)
    {
        printf("Error starting\n");
        close(connfd);
        return 0;
    }

    char nameBuffer[100];
    readCharBuffer(connfd,nameBuffer,inHeader.length);
    struct player somePlayer;
    strncpy(somePlayer.name,nameBuffer,100);
    printf("recieved name '%s'\n",somePlayer.name);

    int currPlayerID = playerID(somePlayer.name);

    if (playerID(nameBuffer) == -1) // if player not found
    {
        createHeader(&outHeader, CREATE, 0); // send a create a player message
        sendHeader(connfd,&outHeader); // send create signal
        readHeader(connfd,&inHeader);
        if (inHeader.responseType != CREATE)
        {
            printf("ERROR BAD HEADER, exiting\n");
            return 0;
        }
        currPlayerID = getNewPlayerID();
        printf("new player id of %d\n",currPlayerID);
        readPlayer(connfd,&players[currPlayerID]); // reading in created player
#ifdef _REENTRANT
        pthread_mutex_lock(&playersMut);
        playersConnected++;
        pthread_mutex_unlock(&playersMut);
#else
        playersConnected++;
#endif
    }
    else
        printf("current player id of %d\n",currPlayerID);

    struct player* currPlayer = &players[currPlayerID];
    printPlayer(currPlayer);

    if (didStart == 0)
    {
        generateNPCs(&players[currPlayerID], npcs,numNPCs);
        didStart = 1;
    }
    createHeader(&outHeader,LOOK,playersConnected+NUMNPCS); // send starting message
    sendHeader(connfd,&outHeader);
    sendPlayerBuffer(connfd,npcs,NUMNPCS);
    sendPlayerBuffer(connfd,players,playersConnected);

    int playerToFight = 0;
    int exitLoop = 0;
    while (exitLoop != 1) // enter command loop
    {
        readHeader(connfd,&inHeader); // keep reading commands
        switch(inHeader.responseType)
        {
            case FIGHT:
                playerToFight = inHeader.length;
                printf("Fighting player...\n");
                createHeader(&outHeader,FIGHT,0);
                sendHeader(connfd,&outHeader);
                fightPlayer(connfd,currPlayer,&npcs[playerToFight]);
                createHeader(&outHeader,DONE,0);
                sendHeader(connfd,&outHeader);
                break;
            case LOOK:
                printf("Looking...\n");
                int i;
                createHeader(&outHeader,LOOK,NUMNPCS+playersConnected);
                sendHeader(connfd,&outHeader);
                sendPlayerBuffer(connfd,npcs,NUMNPCS);
                sendPlayerBuffer(connfd,players,playersConnected);
                break;
            case STATS:
                printf("Getting stats...\n");
                createHeader(&outHeader,STATS,1);
                sendHeader(connfd,&outHeader);
                sendPlayer(connfd,currPlayer); // get player's stats
                break;
            case ENDSERVER:
                createHeader(&outHeader,ENDSERVER,1);
                sendHeader(connfd,&outHeader);
#ifdef _REENTRANT
                pthread_kill(pthread_self(),SIGINT); // interrupt all threads
#else
                kill(getpid(),SIGINT);
#endif
            case QUIT:
                createHeader(&outHeader,QUIT,0);
                sendHeader(connfd,&outHeader);
#ifdef _REENTRANT
                tids[threadID] = NULL;
#endif
                close(connfd);
                exitLoop = 1;
                break;
            default:
                createHeader(&outHeader,ERROR,0); // let the user know that the input was wrong
                sendHeader(connfd,&outHeader);

        }
    }
#ifdef _REENTRANT
    pthread_exit(0);
#else
    return NULL;
#endif
}

int main (int argc, char** argv)
{
    signal(SIGTERM,sigtermint_handler);
    signal(SIGINT,sigtermint_handler);
    srand((unsigned int)time(NULL)); // seed
    int numNPCs = NUMNPCS;
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
    sfd = -1; // socket file descriptor
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

    // ### BINDING
    bind(sfd, (struct sockaddr *)&addr, sizeof(addr));

    if (listen(sfd,10) == -1)
    {
        printf("Error listening on port %d\n",portNum);
        return 0;
    }

    // SETTING UP GAME
    printf("Initializing player list...\n");
    struct player empty = {"empty",0,0,0,0,0,0};
    int i;
    for (i = 0; i < NUMPLAYERS; i++)
        copyPlayer(&empty,&players[i]);

    // SETTING UP THREADING
#ifdef _REENTRANT
    tids[NUMPLAYERS-1] = pthread_self();
#endif
    while(1)
    {
        printf("Server started, waiting for connection on port %d..\n",portNum);
        int connectDescriptor = accept(sfd, NULL, NULL); // wait for a connection fd
#ifdef _REENTRANT
        pthread_create(&tids[playersConnected],NULL,thread_func,&connectDescriptor);
#else
        thread_func(&connectDescriptor);
#endif
    }
    return 0;
}
