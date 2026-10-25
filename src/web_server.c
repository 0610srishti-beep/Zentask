/* Small single-process HTTP server for the ZenTask web interface.
 * It uses the project's existing task structures and CSV functions. */
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSESOCK closesocket
#define SOCK_ERR INVALID_SOCKET
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
typedef int socket_t;
#define CLOSESOCK close
#define SOCK_ERR (-1)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "task.h"
#include "linked_list.h"
#include "hash_table.h"
#include "priority_heap.h"
#include "file_io.h"

#define PORT 8080
#define REQ_CAP 65536
#define BODY_CAP 262144

static int send_all(socket_t s, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int n = send(s, buf + sent, (int)(len - sent), 0);
#else
        ssize_t n = send(s, buf + sent, len - sent, 0);
#endif
        if (n <= 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}

static void respond(socket_t s, int status, const char *type, const char *body) {
    char headbuf[512];
    const char *label = status == 200 ? "OK" : status == 201 ? "Created" : status == 400 ? "Bad Request" : status == 404 ? "Not Found" : status == 405 ? "Method Not Allowed" : "Internal Server Error";
    size_t bodylen = strlen(body);
    int n = snprintf(headbuf, sizeof headbuf,
        "HTTP/1.1 %d %s\r\nContent-Type: %s; charset=utf-8\r\nContent-Length: %lu\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\n\r\n",
        status, label, type, (unsigned long)bodylen);
    send_all(s, headbuf, (size_t)n);
    send_all(s, body, bodylen);
}

static void json_string(char *out, size_t cap, const char *in) {
    size_t j = 0;
    if (cap < 3) { if (cap) out[0] = 0; return; }
    out[j++] = '"';
    for (size_t i = 0; in && in[i] && j + 7 < cap; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '"' || c == '\\') { out[j++] = '\\'; out[j++] = (char)c; }
        else if (c == '\n') { out[j++] = '\\'; out[j++] = 'n'; }
        else if (c == '\r') { out[j++] = '\\'; out[j++] = 'r'; }
        else if (c == '\t') { out[j++] = '\\'; out[j++] = 't'; }
        else if (c >= 32) out[j++] = (char)c;
    }
    out[j++] = '"'; out[j] = 0;
}

static void task_json(char *out, size_t cap, Task *t) {
    char title[MAX_TITLE * 2 + 8], desc[MAX_DESC * 2 + 8], due[MAX_DATE * 2 + 8];
    json_string(title, sizeof title, t->title);
    json_string(desc, sizeof desc, t->desc);
    json_string(due, sizeof due, t->due);
    snprintf(out, cap, "{\"id\":%d,\"title\":%s,\"description\":%s,\"priority\":%d,\"due\":%s}", t->id, title, desc, t->priority, due);
}

static void tasks_response(socket_t s) {
    char body[BODY_CAP]; size_t used = 0;
    used += (size_t)snprintf(body + used, sizeof body - used, "[ ");
    for (Task *t = head; t && used < sizeof body - 1200; t = t->next) {
        char item[1800]; task_json(item, sizeof item, t);
        used += (size_t)snprintf(body + used, sizeof body - used, "%s%s", used > 2 ? "," : "", item);
    }
    snprintf(body + used, sizeof body - used, "]");
    respond(s, 200, "application/json", body);
}

static char *json_value(const char *body, const char *key) {
    static char pattern[100];
    snprintf(pattern, sizeof pattern, "\"%s\"", key);
    const char *p = strstr(body, pattern);
    if (!p) return NULL;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return NULL;
    p++; while (*p && isspace((unsigned char)*p)) p++;
    return (char *)p;
}

static int json_get_string(const char *body, const char *key, char *out, size_t cap) {
    char *p = json_value(body, key); size_t j = 0;
    if (!p || *p != '"' || cap == 0) return 0;
    p++;
    while (*p && *p != '"' && j + 1 < cap) {
        if (*p == '\\' && p[1]) {
            p++;
            if (*p == 'n') out[j++] = '\n';
            else if (*p == 'r') out[j++] = '\r';
            else if (*p == 't') out[j++] = '\t';
            else out[j++] = *p;
            p++;
        } else out[j++] = *p++;
    }
    out[j] = 0;
    return *p == '"';
}

static int json_get_int(const char *body, const char *key, int *value) {
    char *p = json_value(body, key); char *end;
    long v;
    if (!p) return 0;
    v = strtol(p, &end, 10);
    if (end == p || v < 0 || v > 1000000) return 0;
    *value = (int)v; return 1;
}

static int parse_id(const char *path) {
    const char *p = strstr(path, "/api/tasks/");
    if (!p) return -1;
    p += strlen("/api/tasks/");
    if (!isdigit((unsigned char)*p)) return -1;
    return atoi(p);
}

static void api_create(socket_t s, const char *body) {
    char title[MAX_TITLE], desc[MAX_DESC] = {0}, due[MAX_DATE] = {0}; int priority = 3;
    if (!json_get_string(body, "title", title, sizeof title) || !title[0]) { respond(s, 400, "application/json", "{\"error\":\"A task title is required.\"}"); return; }
    json_get_string(body, "description", desc, sizeof desc);
    json_get_string(body, "due", due, sizeof due);
    json_get_int(body, "priority", &priority);
    Task *t = create_task(title, desc, priority, due);
    list_insert_tail(t); hash_insert(t); heap_insert(t); save_tasks(DATA_FILE);
    char result[2200]; task_json(result, sizeof result, t); respond(s, 201, "application/json", result);
}

