#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#define LOG_DIR "logs"
#define MAX_INPUT 512

// Structure definitions
typedef struct Agent Agent;
typedef struct Listener Listener;
typedef struct Project Project;

typedef struct Project {
    char name[128];
    char description[512];
    struct Agent* C_agent;
    struct Project* next;
} Project;

typedef struct Agent {
    int session_id;
    char hostname[128];
    char username[128];
    char OS[256];
    char architecture[32];
    int privilege;
    time_t first_seen;
    time_t last_seen;
    char process[256];
    int pid;
    char label[128];
    char tags[256];
    char description[512];
    int status;
    struct Listener* listener;
    struct Agent* P_agent;
    struct Agent* N_agent;
    struct Agent* C_agent;
    char log_path[256];
} Agent;

typedef struct Listener {
    char name[128];
    char protocol[10];
    int ipv4[4];
    int port;
    char path[256];
    int status;
    time_t created_at;
    char log_path[256];
    struct Agent* child_agent;
} Listener;

// Global variables
Project* project_list = NULL;
Project* current_project = NULL;
Listener* listener_list = NULL;
int next_session_id = 1;

// Function declarations
void print_banner();
void print_help();
void create_log_directory();
void init_project();
void list_projects();
void switch_project(char* name);
void delete_project(char* name);
Project* find_project(char* name);
void create_listener();
void list_listeners();
void create_agent();
void list_agents(Agent* root, int depth);
void show_agent_info(int session_id);
void delete_agent(int session_id);
void write_log(int session_id, char* content);
void view_log(int session_id);
void cleanup();
Agent* find_agent(Agent* root, int session_id);
Listener* find_listener(char* name);

// Function definitions

void print_banner() {
    printf("\n=== Agent Manager v1.0 - C2 Framework Tool ===\n");
}

void print_help() {
    printf("\nCommands:\n");
    printf("  project init / project list / project switch <name> / project delete <name>\n");
    printf("  listener create / listener list\n");
    printf("  agent create / agent list / agent info <id> / agent delete <id>\n");
    printf("  log write <id> <text> / log view <id>\n");
    printf("  help / exit\n");
}

void create_log_directory() {
#ifdef _WIN32
    _mkdir(LOG_DIR);
#else
    struct stat st = { 0 };
    if (stat(LOG_DIR, &st) == -1) {
        mkdir(LOG_DIR, 0700);
    }
#endif
}

Project* find_project(char* name) {
    Project* temp = project_list;
    while (temp) {
        if (strcmp(temp->name, name) == 0) return temp;
        temp = temp->next;
    }
    return NULL;
}

void init_project() {
    Project* new_project = (Project*)malloc(sizeof(Project));

    printf("Project name: ");
    fgets(new_project->name, 128, stdin);
    new_project->name[strcspn(new_project->name, "\n")] = 0;

    if (find_project(new_project->name)) {
        printf("Project '%s' already exists.\n", new_project->name);
        free(new_project);
        return;
    }

    printf("Description: ");
    fgets(new_project->description, 512, stdin);
    new_project->description[strcspn(new_project->description, "\n")] = 0;

    new_project->C_agent = NULL;
    new_project->next = NULL;

    if (!project_list) {
        project_list = new_project;
    }
    else {
        Project* temp = project_list;
        while (temp->next) temp = temp->next;
        temp->next = new_project;
    }

    current_project = new_project;
    printf("Project '%s' created and activated.\n", new_project->name);
}

void list_projects() {
    if (!project_list) {
        printf("No projects available.\n");
        return;
    }

    printf("\n=== Projects ===\n");
    Project* temp = project_list;
    int count = 1;

    while (temp) {
        printf("%d. %s - %s %s\n",
            count++,
            temp->name,
            temp->description,
            (temp == current_project) ? "[ACTIVE]" : "");
        temp = temp->next;
    }
}

void switch_project(char* name) {
    Project* proj = find_project(name);
    if (!proj) {
        printf("Project '%s' not found.\n", name);
        return;
    }
    current_project = proj;
    printf("Switched to project '%s'.\n", name);
}

void delete_project(char* name) {
    if (!project_list) {
        printf("No projects available.\n");
        return;
    }

    Project* temp = project_list;
    Project* prev = NULL;

    while (temp) {
        if (strcmp(temp->name, name) == 0) {
            if (prev) {
                prev->next = temp->next;
            }
            else {
                project_list = temp->next;
            }

            if (current_project == temp) {
                current_project = project_list;
                if (current_project) {
                    printf("Switched to project '%s'.\n", current_project->name);
                }
                else {
                    printf("No projects remaining.\n");
                }
            }

            free(temp);
            printf("Project '%s' deleted.\n", name);
            return;
        }
        prev = temp;
        temp = temp->next;
    }

    printf("Project '%s' not found.\n", name);
}

