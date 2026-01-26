#include "server.h"

#define USER_DB "user.txt" 

// --- 全局变量定义 ---
int client_fds[MAX_CLIENTS];
Player players[MAX_CLIENTS];
ChessRoom rooms[MAX_ROOMS];

// --- 统一响应函数 ---
void send_json_response(int fd, int type, const char* status, const char* msg) {
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddNumberToObject(reply, "type", type);
    cJSON_AddStringToObject(reply, "status", status);
    cJSON_AddStringToObject(reply, "msg", msg);
    
    if (type == 5 && strcmp(status, "success") == 0) {
        for(int i = 0; i < MAX_ROOMS; i++) {
            if(rooms[i].host_fd == fd && rooms[i].is_active) {
                cJSON_AddNumberToObject(reply, "room_id", rooms[i].room_id);
                break;
            }
        }
    }
    char *out = cJSON_PrintUnformatted(reply);
    send(fd, out, strlen(out), 0);
    cJSON_free(out);
    cJSON_Delete(reply);
}

// --- 1. 注册逻辑 ---
void handle_register(int fd, cJSON *root) {
    cJSON *u = cJSON_GetObjectItem(root, "user");
    cJSON *p = cJSON_GetObjectItem(root, "pass");
    if (!u || !p) return;

    char f_u[32], f_p[32];
    int exists = 0;
    FILE *fp = fopen(USER_DB, "r");
    if (fp) {
        while (fscanf(fp, "%s %s", f_u, f_p) != EOF) {
            if (strcmp(f_u, u->valuestring) == 0) {
                exists = 1;
                break;
            }
        }
        fclose(fp);
    }

    if (exists) {
        send_json_response(fd, 1, "repeat", "User already exists");
    } else {
        fp = fopen(USER_DB, "a+");
        if (fp) {
            fprintf(fp, "%s %s\n", u->valuestring, p->valuestring);
            fclose(fp);
            send_json_response(fd, 1, "success", "Register success");
        } else {
            send_json_response(fd, 1, "failed", "Server file error");
        }
    }
}

// --- 2. 登录逻辑 ---
void handle_login(int fd, cJSON *root, Player *p) {
    cJSON *u = cJSON_GetObjectItem(root, "user");
    cJSON *pass = cJSON_GetObjectItem(root, "pass");
    if (!u || !pass) return;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (players[i].is_authenticated && strcmp(players[i].username, u->valuestring) == 0) {
            send_json_response(fd, 2, "repeat", "User is already online");
            return;
        }
    }

    FILE *fp = fopen(USER_DB, "r");
    char f_u[32], f_p[32];
    int auth_status = 0; 
    if (fp) {
        while (fscanf(fp, "%s %s", f_u, f_p) != EOF) {
            if (strcmp(f_u, u->valuestring) == 0) {
                if (strcmp(f_p, pass->valuestring) == 0) auth_status = 1;
                else auth_status = 2;
                break;
            }
        }
        fclose(fp);
    }

    if (auth_status == 1) {
        p->is_authenticated = 1;
        p->fd = fd;
        strncpy(p->username, u->valuestring, 31);
        send_json_response(fd, 2, "success", "Welcome");
    } else {
        send_json_response(fd, 2, "failed", "Invalid username or password");
    }
}

// --- 3. 离线处理 ---
void handle_cleanup(int fd, Player *p) {
    p->is_authenticated = 0; 
    memset(p->username, 0, 32);
    p->room_id = 0;
    printf("[SERVER] FD %d 已安全离线\n", fd);
}

// --- 4. 主循环 ---
int main() {
    system("fuser -k 8888/tcp > /dev/null 2>&1"); 
    sleep(1);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888), {htonl(INADDR_ANY)}};
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed"); return -1;
    }
    listen(listen_fd, 5);
    printf("Server Start! Listening on 8888...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) client_fds[i] = -1;

    while (1) {
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(listen_fd, &rset);
        int max_fd = listen_fd;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] > 0) {
                FD_SET(client_fds[i], &rset);
                if (client_fds[i] > max_fd) max_fd = client_fds[i];
            }
        }

        select(max_fd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listen_fd, &rset)) {
            int nfd = accept(listen_fd, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == -1) {
                    client_fds[i] = nfd;
                    memset(&players[i], 0, sizeof(Player));
                    players[i].fd = nfd;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_fds[i];
            if (fd > 0 && FD_ISSET(fd, &rset)) {
                char buf[2048] = {0};
                int len = recv(fd, buf, 2047, 0);
                if (len <= 0) {
                    handle_cleanup(fd, &players[i]);
                    close(fd); client_fds[i] = -1;
                } else {
                    cJSON *json = cJSON_Parse(buf);
                    if (!json) continue;
                    cJSON *t = cJSON_GetObjectItem(json, "type");
                    if (t) {
                        int type = t->valueint;
                        if (type == 1) handle_register(fd, json);
                        else if (type == 2) handle_login(fd, json, &players[i]);
                        else if (type == 5) handle_create_room(fd, json, &players[i]);
                        else if (type == 6) handle_cleanup(fd, &players[i]);
                        else if (type == 7) handle_join_room(fd, json, &players[i]);
                        else if (type == 9) handle_leave_room(fd, json, &players[i]);
                        else if (type == 11) handle_ready_toggle(fd, json, &players[i]);
                    }
                    cJSON_Delete(json);
                }
            }
        }
    }
    return 0;
}

