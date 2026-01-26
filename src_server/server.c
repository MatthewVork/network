#include "server.h"

// 定义用户数据文件路径
#define USER_DB "user.txt" 

// --- 全局变量定义 ---
int client_fds[MAX_CLIENTS];
Player players[MAX_CLIENTS];
ChessRoom rooms[MAX_ROOMS];

// --- 函数前向声明 ---
void handle_join_room(int fd, cJSON *root, Player *p);
void handle_ready(int fd, cJSON *root, Player *p);
void handle_leave_room(int fd, cJSON *root, Player *p);
void handle_cleanup(int fd, Player *p);

// ============================================================
// 1. 核心工具函数：发送 JSON 响应
// ============================================================
void send_json_response(int fd, int type, const char* status, const char* msg) {
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddNumberToObject(reply, "type", type);
    cJSON_AddStringToObject(reply, "status", status);
    cJSON_AddStringToObject(reply, "msg", msg);
    
    // 特殊逻辑：如果是创建房间成功，必须把 room_id 带回去，否则客户端没法显示
    if (type == 5 && strcmp(status, "success") == 0) {
        for(int i = 0; i < MAX_ROOMS; i++) {
            if(rooms[i].host_fd == fd && rooms[i].is_active) {
                cJSON_AddNumberToObject(reply, "room_id", rooms[i].room_id);
                break;
            }
        }
    }
    
    char *out = cJSON_PrintUnformatted(reply);
    
    // 【关键调试信息】让你知道服务器确实回包了
    printf("[SERVER SEND -> FD:%d] %s\n", fd, out);
    
    send(fd, out, strlen(out), 0);
    cJSON_free(out);
    cJSON_Delete(reply);
}

// ============================================================
// 2. 业务逻辑处理函数
// ============================================================

// --- 处理注册 (Type 1) ---
void handle_register(int fd, cJSON *root) {
    printf("[Logic] 处理注册请求...\n");
    cJSON *u = cJSON_GetObjectItem(root, "user");
    cJSON *p = cJSON_GetObjectItem(root, "pass");
    if (!u || !p) return;

    char f_u[32], f_p[32];
    int exists = 0;

    // 检查用户名是否已存在
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
            send_json_response(fd, 1, "failed", "Server File Error");
        }
    }
}

// --- 处理登录 (Type 2) ---
void handle_login(int fd, cJSON *root, Player *p) {
    printf("[Logic] 处理登录请求...\n");
    cJSON *u = cJSON_GetObjectItem(root, "user");
    cJSON *pass = cJSON_GetObjectItem(root, "pass");
    if (!u || !pass) return;

    // 防止重复登录
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (players[i].is_authenticated && strcmp(players[i].username, u->valuestring) == 0) {
            send_json_response(fd, 2, "repeat", "User already online");
            return;
        }
    }

    FILE *fp = fopen(USER_DB, "r");
    char f_u[32], f_p[32];
    int auth_status = 0; // 0:没找到 1:成功 2:密码错

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
        send_json_response(fd, 2, "success", "Login OK");
    } else if (auth_status == 2) {
        send_json_response(fd, 2, "failed", "Wrong Password");
    } else {
        send_json_response(fd, 2, "failed", "User Not Found");
    }
}

// --- 处理创建房间 (Type 5) ---
void handle_create_room(int fd, cJSON *root, Player *p) {
    if (!p->is_authenticated) return;
    
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].is_active) {
            rooms[i].is_active = 1;
            rooms[i].room_id = 1000 + i; // 简单分配：1000, 1001...
            rooms[i].host_fd = fd;
            rooms[i].red_fd = fd;       // 房主默认红方
            rooms[i].black_fd = -1;
            rooms[i].is_full = 0;
            rooms[i].red_ready = 0;
            rooms[i].black_ready = 0;
            
            p->room_id = rooms[i].room_id;
            p->side = 1; // 1=RED
            
            printf("[Logic] 用户 %s 创建了房间 %d\n", p->username, rooms[i].room_id);
            send_json_response(fd, 5, "success", "Room Created");
            return;
        }
    }
    send_json_response(fd, 5, "failed", "Room Limit Reached");
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
                send_json_response(fd, 7, "failed", "Room Full");
                return;
            }

            // 加入成功 -> 设为黑方
            rooms[i].black_fd = fd;
            rooms[i].is_full = 1;
            p->room_id = target_id;
            p->side = 2; // 2=BLACK

            // 1. 告诉加入者：成功
            send_json_response(fd, 7, "success", "Join Success");

            // 2. 告诉房主(红方)：有人进来了 (Type 8)
            cJSON *notice = cJSON_CreateObject();
            cJSON_AddNumberToObject(notice, "type", 8);
            cJSON_AddStringToObject(notice, "status", "update");
            cJSON_AddStringToObject(notice, "msg", "Opponent Joined");
            cJSON_AddStringToObject(notice, "opp_name", p->username); // 把加入者的名字发过去
            
            char *out = cJSON_PrintUnformatted(notice);
            if(rooms[i].red_fd != -1) {
                printf("[SERVER SEND -> FD:%d] (Notify Host) %s\n", rooms[i].red_fd, out);
                send(rooms[i].red_fd, out, strlen(out), 0);
            }
            cJSON_free(out);
            cJSON_Delete(notice);
            return;
        }
    }
    send_json_response(fd, 7, "failed", "Room Not Found");
}

