#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

const char *DOMINOS = "/tmp/DOMINOS";

#define NUMPECAS 7
#define P 8
#define C 12

// comandos do jogador (ao cliente)
char *P_CMDS[P] = {
    "tiles",
    "info",
    "game",
    "play",//'play 99 left' or 'play 18 right'
    "get",
    "pass",
    "help",
    "giveup"
};

// comandos do cliente (ao servidor)
char *C_CMDS[C] = {
    "login",
    "exit",
    "logout",
    "status",
    "users",
    "new",// new <nome> <s>
    "play",
    "quit",
    "start",
    "shutdown",
    "restart",
    "games"
};

struct request {
	int pid;
	int player_id;
    char name[32];
	char fifo[32];
	char cmd[16];
};

struct response {
	int pid;
	char msg[512];
	struct request req;
	int cmd;// 0:msg, 1:quit, -1:error
};

int send_signal(int pid, int SIG){
    int ret;
    ret = kill(pid, SIG);
    //printf("ret : %d", ret);
    return ret;
}
