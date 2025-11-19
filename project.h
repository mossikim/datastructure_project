#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Project{
    //이름, 설명 구현해야됨

    struct Agent* C_agent;
} Project;

typedef struct Agent{
    int session_id;
    char hostname[128]="";
    char username[128]="";
    char OS[256]="";
    char architecture[32]="";
    int privilege;
    time_t first_seen;
    time_t last_seen;
    char process[256]="";
    int pid;

    int status;

    struct Listener* listener;

    struct Agent* P_agent;
    struct Agent* N_agent;
    struct Agent* C_agent;
} Agent;

typedef struct Listener{
    char name[128]="";
    char protocol[10]="";
    int ipv4[4]={0,0,0,0};
    int port=0;
    char path[256]="";
    int status;
    time_t created_at;

    char log_path[256]="";

    struct Agent* child_agent;
} Listener;