void create_listener() {
    Listener* new_listener = (Listener*)malloc(sizeof(Listener));

    printf("Listener name: ");
    fgets(new_listener->name, 128, stdin);
    new_listener->name[strcspn(new_listener->name, "\n")] = 0;

    printf("Protocol (http/https/smb): ");
    fgets(new_listener->protocol, 10, stdin);
    new_listener->protocol[strcspn(new_listener->protocol, "\n")] = 0;

    printf("IP (e.g., 192.168.1.100): ");
    scanf("%d.%d.%d.%d", &new_listener->ipv4[0], &new_listener->ipv4[1],
        &new_listener->ipv4[2], &new_listener->ipv4[3]);
    getchar();

    printf("Port: ");
    scanf("%d", &new_listener->port);
    getchar();

    printf("Path: ");
    fgets(new_listener->path, 256, stdin);
    new_listener->path[strcspn(new_listener->path, "\n")] = 0;

    new_listener->status = 1;
    new_listener->created_at = time(NULL);
    new_listener->child_agent = NULL;

    snprintf(new_listener->log_path, 256, "%s/listener_%s.log", LOG_DIR, new_listener->name);

    FILE* fp = fopen(new_listener->log_path, "a");
    if (fp) {
        char time_str[26];
        struct tm* tm_info = localtime(&new_listener->created_at);
        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(fp, "[%s] Listener created - %s://%d.%d.%d.%d:%d%s\n",
            time_str, new_listener->protocol,
            new_listener->ipv4[0], new_listener->ipv4[1],
            new_listener->ipv4[2], new_listener->ipv4[3],
            new_listener->port, new_listener->path);
        fclose(fp);
    }

    if (!listener_list) {
        listener_list = new_listener;
    }
    else {
        Listener* temp = listener_list;
        while (temp->child_agent) temp = (Listener*)temp->child_agent;
        temp->child_agent = (Agent*)new_listener;
    }

    printf("Listener '%s' created.\n", new_listener->name);
}

void list_listeners() {
    if (listener_list == NULL) {
        printf("No listeners available.\n");
        return;
    }

    printf("\n=== Listeners ===\n");
    Listener* temp = listener_list;
    int count = 1;

    while (temp != NULL) {
        printf("%d. %s [%s] - %d.%d.%d.%d:%d%s - Status: %s\n",
            count++, temp->name, temp->protocol,
            temp->ipv4[0], temp->ipv4[1], temp->ipv4[2], temp->ipv4[3],
            temp->port, temp->path,
            temp->status ? "Running" : "Stopped");
        temp = (Listener*)temp->child_agent;
    }
}

Agent* find_agent(Agent* root, int session_id) {
    if (root == NULL) return NULL;

    if (root->session_id == session_id) return root;

    Agent* result = find_agent(root->C_agent, session_id);
    if (result) return result;

    return find_agent(root->N_agent, session_id);
}

