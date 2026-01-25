#include "server.h"

#define PORT 8888

int client_fds[MAX_CLIENTS];
Player players[MAX_CLIENTS];
ChessRoom rooms[MAX_ROOMS];

void send_json_response(int fd, int type, const char* status, const char* msg) {
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddNumberToObject(reply, "type", type);
    cJSON_AddStringToObject(reply, "status", status);
    cJSON_AddStringToObject(reply, "msg", msg);
    
    if (type == 5 && strcmp(status, "success") == 0) {
        for(int i = 0; i < MAX_ROOMS; i++) {
            if(rooms[i].host_fd == fd) {
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

void handle_login(int fd, cJSON *root, Player *p) {
    cJSON *u = cJSON_GetObjectItem(root, "user");
    if(u && u->valuestring) {
        p->is_authenticated = 1;
        p->fd = fd;
        strncpy(p->username, u->valuestring, 31);
        send_json_response(fd, 2, "success", "登录成功");
        printf("【系统】用户 %s 登录成功\n", p->username);
    }
}

void handle_create_room(int fd, cJSON *root, Player *p) {
    if (!p->is_authenticated) {
        send_json_response(fd, 5, "error", "请先登录");
        return;
    }
    int idx = -1;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active == 0) {
            idx = i;
            break;
        }
    }
    if (idx != -1) {
        rooms[idx].is_active = 1;
        rooms[idx].room_id = 1000 + idx;
        rooms[idx].host_fd = fd;
        rooms[idx].red_fd = fd;
        p->room_id = rooms[idx].room_id;
        p->side = 1; 
        send_json_response(fd, 5, "success", "创建成功");
        printf("【房间】用户 %s 创建了房间 %d\n", p->username, rooms[idx].room_id);
    } else {
        send_json_response(fd, 5, "error", "房间已满");
    }
}

void handle_leave_room(int fd, Player *p) {
    if (p->room_id == 0) return;
    int idx = -1;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_id == p->room_id) {
            idx = i;
            break;
        }
    }
    if (idx != -1) {
        if (rooms[idx].red_fd == fd) rooms[idx].red_fd = 0;
        else if (rooms[idx].black_fd == fd) rooms[idx].black_fd = 0;

        printf("【退出】用户 [%s] 离开房间 %d\n", p->username, rooms[idx].room_id);

        if (rooms[idx].red_fd == 0 && rooms[idx].black_fd == 0) {
            memset(&rooms[idx], 0, sizeof(ChessRoom));
            printf("【销毁】房间 %d 已被清空回收\n", 1000 + idx);
        } else {
            rooms[idx].is_full = 0;
            int other_fd = (rooms[idx].red_fd != 0) ? rooms[idx].red_fd : rooms[idx].black_fd;
            send_json_response(other_fd, 6, "info", "对方已离开房间");
        }
    }
    p->room_id = 0;
    p->side = 0;
    send_json_response(fd, 6, "success", "已退出");
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888), {htonl(INADDR_ANY)}};
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 5);

    memset(rooms, 0, sizeof(rooms));
    for (int i = 0; i < MAX_CLIENTS; i++) client_fds[i] = -1;

    printf("象棋服务端运行中，端口 8888...\n");

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(listen_fd, &read_fds);
        int max_fd = listen_fd;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] > 0) {
                FD_SET(client_fds[i], &read_fds);
                if (client_fds[i] > max_fd) max_fd = client_fds[i];
            }
        }
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) continue;
        if (FD_ISSET(listen_fd, &read_fds)) {
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
            if (fd > 0 && FD_ISSET(fd, &read_fds)) {
                char buf[2048] = {0};
                int len = recv(fd, buf, 2047, 0);
                if (len <= 0) {
                    handle_leave_room(fd, &players[i]);
                    close(fd); client_fds[i] = -1;
                } else {
                    cJSON *json = cJSON_Parse(buf);
                    if (!json) continue;
                    cJSON *type_item = cJSON_GetObjectItem(json, "type");
                    if (type_item) {
                        int type = type_item->valueint;
                        if (type == 2) handle_login(fd, json, &players[i]);
                        else if (type == 5) handle_create_room(fd, json, &players[i]);
                        else if (type == 6) handle_leave_room(fd, &players[i]);
                    }
                    cJSON_Delete(json);
                }
            }
        }
    }
    return 0;
}