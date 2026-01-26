#include "server.h"

#define USER_DB "user.txt" // 存储账号密码的文件名

// --- 全局变量 ---
int client_fds[MAX_CLIENTS];
Player players[MAX_CLIENTS];
ChessRoom rooms[MAX_ROOMS];

void handle_join_room(int fd, cJSON *root, Player *p); // 前向声明

// --- 统一响应函数 ---
void send_json_response(int fd, int type, const char* status, const char* msg) {
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddNumberToObject(reply, "type", type);
    cJSON_AddStringToObject(reply, "status", status);
    cJSON_AddStringToObject(reply, "msg", msg);
    
    // 如果是创建房间成功，要把分配的ID带回去，否则客户端没法显示
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

// --- 1. 注册逻辑：保存到文件 ---
void handle_register(int fd, cJSON *root) {
    cJSON *u = cJSON_GetObjectItem(root, "user");
    cJSON *p = cJSON_GetObjectItem(root, "pass");
    if (!u || !p) return;

    char f_u[32], f_p[32];
    int exists = 0;

    // A. 先读文件，看用户名是否被注册过
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
        // B. 用户名不存在，追加写入文件 (保存密码)
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

// --- 2. 登录逻辑：从文件读取并校验密码 ---
void handle_login(int fd, cJSON *root, Player *p) {
    cJSON *u = cJSON_GetObjectItem(root, "user");
    cJSON *pass = cJSON_GetObjectItem(root, "pass");
    if (!u || !pass) return;

    // A. 内存查重：防止同一个号同时登两个客户端
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (players[i].is_authenticated && strcmp(players[i].username, u->valuestring) == 0) {
            send_json_response(fd, 2, "repeat", "User is already online");
            return;
        }
    }

    // B. 文件校验：去 user.txt 里对暗号
    FILE *fp = fopen(USER_DB, "r");
    char f_u[32], f_p[32];
    int auth_status = 0; // 0: 没找到, 1: 成功, 2: 密码错

    if (fp) {
        while (fscanf(fp, "%s %s", f_u, f_p) != EOF) {
            if (strcmp(f_u, u->valuestring) == 0) {
                if (strcmp(f_p, pass->valuestring) == 0) {
                    auth_status = 1; // 账号密码全对
                } else {
                    auth_status = 2; // 账号对，密码错
                }
                break;
            }
        }
        fclose(fp);
    }

    if (auth_status == 1) {
        // 验证通过，记录在线状态
        p->is_authenticated = 1;
        p->fd = fd;
        strncpy(p->username, u->valuestring, 31);
        send_json_response(fd, 2, "success", "Welcome");
    } else {
        // 验证失败
        send_json_response(fd, 2, "failed", "Invalid username or password");
    }
}

// --- 3. 离线清理：确保账号能再次登录 ---
void handle_cleanup(int fd, Player *p) {
    p->is_authenticated = 0; // 只要断开，就把这个名额空出来
    memset(p->username, 0, 32);
    p->room_id = 0;
    printf("[SERVER] FD %d 已安全离线，在线标记已清除\n", fd);
}

// --- 4. 主函数：包含强制清理端口逻辑 ---
int main() {
    system("fuser -k 8888/tcp > /dev/null 2>&1"); // 强制征用端口
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
                        if (type == 1) {
                            handle_register(fd, json);
                        } else if (type == 2) {
                            handle_login(fd, json, &players[i]);
                        } else if (type == 5) {
                            handle_create_room(fd, json, &players[i]);
                        } else if (type == 6) { // 新增：处理主动退出
                            printf("[SERVER] 用户 %s 请求退出登录\n", players[i].username);
                            handle_cleanup(fd, &players[i]);
                            // 可选：给客户端一个确认回包
                            send_json_response(fd, 6, "success", "Logout OK");
                        } else if (type == 7) handle_join_room(fd, json, &players[i]); // 新增加入房间
                          else if (type == 9) handle_leave_room(fd, json, &players[i]); // 新增离开房间
                    }
                    cJSON_Delete(json);
                }
            }
        }
    }
    return 0;
}

// --- 处理创建房间 (Type 5) ---
void handle_create_room(int fd, cJSON *root, Player *p) {
    if (!p->is_authenticated) return;
    
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].is_active) {
            rooms[i].is_active = 1;
            rooms[i].room_id = 1000 + i; // 自动分配房间号
            rooms[i].host_fd = fd;
            rooms[i].red_fd = fd;       // 房主默认红方
            rooms[i].black_fd = -1;
            rooms[i].is_full = 0;
            
            p->room_id = rooms[i].room_id;
            p->side = 1; // 红方
            
            printf("[SERVER] 用户 %s 创建房间 %d 成功\n", p->username, rooms[i].room_id);
            send_json_response(fd, 5, "success", "OK");
            return;
        }
    }
    send_json_response(fd, 5, "failed", "Room limit reached");
}

// --- 处理加入房间 (Type 7) ---
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

            // 加入成功，设为黑方
            rooms[i].black_fd = fd;
            rooms[i].is_full = 1;
            p->room_id = target_id;
            p->side = 2; // 黑方

            // 1. 回复加入者成功
            send_json_response(fd, 7, "success", "Join OK");

            // 2. 通知房主有人进来了 (Type 8: 对手加入通知)
            cJSON *notice = cJSON_CreateObject();
            cJSON_AddNumberToObject(notice, "type", 8);
            cJSON_AddStringToObject(notice, "status", "update");
            cJSON_AddStringToObject(notice, "msg", "Opponent joined");
            cJSON_AddStringToObject(notice, "opp_name", p->username);
            char *out = cJSON_PrintUnformatted(notice);
            send(rooms[i].red_fd, out, strlen(out), 0);
            
            printf("[SERVER] 用户 %s 加入房间 %d，通知房主 FD %d\n", p->username, target_id, rooms[i].red_fd);
            
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
            // 1. 找到对手的 FD
            int opponent_fd = (rooms[i].red_fd == fd) ? rooms[i].black_fd : rooms[i].red_fd;

            // 2. 如果是房主(红方)退出，直接销毁房间
            if (rooms[i].red_fd == fd) {
                printf("[SERVER] 房主 %s 退出，房间 %d 销毁\n", p->username, rid);
                if (opponent_fd != -1) {
                    send_json_response(opponent_fd, 9, "update", "Host left"); // 通知对手
                }
                rooms[i].is_active = 0; 
            } 
            // 3. 如果是加入者(黑方)退出，房间设为未满
            else {
                printf("[SERVER] 加入者 %s 退出房间 %d\n", p->username, rid);
                rooms[i].black_fd = -1;
                rooms[i].is_full = 0;
                if (opponent_fd != -1) {
                    send_json_response(opponent_fd, 9, "update", "Opponent left");
                }
            }

            // 4. 清理玩家本人的房间状态，但保留登录状态
            p->room_id = 0;
            p->side = 0;
            
            send_json_response(fd, 9, "success", "Left OK");
            return;
        }
    }
}