# datastructure_project
자구 프로젝트

# Agent Manager 코드 분석 문서

## 📑 목차
1. [전체 구조 개요](#전체-구조-개요)
2. [자료구조 상세 분석](#자료구조-상세-분석)
3. [핵심 함수 분석](#핵심-함수-분석)
4. [알고리즘 설명](#알고리즘-설명)
5. [메모리 관리](#메모리-관리)
6. [수정/확장 가이드](#수정확장-가이드)

---

## 전체 구조 개요

### 파일 구성
```
agent_manager/
├── agent_manager.h      # 모든 구조체, 상수, 함수 선언
├── agent_core.c         # 초기화, Agent/Project 생성/관리
├── agent_display.c      # 트리 시각화, 검색, 필터링
├── agent_cli.c          # 사용자 인터페이스 (CLI)
└── main.c               # 진입점, 데모 데이터
```

### 의존성 관계
```
main.c
  ↓
agent_cli.c ──→ agent_core.c
  ↓                ↓
agent_display.c ←─┘
  ↓
agent_manager.h (모두가 참조)
```

---

## 자료구조 상세 분석

### 1. AgentNode (트리 구조)

```c
typedef struct AgentNode {
    // 기본 정보
    char agent_id[128];        // 고유 식별자
    char nickname[128];        // 사용자 지정 이름
    char description[512];     // 설명
    AgentStatus status;        // 상태 (enum)
    
    // 네트워크 정보
    char ip_address[64];
    int port;
    char hostname[128];
    char username[128];
    char os_info[512];
    
    // 메타데이터
    time_t first_seen;         // 최초 발견 시각
    time_t last_seen;          // 마지막 활동 시각
    int beacon_interval;       // 비콘 간격 (초)
    
    // 연결 리스트들
    AgentTag* tags;            // 태그 목록 (연결 리스트)
    char log_file_path[256];
    LogEntry* log_head;        // 로그 항목 (연결 리스트)
    int log_count;
    ConsoleSession* console;   // 연결된 콘솔 세션
    
    // 트리 구조 포인터
    struct AgentNode* parent;       // 부모 노드
    struct AgentNode* first_child;  // 첫 번째 자식
    struct AgentNode* next_sibling; // 다음 형제
    
    bool is_project;           // 프로젝트 노드 여부
} AgentNode;
```

**핵심 포인트:**
- **트리 구조**: `parent`, `first_child`, `next_sibling` 포인터로 N-ary 트리 구현
- **is_project 플래그**: 같은 구조체로 프로젝트와 Agent 모두 표현
- **연결 리스트 포함**: 태그, 로그는 각각 별도의 연결 리스트로 관리

**메모리 구조 예시:**
```
ROOT
├─ first_child → RedTeam_Alpha (Project)
│   ├─ first_child → AGENT-001
│   │   ├─ next_sibling → AGENT-002
│   │   │   └─ next_sibling → AGENT-003
│   │   └─ tags → [critical] → [windows] → NULL
│   └─ next_sibling → BlueTeam_Beta (Project)
│       └─ first_child → AGENT-004
└─ next_sibling → NULL
```

### 2. AgentTag (연결 리스트)

```c
typedef struct AgentTag {
    char tag[64];              // 태그 문자열
    struct AgentTag* next;     // 다음 태그
} AgentTag;
```

**사용 예:**
```
agent->tags → "critical" → "windows" → "server" → NULL
```

**장점:**
- 동적으로 태그 추가/제거 가능
- 메모리 효율적 (필요한 만큼만 할당)

### 3. LogEntry (연결 리스트)

```c
typedef struct LogEntry {
    char timestamp[32];        // "2025-01-15 14:30:45"
    char message[512];         // 로그 메시지
    struct LogEntry* next;     // 다음 로그
} LogEntry;
```

**구조:**
```
agent->log_head → [최신 로그] → [이전 로그] → [더 이전 로그] → NULL
```

**특징:**
- 새 로그는 항상 맨 앞에 추가 (O(1))
- 최신 로그부터 순회 가능

### 4. ConsoleSession (연결 리스트)

```c
typedef struct ConsoleSession {
    int session_id;            // 고유 세션 ID
    char agent_id[128];        // 연결된 Agent ID
    time_t created_at;         // 생성 시각
    bool is_active;            // 활성 상태
    struct ConsoleSession* next;
} ConsoleSession;
```

**ConsoleManager:**
```c
typedef struct ConsoleManager {
    ConsoleSession* head;      // 세션 리스트 헤드
    int total_sessions;        // 총 세션 수
} ConsoleManager;
```

### 5. AgentManager (메인 관리 구조체)

```c
typedef struct AgentManager {
    AgentNode* root;           // 트리의 루트
    ConsoleManager* console_mgr;
    int total_agents;          // 전체 Agent 수
    int active_agents;         // 활성 Agent 수
} AgentManager;
```

---

## 핵심 함수 분석

### 1. 초기화 함수

#### `agent_manager_init()`
```c
AgentManager* agent_manager_init() {
    // 1. AgentManager 메모리 할당
    AgentManager* mgr = malloc(sizeof(AgentManager));
    
    // 2. 루트 노드 생성
    mgr->root = malloc(sizeof(AgentNode));
    strcpy(mgr->root->agent_id, "ROOT");
    mgr->root->is_project = true;
    mgr->root->parent = NULL;
    mgr->root->first_child = NULL;
    mgr->root->next_sibling = NULL;
    
    // 3. Console Manager 초기화
    mgr->console_mgr = malloc(sizeof(ConsoleManager));
    mgr->console_mgr->head = NULL;
    mgr->console_mgr->total_sessions = 0;
    
    // 4. 카운터 초기화
    mgr->total_agents = 0;
    mgr->active_agents = 0;
    
    return mgr;
}
```

**흐름도:**
```
[malloc AgentManager]
       ↓
[malloc root node]
       ↓
[initialize root fields]
       ↓
[malloc ConsoleManager]
       ↓
[initialize counters]
       ↓
[return manager]
```

### 2. 프로젝트 생성

#### `create_project()`
```c
AgentNode* create_project(AgentManager* mgr, const char* project_name, 
                          const char* description) {
    // 1. 새 노드 할당
    AgentNode* project = malloc(sizeof(AgentNode));
    memset(project, 0, sizeof(AgentNode));
    
    // 2. 기본 정보 설정
    strncpy(project->agent_id, project_name, MAX_NAME_LEN - 1);
    strncpy(project->nickname, project_name, MAX_NAME_LEN - 1);
    strncpy(project->description, description, MAX_DESC_LEN - 1);
    project->is_project = true;
    
    // 3. 루트의 자식으로 추가
    project->parent = mgr->root;
    
    if (!mgr->root->first_child) {
        // 첫 번째 자식인 경우
        mgr->root->first_child = project;
    } else {
        // 형제로 추가 (맨 끝에)
        AgentNode* sibling = mgr->root->first_child;
        while (sibling->next_sibling) {
            sibling = sibling->next_sibling;
        }
        sibling->next_sibling = project;
    }
    
    return project;
}
```

**시각화:**
```
Before:
ROOT
└─ first_child → Project1 → next_sibling → NULL

After:
ROOT
└─ first_child → Project1 → next_sibling → Project2 → NULL
```

### 3. Agent 추가

#### `add_agent()`
```c
AgentNode* add_agent(AgentManager* mgr, AgentNode* parent, 
                     const char* agent_id) {
    // 1. Agent 노드 생성
    AgentNode* agent = malloc(sizeof(AgentNode));
    memset(agent, 0, sizeof(AgentNode));
    
    // 2. 기본 설정
    strncpy(agent->agent_id, agent_id, MAX_NAME_LEN - 1);
    agent->is_project = false;
    agent->status = AGENT_ACTIVE;
    agent->first_seen = time(NULL);
    agent->last_seen = time(NULL);
    
    // 3. 로그 파일 경로 생성
    snprintf(agent->log_file_path, MAX_PATH_LEN, 
             "./logs/%s.log", agent_id);
    
    // 4. 부모의 자식으로 추가 (create_project와 동일한 로직)
    agent->parent = parent;
    if (!parent->first_child) {
        parent->first_child = agent;
    } else {
        AgentNode* sibling = parent->first_child;
        while (sibling->next_sibling) {
            sibling = sibling->next_sibling;
        }
        sibling->next_sibling = agent;
    }
    
    // 5. 카운터 증가 및 로그 기록
    mgr->total_agents++;
    mgr->active_agents++;
    add_log_entry(agent, "Agent registered");
    
    return agent;
}
```

### 4. 트리 탐색 (재귀)

#### `find_agent_recursive()`
```c
AgentNode* find_agent_recursive(AgentNode* node, const char* agent_id) {
    if (!node) return NULL;
    
    // 1. 현재 노드 확인 (프로젝트는 제외)
    if (!node->is_project && strcmp(node->agent_id, agent_id) == 0) {
        return node;
    }
    
    // 2. 모든 자식 노드 재귀 탐색
    AgentNode* child = node->first_child;
    while (child) {
        AgentNode* found = find_agent_recursive(child, agent_id);
        if (found) return found;
        child = child->next_sibling;  // 다음 형제로
    }
    
    return NULL;
}
```

**탐색 순서 (DFS):**
```
ROOT
├─ Project1
│  ├─ Agent1  ← 1번째 확인
│  └─ Agent2  ← 2번째 확인
└─ Project2
   ├─ Agent3  ← 3번째 확인
   └─ Agent4  ← 4번째 확인
```

### 5. 로그 관리

#### `add_log_entry()`
```c
void add_log_entry(AgentNode* agent, const char* message) {
    if (!agent) return;
    
    // 1. 새 로그 항목 생성
    LogEntry* entry = malloc(sizeof(LogEntry));
    
    // 2. 타임스탬프 및 메시지 설정
    strncpy(entry->timestamp, get_timestamp(), 
            sizeof(entry->timestamp) - 1);
    strncpy(entry->message, message, MAX_DESC_LEN - 1);
    
    // 3. 리스트 맨 앞에 추가 (최신 로그가 항상 앞)
    entry->next = agent->log_head;
    agent->log_head = entry;
    agent->log_count++;
}
```

**로그 리스트 구조:**
```
agent->log_head
    ↓
[2025-01-15 15:00:00] "Status changed" → next
    ↓
[2025-01-15 14:00:00] "Command sent" → next
    ↓
[2025-01-15 13:00:00] "Agent registered" → NULL
```

### 6. 태그 시스템

#### `add_agent_tag()`
```c
void add_agent_tag(AgentNode* agent, const char* tag) {
    // 1. 중복 체크
    AgentTag* current = agent->tags;
    while (current) {
        if (strcmp(current->tag, tag) == 0) {
            return;  // 이미 존재
        }
        current = current->next;
    }
    
    // 2. 새 태그 생성
    AgentTag* new_tag = malloc(sizeof(AgentTag));
    strncpy(new_tag->tag, tag, MAX_TAG_LEN - 1);
    
    // 3. 리스트 맨 앞에 추가
    new_tag->next = agent->tags;
    agent->tags = new_tag;
}
```

#### `remove_agent_tag()`
```c
void remove_agent_tag(AgentNode* agent, const char* tag) {
    AgentTag* current = agent->tags;
    AgentTag* prev = NULL;
    
    while (current) {
        if (strcmp(current->tag, tag) == 0) {
            // 찾았음
            if (prev) {
                prev->next = current->next;  // 중간 노드
            } else {
                agent->tags = current->next; // 첫 노드
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}
```

**삭제 시나리오:**
```
Before: [critical] → [windows] → [server] → NULL
                      ↑ 삭제 대상

After:  [critical] → [server] → NULL
```

### 7. 트리 시각화

#### `display_tree()` (재귀)
```c
void display_tree(AgentNode* node, int depth) {
    if (!node) return;
    
    // 1. 들여쓰기 출력
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    // 2. 노드 정보 출력
    if (node->is_project) {
        printf("📁 %s (%s)\n", node->nickname, node->description);
    } else {
        printf("🖥️  %s [%s] - %s", 
               node->nickname, 
               status_to_string(node->status),
               node->ip_address);
        
        // 태그 출력
        if (node->tags) {
            printf(" 🏷️ ");
            AgentTag* tag = node->tags;
            while (tag) {
                printf("[%s]", tag->tag);
                tag = tag->next;
            }
        }
        printf("\n");
    }
    
    // 3. 모든 자식 재귀 출력
    AgentNode* child = node->first_child;
    while (child) {
        display_tree(child, depth + 1);
        child = child->next_sibling;
    }
}
```

**출력 예:**
```
📁 ROOT
  📁 RedTeam_Alpha
    🖥️  DC-Primary [ACTIVE] - 192.168.1.10 🏷️ [critical][windows]
    🖥️  WebServer [ACTIVE] - 203.0.113.45 🏷️ [web][linux]
  📁 BlueTeam_Beta
    🖥️  Database [ACTIVE] - 192.168.1.100 🏷️ [database]
```

### 8. Console 세션 관리

#### `create_console_session()`
```c
ConsoleSession* create_console_session(AgentManager* mgr, 
                                        const char* agent_id) {
    // 1. 세션 생성
    ConsoleSession* session = malloc(sizeof(ConsoleSession));
    
    // 2. 고유 ID 부여 (자동 증가)
    session->session_id = ++mgr->console_mgr->total_sessions;
    strncpy(session->agent_id, agent_id, MAX_NAME_LEN - 1);
    session->created_at = time(NULL);
    session->is_active = true;
    
    // 3. 리스트 맨 앞에 추가
    session->next = mgr->console_mgr->head;
    mgr->console_mgr->head = session;
    
    return session;
}
```

**세션 리스트:**
```
console_mgr->head
    ↓
[Session 3: AGENT-003] → next
    ↓
[Session 2: AGENT-002] → next
    ↓
[Session 1: AGENT-001] → NULL
```

---

## 알고리즘 설명

### 1. 트리 순회 (DFS - Depth First Search)

**Pre-order 방식:**
1. 현재 노드 처리
2. 첫 번째 자식 방문
3. 다음 형제 방문

```c
void traverse(AgentNode* node) {
    if (!node) return;
    
    process(node);                    // 1. 현재 노드
    traverse(node->first_child);      // 2. 자식
    traverse(node->next_sibling);     // 3. 형제
}
```

**복잡도:**
- 시간: O(n) - 모든 노드 방문
- 공간: O(h) - 재귀 스택 (h = 트리 높이)

### 2. 필터링 알고리즘

#### 태그 필터링
```c
void filter_by_tag_recursive(AgentNode* node, const char* tag, bool* found) {
    if (!node) return;
    
    // Agent이고 태그가 있는 경우
    if (!node->is_project && has_tag(node, tag)) {
        print_agent(node);
        *found = true;
    }
    
    // 모든 자식/형제 확인
    filter_by_tag_recursive(node->first_child, tag, found);
    filter_by_tag_recursive(node->next_sibling, tag, found);
}
```

**복잡도:**
- 시간: O(n * m) where n=노드수, m=평균 태그 개수
- 공간: O(h)

### 3. 검색 알고리즘

```c
void search_agents_recursive(AgentNode* node, const char* keyword, 
                              bool* found) {
    if (!node) return;
    
    if (!node->is_project) {
        // 여러 필드에서 검색 (strstr 사용)
        if (strstr(node->agent_id, keyword) || 
            strstr(node->nickname, keyword) ||
            strstr(node->description, keyword) ||
            strstr(node->ip_address, keyword)) {
            print_agent(node);
            *found = true;
        }
    }
    
    // 재귀 탐색
    search_agents_recursive(node->first_child, keyword, found);
    search_agents_recursive(node->next_sibling, keyword, found);
}
```

**개선 가능:**
- Boyer-Moore 알고리즘으로 문자열 검색 최적화
- 해시 테이블로 O(1) 검색 구현
- Trie 구조로 prefix 검색

---

## 메모리 관리

### 1. 할당 시점

```
agent_manager_init()
    ├─ malloc(AgentManager)
    ├─ malloc(AgentNode) - ROOT
    └─ malloc(ConsoleManager)

create_project()
    └─ malloc(AgentNode) - Project

add_agent()
    └─ malloc(AgentNode) - Agent

add_agent_tag()
    └─ malloc(AgentTag)

add_log_entry()
    └─ malloc(LogEntry)

create_console_session()
    └─ malloc(ConsoleSession)
```

### 2. 해제 시점

```c
void free_node_recursive(AgentNode* node) {
    if (!node) return;
    
    // 1. 모든 자식 해제 (재귀)
    AgentNode* child = node->first_child;
    while (child) {
        AgentNode* next = child->next_sibling;
        free_node_recursive(child);
        child = next;
    }
    
    // 2. 태그 리스트 해제
    AgentTag* tag = node->tags;
    while (tag) {
        AgentTag* next = tag->next;
        free(tag);
        tag = next;
    }
    
    // 3. 로그 리스트 해제
    LogEntry* log = node->log_head;
    while (log) {
        LogEntry* next = log->next;
        free(log);
        log = next;
    }
    
    // 4. 노드 자체 해제
    free(node);
}
```

**해제 순서 (중요!):**
```
1. 자식 노드 (재귀)
2. 연결 리스트들 (태그, 로그)
3. 노드 자체
```

### 3. 메모리 누수 방지

**체크리스트:**
- [ ] 모든 `malloc()`에 대응하는 `free()` 존재
- [ ] 재귀 함수에서 모든 경로 해제
- [ ] 연결 리스트 순회 시 임시 포인터 사용
- [ ] 에러 처리 시 이미 할당된 메모리 해제

**예시: 에러 처리**
```c
AgentNode* create_project(...) {
    AgentNode* project = malloc(sizeof(AgentNode));
    if (!project) {
        return NULL;  // 할당 실패
    }
    
    // ... 초기화 ...
    
    // 만약 여기서 실패한다면?
    if (some_error) {
        free(project);  // 반드시 해제!
        return NULL;
    }
    
    return project;
}
```

---

## 수정/확장 가이드

### 1. 새로운 Agent 속성 추가

**단계:**
1. `agent_manager.h`의 `AgentNode` 구조체에 필드 추가
2. `add_agent()`에서 초기화
3. `display_agent_info()`에서 출력 추가
4. 필요시 setter 함수 작성

**예: Process ID 추가**
```c
// 1. 구조체 수정
typedef struct AgentNode {
    // ... 기존 필드 ...
    int process_id;  // 추가
} AgentNode;

// 2. 초기화
AgentNode* add_agent(...) {
    // ...
    agent->process_id = 0;  // 기본값
    // ...
}

// 3. 출력
void display_agent_info(AgentNode* agent) {
    // ...
    printf("Process ID:  %d\n", agent->process_id);
}

// 4. Setter
void set_agent_pid(AgentNode* agent, int pid) {
    if (!agent) return;
    agent->process_id = pid;
    
    char log_msg[MAX_DESC_LEN];
    snprintf(log_msg, sizeof(log_msg), "PID updated to %d", pid);
    add_log_entry(agent, log_msg);
}
```

### 2. 새로운 명령어 추가

**단계:**
1. `agent_cli.c`의 `display_menu()`에 명령어 추가
2. `process_command()`에 처리 로직 추가
3. 필요시 `agent_core.c`에 함수 구현
4. `agent_manager.h`에 함수 선언

**예: Agent 복제 기능**
```c
// 1. agent_core.c에 함수 구현
AgentNode* clone_agent(AgentManager* mgr, const char* src_id, 
                        const char* new_id) {
    AgentNode* src = find_agent(mgr->root, src_id);
    if (!src) return NULL;
    
    AgentNode* clone = add_agent(mgr, src->parent, new_id);
    
    // 정보 복사
    set_agent_nickname(clone, src->nickname);
    set_agent_description(clone, src->description);
    update_agent_info(clone, src->ip_address, src->port, 
                      src->hostname, src->username, src->os_info);
    
    // 태그 복사
    AgentTag* tag = src->tags;
    while (tag) {
        add_agent_tag(clone, tag->tag);
        tag = tag->next;
    }
    
    return clone;
}

// 2. agent_manager.h에 선언
AgentNode* clone_agent(AgentManager* mgr, const char* src_id, 
                        const char* new_id);

// 3. agent_cli.c에 명령어 추가
void process_command(AgentManager* mgr, const char* input) {
    // ...
    else if (strcmp(command, "clone") == 0) {
        if (strlen(arg1) == 0 || strlen(arg2) == 0) {
            printf("[-] Usage: clone <src_agent_id> <new_agent_id>\n");
            return;
        }
        AgentNode* clone = clone_agent(mgr, arg1, arg2);
        if (clone) {
            printf("[+] Agent cloned successfully\n");
        } else {
            printf("[-] Clone failed\n");
        }
    }
    // ...
}
```

### 3. 실제 Havoc IPC 연동

**현재 (시뮬레이션):**
```c
void havoc_send_command(AgentNode* agent, const char* command) {
    printf("[Havoc] Sending command: %s\n", command);
    add_log_entry(agent, "Command sent");
}
```

**실제 구현 (Named Pipe - Windows):**
```c
#ifdef _WIN32
#include <windows.h>

void havoc_send_command(AgentNode* agent, const char* command) {
    HANDLE hPipe;
    DWORD bytesWritten;
    
    // Havoc 파이프 연결
    hPipe = CreateFile(
        TEXT("\\\\.\\pipe\\HavocPipe"),
        GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL
    );
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to connect to Havoc\n");
        return;
    }
    
    // 명령 전송
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), 
             "{\"agent_id\":\"%s\",\"command\":\"%s\"}", 
             agent->agent_id, command);
    
    WriteFile(hPipe, buffer, strlen(buffer), &bytesWritten, NULL);
    CloseHandle(hPipe);
    
    add_log_entry(agent, "Command sent via IPC");
}
#endif
```

**Unix Domain Socket (Linux):**
```c
#ifdef __linux__
#include <sys/socket.h>
#include <sys/un.h>

void havoc_send_command(AgentNode* agent, const char* command) {
    int sockfd;
    struct sockaddr_un addr;
    
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/havoc.sock", 
            sizeof(addr.sun_path) - 1);
    
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return;
    }
    
    // JSON 형식으로 전송
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), 
             "{\"agent_id\":\"%s\",\"command\":\"%s\"}\n",
             agent->agent_id, command);
    
    send(sockfd, buffer, strlen(buffer), 0);
    close(sockfd);
    
    add_log_entry(agent, "Command sent via socket");
}
#endif
```

### 4. 데이터베이스 영구 저장

**SQLite 통합 예시:**
```c
#include <sqlite3.h>

void save_agent_to_db(AgentNode* agent) {
    sqlite3* db;
    sqlite3_stmt* stmt;
    
    sqlite3_open("agents.db", &db);
    
    const char* sql = 
        "INSERT OR REPLACE INTO agents "
        "(agent_id, nickname, description, ip, port, status) "
        "VALUES (?, ?, ?, ?, ?, ?)";
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, agent->agent_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, agent->nickname, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, agent->description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, agent->ip_address, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, agent->port);
    sqlite3_bind_int(stmt, 6, agent->status);
    
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

AgentNode* load_agent_from_db(const char* agent_id) {
    // 유사하게 SELECT로 로드
}
```

### 5. 멀티스레드 지원

**문제점:**
- 여러 스레드가 동시에 트리 수정 시 race condition

**해결책: Mutex 사용**
```c
#include <pthread.h>

typedef struct AgentManager {
    AgentNode* root;
    ConsoleManager* console_mgr;
    int total_agents;
    int active_agents;
    pthread_mutex_t lock;  // 추가!
} AgentManager;

// 초기화
AgentManager* agent_manager_init() {
    // ...
    pthread_mutex_init(&mgr->lock, NULL);
    return mgr;
}

// 사용
AgentNode* add_agent(...) {
    pthread_mutex_lock(&mgr->lock);
    
    // ... Agent 추가 로직 ...
    
    pthread_mutex_unlock(&mgr->lock);
    return agent;
}

// 정리
void agent_manager_free(AgentManager* mgr) {
    pthread_mutex_destroy(&mgr->lock);
    // ...
}
```

### 6. 성능 최적화

#### 현재 문제점
- `find_agent()`: O(n) - 모든 노드 순회
- 태그 검색: O(n*m) - 모든 Agent의 모든 태그 확인

#### 해시 테이블 추가
```c
#include <uthash.h>  // UTHASH 라이브러리

typedef struct AgentHash {
    char agent_id[128];
    AgentNode* node;
    UT_hash_handle hh;
} AgentHash;

typedef struct AgentManager {
    AgentNode* root;
    AgentHash* agent_index;  // 추가!
    // ...
} AgentManager;

// Agent 추가 시 해시 테이블에도 등록
AgentNode* add_agent(...) {
    // ... 기존 코드 ...
    
    AgentHash* entry = malloc(sizeof(AgentHash));
    strcpy(entry->agent_id, agent_id);
    entry->node = agent;
    HASH_ADD_STR(mgr->agent_index, agent_id, entry);
    
    return agent;
}

// O(1) 검색
AgentNode* find_agent_fast(AgentManager* mgr, const char* agent_id) {
    AgentHash* entry;
    HASH_FIND_STR(mgr->agent_index, agent_id, entry);
    return entry ? entry->node : NULL;
}
```

---

## 디버깅 팁

### 1. 메모리 누수 체크

**Valgrind 사용:**
```bash
valgrind --leak-check=full --show-leak-kinds=all \
         ./agent_manager --quick-demo
```

**예상 출력:**
```
HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
  total heap usage: 45 allocs, 45 frees
```

### 2. 로그 추가

```c
#ifdef DEBUG
#define LOG(fmt, ...) \
    fprintf(stderr, "[DEBUG %s:%d] " fmt "\n", \
            __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

// 사용
AgentNode* add_agent(...) {
    LOG("Adding agent: %s to project: %s", 
        agent_id, parent->agent_id);
    // ...
}
```

**컴파일:**
```bash
gcc -DDEBUG -g -o agent_manager *.c
```

### 3. Assert 사용

```c
#include <assert.h>

AgentNode* add_agent(...) {
    assert(mgr != NULL);
    assert(parent != NULL);
    assert(agent_id != NULL);
    
    // ...
}
```

### 4. GDB 디버깅

```bash
gdb ./agent_manager

(gdb) break add_agent
(gdb) run --demo
(gdb) print *agent
(gdb) print agent->parent->nickname
(gdb) backtrace
```

---

## 알려진 제한사항 및 개선점

### 현재 제한사항
1. **메모리만 저장** - 프로그램 종료 시 모든 데이터 손실
2. **단일 스레드** - 동시 접근 불가
3. **선형 검색** - 많은 Agent 시 느림
4. **IPC 미구현** - Havoc와 실제 통신 불가
5. **입력 검증 부족** - 잘못된 입력 시 segfault 가능

### 개선 방향
1. **SQLite/JSON 파일 저장**
2. **pthread mutex로 동기화**
3. **해시 테이블 인덱싱**
4. **실제 IPC/소켓 통신**
5. **입력 검증 강화**
6. **웹 UI (HTTP 서버)**
7. **플러그인 시스템**

---

## 코드 수정 체크리스트

수정 전 확인:
- [ ] 어떤 기능을 추가/수정할 것인가?
- [ ] 어떤 파일을 변경해야 하는가?
- [ ] 메모리 할당/해제가 필요한가?
- [ ] 기존 코드와 호환되는가?
- [ ] 헤더 파일에 선언을 추가했는가?

수정 후 확인:
- [ ] 컴파일 에러 없음
- [ ] Valgrind로 메모리 누수 체크
- [ ] 기존 기능 정상 작동
- [ ] 새 기능 테스트 완료
- [ ] 주석 및 문서 업데이트

---

## 참고 자료

- **C 프로그래밍**: K&R "The C Programming Language"
- **자료구조**: "Introduction to Algorithms" (CLRS)
- **Linux IPC**: `man 7 unix`, `man 7 pipe`
- **Havoc Framework**: https://github.com/HavocFramework/Havoc

이 문서를 참고하여 코드를 분석하고 필요에 따라 수정하시기 바랍니다!
