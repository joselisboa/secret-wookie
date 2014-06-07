#define FIFOPATH "/tmp/fifo"

int server_fifo, client_fifo;
struct request req;
struct response res;

void play(int sig);
void quit(int sig);
void print_response(struct response res);
void getname(char name[]);//, char *prompt[], char *format[]){
struct response send(struct request req);
void shutdown(int spid);
void restart(char proc[]);
void start(char proc[]);
int validate_cmd(char command[]);
int cleanup();

int playing = false;
int is_playing();
//-----------------------------------------------------------------------------
//                                                                     client.h
// TODO split - - - > - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                     client.c
//-----------------------------------------------------------------------------
// validates command
int validate_cmd(char command[]){
    char cmd[8], param1[32], param2[16];
    int i, k;

    // scan user commands
    if(!(k = sscanf(command, "%s %s %s", cmd, param1, param2))) return 0;
    //printf("k=%d, cmd=%s, param1=%s, param2=%s", k, cmd, param1, param2);

    // client commands
    for(i=0; i<C; i++)
        if(strcmp(C_CMDS[i], cmd) == 0) 
            break;

    switch(i){
        case 0://"login",
        case 1://"exit",
        case 2://"logout",
            return 1;

        case 3://"status",
            return 0;

        case 4://"users",
            return 1;

        case 5://"new",// new <nome> <s>
            if(k != 3){
                _printf(4, "'%s' requires additional paramaters\n", cmd);
                return -1;
            }

            if(atoi(param2) < 1){
                _puts("the second parameter must be a positive number", 4);
                return -1;
            }

            return 1;

        // play
        case 6://"play",//'play 99 left' or 'play 18 right'
            // client command
            if(k == 1) return 1;

            // player command
            if(k != 3) {
                _printf(4, "'%s' requires additional paramaters\n", cmd);
                return -1;
            }
            if(atoi(param1) > 28 || atoi(param1) < 1){
                _puts("the first parameter must be a number between 1 and 28", 4);
                return -1;
            }
            if(!(strcmp(param2, "left") == 0)
                    && !(strcmp(param2, "right") == 0)){
                _puts("the second parameter must be 'left' or 'right'", 4);
                return -1;
            }
            if(is_playing() == true) {
                _puts("wait", 8);
                return 1;
            }
            return -1;

        case 7://"quit",
            return 0;

        case 8://"start",
            start(SERVER);
            return -1;

        case 9:// shutdown
            shutdown(getzpid(SERVER));
            return -1;

        case 10:// restart
            restart(SERVER);
            return -1;

        case 11:// users
            return 1;

        default:

        // player commands
        for(i=0; i<P; i++) if(strcmp(P_CMDS[i], cmd) == 0) break;

        switch(i){
            case 0:// tiles
            case 1:// info 
            case 2:// game 
            case 4:// get
            case 5:// pass
            case 7:// giveup
            case 8:// hint
                return is_playing();

            case 6:// help
                _puts("type 'hint' for help on choosing a tile", 5);
                _puts("HELP", 8);
                return -1;

            // players (list)
            case 9: return 1;
            default: return 0;
        }
    }

    return 0;
}

// ----------------------------------------------------------------------------
// prompts player's name
void getname(char name[]){//, char *prompt[], char *format[]){
    char buffer[32];
    int i, r = 0;

    fflush(stdout);

    while(!r){
    BEGIN:
        _puts("your name:", 3);
        printf("> ");
        scanf(" %[^\n]", buffer);
        r = sscanf(buffer, " %s", name);
        
        if(strcmp(name, "exit")==0) break;

        // client commands
        for(i=0; i<C; i++) if(strcmp(C_CMDS[i], name) == 0) {
            _printf(4, "the name '%s' is reserved\n", name);
            r = 0;
            goto BEGIN;
        }

        // player commands
        for(i=0; i<P; i++) if(strcmp(P_CMDS[i], name) == 0){
            _printf(4, "the name '%s' is reserved\n", name);
            r=0;
            goto BEGIN;
        }
    }
}

//-----------------------------------------------------------------------------
// sends request to server
struct response send(struct request req){
    struct response res;

    // enviar dados ao servidor
    write(server_fifo, &req, sizeof(req));

    _puts("wait", 8);

    // abrir fifo privado em modo de leitura
    if((client_fifo = open(req.fifo, O_RDONLY)) < 0){
        perror(req.fifo);
        exit(-1);
    }
    // ler dados
    read(client_fifo, &res, sizeof(res));

    // fechar o fifo
    close(client_fifo);

    return res;
}

// ----------------------------------------------------------------------------
// SHUTDOWN shuts down server and exits
void shutdown(int spid){
    _printf(8, "kill -%d  %d [%d]\n", SIGUSR2, spid, kill(spid, SIGUSR2));
    //TODO check status?
    cleanup();
    //exit(1);
}

// ----------------------------------------------------------------------------
// RESTART
void restart(char proc[]){
    int pid = getzpid(proc);
    _printf(8, "%s is restarting\n", proc);
    shutdown(pid);
    sleep(1);
    start(SERVER);
}

// ----------------------------------------------------------------------------
// START (starts the server)
void start(char proc[]){
    _printf(8, "starting %s\nwait\n", proc);
    system(proc);
    sleep(1);
}

//-----------------------------------------------------------------------------
// PLAY (game play) SIGUSR1 handler
void play(int sig){
    struct move status;
    int len;

    // [5] abrir fifo privado em modo de leitura
    if((client_fifo = open(req.fifo, O_RDONLY)) < 0){
        perror(req.fifo);
        return;
    }

    // [6] ler dados
    read(client_fifo, &status, sizeof(status));
    // fechar o fifo do cliente
    close(client_fifo);

    // get game info
    if(status.move == 1) playing = true;
    else if(status.turn == -1) playing = false;

    puts(status.msg);
    printf("> ");
    fflush(stdout);
}

//-----------------------------------------------------------------------------
// QUIT closes client (SIGUSR2 handler)
void quit(int sig){
    int i = 4;
    _puts("\nthe server is shutting down", 3);
    _puts("client terminating now", 5);
    fflush(stdout);
    
    while(i > 1){
        _printf(3, "%d\n", --i);
        sleep(1);  
    }

    exit(cleanup());
}

//-----------------------------------------------------------------------------
int cleanup(){
    close(server_fifo);
    unlink(req.fifo);
    return 1;
}

//-----------------------------------------------------------------------------
// DEV
void print_response(struct response res){
    puts("res: {");
    printf("    pid: %d\n", res.pid);
    printf("    msg: \"%s\"\n", res.msg);
    puts("    req: {");
    printf("        pid = %d,\n", res.req.pid);
    printf("        name: \"%s\",\n", res.req.name);
    printf("        player_id: %d,\n", res.req.player_id);
    printf("        fifo: \"%s\",\n", res.req.fifo);
    printf("        cmd: \"%s\"\n", res.req.cmd);
    puts("    },");
    printf("    res: %d\n", res.cmd);
    puts("}");
}

int is_playing(){
    if(!playing) {
        _puts("you are not playing", 4);
        return -1;
    }
    return 1;
}