// --- 业务处理函数 ---
void handle_create_room(int fd, cJSON *root, Player *p) {
    if (!p->is_authenticated) return;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].is_active) {
            rooms[i].is_active = 1;
            rooms[i].room_id = 1000 + i;
            rooms[i].host_fd = fd;
            rooms[i].red_fd = fd;
            rooms[i].black_fd = -1;
            rooms[i].is_full = 0;
            rooms[i].red_ready = 0;
            rooms[i].black_ready = 0;
            p->room_id = rooms[i].room_id;
            p->side = 1;
            send_json_response(fd, 5, "success", "OK");
            return;
        }
    }
    send_json_response(fd, 5, "failed", "Room limit reached");
}

void handle_join_room(int fd, cJSON *root, Player *p) {
    if (!p->is_authenticated) return;
    cJSON *rid_obj = cJSON_GetObjectItem(root, "room_id");
    if (!rid_obj) return;
    int target_id = rid_obj->valueint;

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active && rooms[i].room_id == target_id) {
            if (rooms[i].is_full) {
                send_json_response(fd, 7, "failed", "Room is full");
                return;
            }
            rooms[i].black_fd = fd;
            rooms[i].is_full = 1;
            p->room_id = target_id;
            p->side = 2;

            send_json_response(fd, 7, "success", "Join OK");

            cJSON *notice = cJSON_CreateObject();
            cJSON_AddNumberToObject(notice, "type", 8);
            cJSON_AddStringToObject(notice, "status", "update");
            cJSON_AddStringToObject(notice, "opp_name", p->username);
            char *out = cJSON_PrintUnformatted(notice);
            send(rooms[i].red_fd, out, strlen(out), 0);
            cJSON_free(out);
            cJSON_Delete(notice);
            return;
        }
    }
    send_json_response(fd, 7, "failed", "Room not found");
}

void handle_leave_room(int fd, cJSON *root, Player *p) {
    if (!p->is_authenticated || p->room_id == 0) return;
    int rid = p->room_id;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active && rooms[i].room_id == rid) {
            int opp_fd = (rooms[i].red_fd == fd) ? rooms[i].black_fd : rooms[i].red_fd;
            if (rooms[i].red_fd == fd) rooms[i].is_active = 0; 
            else {
                rooms[i].black_fd = -1;
                rooms[i].is_full = 0;
                rooms[i].black_ready = 0;
            }
            if (opp_fd != -1) send_json_response(opp_fd, 9, "update", "Opponent left");
            p->room_id = 0;
            send_json_response(fd, 9, "success", "Left OK");
            return;
        }
    }
}

void handle_ready_toggle(int fd, cJSON *root, Player *p) {
    if (!p->is_authenticated || p->room_id == 0) return;
    int rid = p->room_id;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active && rooms[i].room_id == rid) {
            int is_red = (rooms[i].red_fd == fd);
            if (is_red) rooms[i].red_ready = !rooms[i].red_ready;
            else rooms[i].black_ready = !rooms[i].black_ready;

            int opp_fd = is_red ? rooms[i].black_fd : rooms[i].red_fd;
            if (opp_fd != -1) {
                cJSON *notice = cJSON_CreateObject();
                cJSON_AddNumberToObject(notice, "type", 12);
                cJSON_AddStringToObject(notice, "user", p->username);
                cJSON_AddNumberToObject(notice, "is_ready", is_red ? rooms[i].red_ready : rooms[i].black_ready);
                cJSON_AddNumberToObject(notice, "side", is_red ? 1 : 2);
                char *out = cJSON_PrintUnformatted(notice);
                send(opp_fd, out, strlen(out), 0);
                cJSON_free(out);
                cJSON_Delete(notice);
            }
            send_json_response(fd, 11, "success", ""); 
            return;
        }
    }
}