// --- 处理准备 (Type 12) ---
void handle_ready(int fd, cJSON *root, Player *p) {
    if (!p->is_authenticated || p->room_id == 0) return;
    int rid = p->room_id;
    
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active && rooms[i].room_id == rid) {
            int current_ready = 0;
            // 切换状态
            if (p->side == 1) { 
                rooms[i].red_ready = !rooms[i].red_ready;
                current_ready = rooms[i].red_ready;
            } else if (p->side == 2) { 
                rooms[i].black_ready = !rooms[i].black_ready;
                current_ready = rooms[i].black_ready;
            }

            // 广播给房间内的所有人
            cJSON *msg = cJSON_CreateObject();
            cJSON_AddNumberToObject(msg, "type", 12);
            cJSON_AddNumberToObject(msg, "side", p->side);
            cJSON_AddNumberToObject(msg, "is_ready", current_ready);
            
            char *out = cJSON_PrintUnformatted(msg);
            
            if (rooms[i].red_fd != -1) {
                printf("[SERVER BROADCAST -> RED] %s\n", out);
                send(rooms[i].red_fd, out, strlen(out), 0);
            }
            if (rooms[i].black_fd != -1) {
                printf("[SERVER BROADCAST -> BLACK] %s\n", out);
                send(rooms[i].black_fd, out, strlen(out), 0);
            }
            
            cJSON_free(out);
            cJSON_Delete(msg);
            
            // 检查游戏是否开始 (双方都准备)
            if (rooms[i].red_ready && rooms[i].black_ready) {
                printf("[Logic] 房间 %d 双方准备就绪，游戏开始！\n", rid);
                // 这里可以发送 Type 15 Start Game 消息
            }
            return;
        }
    }
}

// --- 处理离开房间 (Type 9) ---
void handle_leave_room(int fd, cJSON *root, Player *p) {
    if (!p->is_authenticated || p->room_id == 0) return;
    int rid = p->room_id;

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_active && rooms[i].room_id == rid) {
            int opp_fd = (rooms[i].red_fd == fd) ? rooms[i].black_fd : rooms[i].red_fd;
            
            // 如果房主离开，销毁房间
            if (rooms[i].red_fd == fd) {
                printf("[Logic] 房主离开，销毁房间 %d\n", rid);
                rooms[i].is_active = 0;
                if(opp_fd != -1) send_json_response(opp_fd, 9, "update", "Host Left");
            } else {
                // 加入者离开
                printf("[Logic] 加入者离开房间 %d\n", rid);
                rooms[i].black_fd = -1;
                rooms[i].is_full = 0;
                rooms[i].black_ready = 0;
                if(opp_fd != -1) send_json_response(opp_fd, 9, "update", "Opponent Left");
            }
            
            // 重置玩家状态
            p->room_id = 0;
            p->side = 0;
            send_json_response(fd, 9, "success", "Leave OK");
            return;
        }
    }
}

// --- 离线清理 ---
void handle_cleanup(int fd, Player *p) {
    printf("[System] 清理用户资源 FD=%d\n", fd);
    if(p->room_id != 0) {
        // 如果在房间里断线，尝试做一次退出房间逻辑
        // 这里简化处理，直接重置
        for(int i=0; i<MAX_ROOMS; i++) {
            if(rooms[i].room_id == p->room_id) {
                if(rooms[i].host_fd == fd) rooms[i].is_active = 0; // 房主跑了，房间销毁
                else { rooms[i].black_fd = -1; rooms[i].is_full = 0; }
                break;
            }
        }
    }
    p->is_authenticated = 0;
    p->room_id = 0;
    memset(p->username, 0, 32);
}

// ============================================================
// 3. 主程序
// ============================================================
int main() {
    // 1. 自动清理端口 (防止 bind failed)
    system("fuser -k 8888/tcp > /dev/null 2>&1");
    sleep(1); // 等一秒让端口释放

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888), {htonl(INADDR_ANY)}};
    
    // 端口复用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return -1;
    }
    
    listen(listen_fd, 5);
    printf("==========================================\n");
    printf("🚀 Server Started on Port 8888\n");
    printf("Waiting for connections...\n");
    printf("==========================================\n");

    // 初始化 fd 数组
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

        // 新连接
        if (FD_ISSET(listen_fd, &rset)) {
            int nfd = accept(listen_fd, NULL, NULL);
            printf("[System] 新客户端接入: FD=%d\n", nfd);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == -1) {
                    client_fds[i] = nfd;
                    memset(&players[i], 0, sizeof(Player));
                    players[i].fd = nfd;
                    break;
                }
            }
        }

        // 处理数据
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_fds[i];
            if (fd > 0 && FD_ISSET(fd, &rset)) {
                char buf[2048] = {0};
                int len = recv(fd, buf, 2047, 0);
                
                if (len <= 0) {
                    printf("[System] 客户端断开 FD=%d\n", fd);
                    handle_cleanup(fd, &players[i]);
                    close(fd); 
                    client_fds[i] = -1;
                } else {
                    printf("[SERVER RECV] %s\n", buf); // 打印收到的原始 JSON
                    
                    cJSON *json = cJSON_Parse(buf);
                    if (!json) {
                        printf("[Error] JSON 解析失败\n");
                        continue;
                    }

                    cJSON *t = cJSON_GetObjectItem(json, "type");
                    if (t) {
                        int type = t->valueint;
                        // 分发逻辑
                        if (type == 1) handle_register(fd, json);
                        else if (type == 2) handle_login(fd, json, &players[i]);
                        else if (type == 5) handle_create_room(fd, json, &players[i]);
                        else if (type == 6) { /* 退出登录 */ handle_cleanup(fd, &players[i]); }
                        else if (type == 7) handle_join_room(fd, json, &players[i]);
                        else if (type == 9) handle_leave_room(fd, json, &players[i]);
                        else if (type == 12) handle_ready(fd, json, &players[i]);
                    }
                    cJSON_Delete(json);
                }
            }
        }
    }
    return 0;
}