static void api_update(socket_t s, int id, const char *body) {
    Task *t = hash_find(id); int priority;
    if (!t) { respond(s, 404, "application/json", "{\"error\":\"Task not found.\"}"); return; }
    char value[MAX_DESC];
    if (json_get_string(body, "title", value, sizeof value) && value[0]) { strncpy(t->title, value, MAX_TITLE - 1); t->title[MAX_TITLE - 1] = 0; }
    if (json_get_string(body, "description", value, sizeof value)) { strncpy(t->desc, value, MAX_DESC - 1); t->desc[MAX_DESC - 1] = 0; }
    if (json_get_string(body, "due", value, sizeof value)) { strncpy(t->due, value, MAX_DATE - 1); t->due[MAX_DATE - 1] = 0; }
    if (json_get_int(body, "priority", &priority) && priority != t->priority) { heap_remove(t); t->priority = priority; heap_insert(t); }
    save_tasks(DATA_FILE); char result[2200]; task_json(result, sizeof result, t); respond(s, 200, "application/json", result);
}

static void api_delete(socket_t s, int id) {
    Task *t = hash_find(id);
    if (!t) { respond(s, 404, "application/json", "{\"error\":\"Task not found.\"}"); return; }
    list_remove(t); hash_remove(t); heap_remove(t); free(t); save_tasks(DATA_FILE);
    respond(s, 200, "application/json", "{\"ok\":true}");
}

static void serve_static(socket_t s, const char *path) {
    const char *name = "web/index.html", *type = "text/html";
    if (strcmp(path, "/styles.css") == 0) { name = "web/styles.css"; type = "text/css"; }
    else if (strcmp(path, "/app.js") == 0) { name = "web/app.js"; type = "application/javascript"; }
    else if (strcmp(path, "/") != 0 && strcmp(path, "/index.html") != 0) { respond(s, 404, "text/plain", "Not found"); return; }
    FILE *f = fopen(name, "rb");
    if (!f) { respond(s, 500, "text/plain", "Frontend files not found. Run the server from the ZenTask project folder."); return; }
    char *buf = (char *)malloc(128 * 1024);
    if (!buf) { fclose(f); respond(s, 500, "text/plain", "Out of memory"); return; }
    size_t n = fread(buf, 1, 128 * 1024 - 1, f); fclose(f); buf[n] = 0;
    respond(s, 200, type, buf); free(buf);
}

static void handle_client(socket_t client) {
    char req[REQ_CAP + 1]; size_t have = 0, header_end = 0; int content_len = 0;
    while (have < REQ_CAP) {
#ifdef _WIN32
        int n = recv(client, req + have, (int)(REQ_CAP - have), 0);
#else
        ssize_t n = recv(client, req + have, REQ_CAP - have, 0);
#endif
        if (n <= 0) break;
        have += (size_t)n; req[have] = 0;
        char *sep = strstr(req, "\r\n\r\n");
        if (sep) {
            header_end = (size_t)(sep - req) + 4;
            char *cl = strstr(req, "Content-Length:");
            if (cl && cl < sep) content_len = atoi(cl + 15);
            if (have >= header_end + (size_t)content_len) break;
        }
    }
    if (!have) return;
    char method[8] = {0}, path[256] = {0};
    sscanf(req, "%7s %255s", method, path);
    char *body = header_end ? req + header_end : req + have;
    if (!header_end) { respond(client, 400, "text/plain", "Bad request"); return; }
    if (strcmp(path, "/api/tasks") == 0 && strcmp(method, "GET") == 0) tasks_response(client);
    else if (strcmp(path, "/api/tasks") == 0 && strcmp(method, "POST") == 0) api_create(client, body);
    else if (strncmp(path, "/api/tasks/", 11) == 0) {
        int id = parse_id(path);
        if (id <= 0) respond(client, 400, "application/json", "{\"error\":\"Invalid task id.\"}");
        else if (strcmp(method, "PUT") == 0) api_update(client, id, body);
        else if (strcmp(method, "DELETE") == 0) api_delete(client, id);
        else respond(client, 405, "text/plain", "Method not allowed");
    } else if (strcmp(path, "/api/top") == 0 && strcmp(method, "GET") == 0) {
        Task *t = heap_top(); if (!t) respond(client, 200, "application/json", "null");
        else { char result[2200]; task_json(result, sizeof result, t); respond(client, 200, "application/json", result); }
    } else if (strcmp(method, "GET") == 0) serve_static(client, path);
    else respond(client, 405, "text/plain", "Method not allowed");
}

int main(void) {
#ifdef _WIN32
    WSADATA wsa; if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { fprintf(stderr, "Winsock startup failed.\n"); return 1; }
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    hash_clear();
    FILE *f = fopen(DATA_FILE, "r");
    if (f) { fclose(f); load_tasks(DATA_FILE); }
    else {
        f = fopen("data/sample_tasks.csv", "r");
        if (f) { fclose(f); load_tasks("data/sample_tasks.csv"); save_tasks(DATA_FILE); }
    }
    socket_t server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == SOCK_ERR) { fprintf(stderr, "Could not create server socket.\n"); return 1; }
    int opt = 1;
#ifdef _WIN32
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof opt);
#else
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
#endif
    struct sockaddr_in addr; memset(&addr, 0, sizeof addr); addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); addr.sin_port = htons(PORT);
    if (bind(server, (struct sockaddr *)&addr, sizeof addr) != 0 || listen(server, 8) != 0) {
        fprintf(stderr, "Could not start on port %d. Is another app using it?\n", PORT); CLOSESOCK(server); return 1;
    }
    printf("ZenTask web is ready: http://127.0.0.1:%d\nPress Ctrl+C to stop.\n", PORT); fflush(stdout);
    for (;;) {
        socket_t client = accept(server, NULL, NULL);
        if (client == SOCK_ERR) continue;
        handle_client(client); CLOSESOCK(client);
    }
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