void create_agent() {
    if (!current_project) {
        printf("Initialize project first (project init).\n");
        return;
    }

    Agent* new_agent = (Agent*)malloc(sizeof(Agent));
    new_agent->session_id = next_session_id++;

    printf("Hostname: ");
    fgets(new_agent->hostname, 128, stdin);
    new_agent->hostname[strcspn(new_agent->hostname, "\n")] = 0;

    printf("Username: ");
    fgets(new_agent->username, 128, stdin);
    new_agent->username[strcspn(new_agent->username, "\n")] = 0;

    printf("OS: ");
    fgets(new_agent->OS, 256, stdin);
    new_agent->OS[strcspn(new_agent->OS, "\n")] = 0;

    printf("Architecture: ");
    fgets(new_agent->architecture, 32, stdin);
    new_agent->architecture[strcspn(new_agent->architecture, "\n")] = 0;

    printf("Privilege (0=user, 1=admin): ");
    scanf("%d", &new_agent->privilege);
    getchar();

    printf("Process: ");
    fgets(new_agent->process, 256, stdin);
    new_agent->process[strcspn(new_agent->process, "\n")] = 0;

    printf("PID: ");
    scanf("%d", &new_agent->pid);
    getchar();

    printf("Label: ");
    fgets(new_agent->label, 128, stdin);
    new_agent->label[strcspn(new_agent->label, "\n")] = 0;

    printf("Tags: ");
    fgets(new_agent->tags, 256, stdin);
    new_agent->tags[strcspn(new_agent->tags, "\n")] = 0;

    printf("Description: ");
    fgets(new_agent->description, 512, stdin);
    new_agent->description[strcspn(new_agent->description, "\n")] = 0;

    printf("Parent agent ID (0 for root): ");
    int parent_id;
    scanf("%d", &parent_id);
    getchar();

    new_agent->first_seen = time(NULL);
    new_agent->last_seen = new_agent->first_seen;
    new_agent->status = 1;
    new_agent->listener = NULL;
    new_agent->P_agent = NULL;
    new_agent->N_agent = NULL;
    new_agent->C_agent = NULL;

    snprintf(new_agent->log_path, 256, "%s/agent_%d.log", LOG_DIR, new_agent->session_id);
    FILE* fp = fopen(new_agent->log_path, "a");
    if (fp) {
        char time_str[26];
        struct tm* tm_info = localtime(&new_agent->first_seen);
        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(fp, "[%s] Agent created - %s@%s (%s)\n",
            time_str, new_agent->username, new_agent->hostname, new_agent->OS);
        fclose(fp);
    }

    if (parent_id == 0) {
        if (!current_project->C_agent) {
            current_project->C_agent = new_agent;
        }
        else {
            Agent* temp = current_project->C_agent;
            while (temp->N_agent) temp = temp->N_agent;
            temp->N_agent = new_agent;
        }
    }
    else {
        Agent* parent = find_agent(current_project->C_agent, parent_id);
        if (parent) {
            new_agent->P_agent = parent;
            if (!parent->C_agent) {
                parent->C_agent = new_agent;
            }
            else {
                Agent* temp = parent->C_agent;
                while (temp->N_agent) temp = temp->N_agent;
                temp->N_agent = new_agent;
            }
        }
        else {
            printf("Parent not found. Adding as root.\n");
            if (!current_project->C_agent) {
                current_project->C_agent = new_agent;
            }
            else {
                Agent* temp = current_project->C_agent;
                while (temp->N_agent) temp = temp->N_agent;
                temp->N_agent = new_agent;
            }
        }
    }

    printf("Agent created. Session ID: %d\n", new_agent->session_id);
}

void list_agents(Agent* root, int depth) {
    if (root == NULL) return;

    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("├─ [%d] %s@%s (%s) - %s\n",
        root->session_id, root->username, root->hostname, root->OS,
        root->status ? "Active" : "Inactive");

    if (strlen(root->label) > 0) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf("   Label: %s\n", root->label);
    }

    if (root->C_agent) {
        list_agents(root->C_agent, depth + 1);
    }

    if (root->N_agent) {
        list_agents(root->N_agent, depth);
    }
}

void show_agent_info(int session_id) {
    Agent* agent = find_agent(current_project ? current_project->C_agent : NULL, session_id);

    if (agent == NULL) {
        printf("Agent with session ID %d not found.\n", session_id);
        return;
    }

    char time_str1[26], time_str2[26];
    struct tm* tm_info;

    printf("\n=== Agent Information ===\n");
    printf("Session ID: %d\n", agent->session_id);
    printf("Hostname: %s\n", agent->hostname);
    printf("Username: %s\n", agent->username);
    printf("OS: %s\n", agent->OS);
    printf("Architecture: %s\n", agent->architecture);
    printf("Privilege: %s\n", agent->privilege ? "Admin" : "User");
    printf("Process: %s (PID: %d)\n", agent->process, agent->pid);
    printf("Status: %s\n", agent->status ? "Active" : "Inactive");

    tm_info = localtime(&agent->first_seen);
    strftime(time_str1, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("First Seen: %s\n", time_str1);

    tm_info = localtime(&agent->last_seen);
    strftime(time_str2, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("Last Seen: %s\n", time_str2);

    if (strlen(agent->label) > 0)
        printf("Label: %s\n", agent->label);
    if (strlen(agent->tags) > 0)
        printf("Tags: %s\n", agent->tags);
    if (strlen(agent->description) > 0)
        printf("Description: %s\n", agent->description);

    printf("Log File: %s\n", agent->log_path);
}

void delete_agent(int session_id) {
    Agent* agent = find_agent(current_project ? current_project->C_agent : NULL, session_id);
    if (agent) {
        agent->status = 0;
        time_t now = time(NULL);
        FILE* fp = fopen(agent->log_path, "a");
        if (fp) {
            char time_str[26];
            struct tm* tm_info = localtime(&now);
            strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
            fprintf(fp, "[%s] Agent deleted\n", time_str);
            fclose(fp);
        }
        printf("Agent %d marked as inactive.\n", session_id);
    }
    else {
        printf("Agent not found.\n");
    }
}

void write_log(int session_id, char* content) {
    Agent* agent = find_agent(current_project ? current_project->C_agent : NULL, session_id);
    if (!agent) {
        printf("Agent not found.\n");
        return;
    }
    FILE* fp = fopen(agent->log_path, "a");
    if (fp) {
        time_t now = time(NULL);
        char time_str[26];
        struct tm* tm_info = localtime(&now);
        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(fp, "[%s] %s\n", time_str, content);
        fclose(fp);
        printf("Log added.\n");
    }
    else {
        printf("Failed to open log.\n");
    }
}

void view_log(int session_id) {
    Agent* agent = find_agent(current_project ? current_project->C_agent : NULL, session_id);
    if (!agent) {
        printf("Agent not found.\n");
        return;
    }
    FILE* fp = fopen(agent->log_path, "r");
    if (fp) {
        printf("\n=== Log for Agent %d ===\n", session_id);
        char line[512];
        while (fgets(line, 512, fp)) printf("%s", line);
        fclose(fp);
    }
    else {
        printf("Log not found.\n");
    }
}

Listener* find_listener(char* name) {
    Listener* temp = listener_list;
    while (temp != NULL) {
        if (strcmp(temp->name, name) == 0) return temp;
        temp = (Listener*)temp->child_agent;
    }
    return NULL;
}

void cleanup() {
    printf("Cleaning up resources...\n");
    if (current_project) {
        free(current_project);
    }
}

int main() {
    char command[MAX_INPUT];

    create_log_directory();
    print_banner();
    print_help();

    while (1) {
        printf("\n[Agent Manager]> ");
        if (fgets(command, MAX_INPUT, stdin) == NULL) break;

        command[strcspn(command, "\n")] = 0;

        if (strlen(command) == 0) continue;

        if (strcmp(command, "help") == 0) {
            print_help();
        }
        else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            printf("Exiting Agent Manager...\n");
            break;
        }
        else if (strncmp(command, "project init", 12) == 0) {
            init_project();
        }
        else if (strcmp(command, "project list") == 0) {
            list_projects();
        }
        else if (strncmp(command, "project switch", 14) == 0) {
            char name[128];
            if (sscanf(command, "project switch %[^\n]", name) == 1) {
                switch_project(name);
            }
            else {
                printf("Usage: project switch <name>\n");
            }
        }
        else if (strncmp(command, "project delete", 14) == 0) {
            char name[128];
            if (sscanf(command, "project delete %[^\n]", name) == 1) {
                delete_project(name);
            }
            else {
                printf("Usage: project delete <name>\n");
            }
        }
        else if (strncmp(command, "listener create", 15) == 0) {
            create_listener();
        }
        else if (strcmp(command, "listener list") == 0) {
            list_listeners();
        }
        else if (strncmp(command, "agent create", 12) == 0) {
            create_agent();
        }
        else if (strcmp(command, "agent list") == 0) {
            if (current_project && current_project->C_agent) {
                printf("\n=== Project: %s ===\n", current_project->name);
                list_agents(current_project->C_agent, 0);
            }
            else {
                printf("No project initialized or no agents available.\n");
            }
        }
        else if (strncmp(command, "agent info", 10) == 0) {
            int sid;
            if (sscanf(command, "agent info %d", &sid) == 1) {
                show_agent_info(sid);
            }
            else {
                printf("Usage: agent info <session_id>\n");
            }
        }
        else if (strncmp(command, "agent delete", 12) == 0) {
            int sid;
            if (sscanf(command, "agent delete %d", &sid) == 1) {
                delete_agent(sid);
            }
            else {
                printf("Usage: agent delete <session_id>\n");
            }
        }
        else if (strncmp(command, "log write", 9) == 0) {
            int sid;
            char content[MAX_INPUT];
            if (sscanf(command, "log write %d %[^\n]", &sid, content) == 2) {
                write_log(sid, content);
            }
            else {
                printf("Usage: log write <session_id> <content>\n");
            }
        }
        else if (strncmp(command, "log view", 8) == 0) {
            int sid;
            if (sscanf(command, "log view %d", &sid) == 1) {
                view_log(sid);
            }
            else {
                printf("Usage: log view <session_id>\n");
            }
        }
        else {
            printf("Unknown command: %s\n", command);
            printf("Type 'help' for available commands.\n");
        }
    }

    cleanup();
    return 0;
}
