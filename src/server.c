#include "server.h"

#define PORT 8888       // 服务器监听端口
#define MAX_CLIENTS 50  // 最大同时在线人数

int client_fds[MAX_CLIENTS]; 
Player players[MAX_CLIENTS];  
ChessRoom rooms[MAX_ROOMS];

int main() {
    int listen_fd; // 服务器监听用的 Socket
    struct sockaddr_in serv_addr; // 服务器的网络信息结构体

    // 1. 创建 Socket (AF_INET: IPv4; SOCK_STREAM: TCP协议)
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // 【重要】设置端口复用：防止服务器崩溃重启时提示“Address already in use”
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //定义了一个开关变量opt， 1 -> 开启， 0 -> 关闭
    //第二个函数是设置套接字选项，SOL_SOCKET -> 通用， SO_REUSEADDR -> 复用地址 
    //因为TCP协议为了保证数据的完整传输，底层在释放端口之前会进入一段等待时间，若重启可能导致时间等待
    //opt：代表开启这个功能，通过这个地址找这个值，然后读取的内存大小由Sizeof决定 

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(PORT);             

    if(bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Bind failed"); 
        exit(EXIT_FAILURE);
    }

    if(listen(listen_fd, 5) < 0)
    {
        perror("Listen failed"); 
        exit(EXIT_FAILURE);
    }

    printf("象棋服务器已启动，正在监听端口: %d...\n", PORT);

    // 初始化玩家和房间
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
        players[i].fd = -1;
        players[i].is_authenticated = 0;
        players[i].room_id = -1;
    }
    for (int i = 0; i < MAX_ROOMS; i++) {
        rooms[i].room_id = i;
        rooms[i].red_fd = rooms[i].black_fd = -1;
        rooms[i].red_ready = rooms[i].black_ready = 0;
        rooms[i].is_full = -1;
    }

    fd_set read_fds; 
    while (1) {
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
            int new_fd = accept(listen_fd, NULL, NULL); 
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == -1) {
                    client_fds[i] = new_fd;
                    players[i].fd = new_fd;
                    printf("【系统】新玩家已连接: FD %d\n", new_fd);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_fds[i];
            if (fd > 0 && FD_ISSET(fd, &read_fds)) {
                char buf[2048] = {0}; 
                int len = recv(fd, buf, sizeof(buf), 0); 

                if (len <= 0) { 
                    handle_disconnect(fd); 
                    close(fd);         
                    client_fds[i] = -1; 
                    players[i].is_authenticated = 0;
                    players[i].room_id = -1;
                } else {
                    cJSON *root = cJSON_Parse(buf);
                    if (!root) continue;
                    cJSON *type_item = cJSON_GetObjectItem(root, "type");
                    if (type_item) {
                        int type = type_item->valueint;
                        switch (type) {
                            case 1: handle_register(fd, root); break; // 注册
                            case 2: handle_login(fd, root, &players[i]); break; // 登录
                            case 3: handle_get_rooms(fd); break; // 大厅列表
                            case 4: handle_join_room(fd, root, &players[i]); break; // 进房
                            case 5: handle_ready(fd, &players[i]); break; // 准备
                            case 6: handle_move(fd, root, &players[i]); break; // 转发棋子
                        }
                    }
                    cJSON_Delete(root);
                }
            }
        }
    }
    return 0;
}

// --- 以下是所有函数的完整实现，绝对没有删减 ---

void handle_register(int fd, cJSON *root) {
    char *user = cJSON_GetObjectItem(root, "user")->valuestring;
    char *pass = cJSON_GetObjectItem(root, "pass")->valuestring;
    FILE *fp = fopen("users.txt", "r");
    if (fp) {
        char fu[32], fpw[32];
        while (fscanf(fp, "%s %s", fu, fpw) != EOF) {
            if (strcmp(user, fu) == 0) { 
                fclose(fp); 
                send_json_response(fd, "error", "用户已存在"); 
                return; 
            }
        }
        fclose(fp);
    }
    fp = fopen("users.txt", "a");
    if (fp) { 
        fprintf(fp, "%s %s\n", user, pass); 
        fclose(fp); 
        send_json_response(fd, "success", "注册成功"); 
    }
}

void handle_login(int fd, cJSON *root, Player *p) {
    char *user = cJSON_GetObjectItem(root, "user")->valuestring;
    char *pass = cJSON_GetObjectItem(root, "pass")->valuestring;
    FILE *fp = fopen("users.txt", "r");
    if (!fp) {
        send_json_response(fd, "error", "数据库文件不存在");
        return;
    }
    char fu[32], fpw[32]; int found = 0;
    while (fscanf(fp, "%s %s", fu, fpw) != EOF) {
        if (strcmp(user, fu) == 0 && strcmp(pass, fpw) == 0) { 
            found = 1; break; 
        }
    }
    fclose(fp);
    if (found) { 
        p->is_authenticated = 1; 
        strncpy(p->username, user, 31); 
        send_json_response(fd, "success", "登录成功"); 
    } else {
        send_json_response(fd, "error", "账号或密码错误");
    }
}

void handle_get_rooms(int fd) {
    cJSON *rep = cJSON_CreateObject();
    cJSON_AddStringToObject(rep, "status", "room_list");
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < MAX_ROOMS; i++) {
        cJSON *r = cJSON_CreateObject();
        cJSON_AddNumberToObject(r, "id", rooms[i].room_id);
        cJSON_AddNumberToObject(r, "state", rooms[i].is_full);
        cJSON_AddItemToArray(arr, r);
    }
    cJSON_AddItemToObject(rep, "data", arr);
    char *out = cJSON_PrintUnformatted(rep);
    send(fd, out, strlen(out), 0);
    free(out); cJSON_Delete(rep);
}

void handle_join_room(int fd, cJSON *root, Player *p) {
    int r_id = cJSON_GetObjectItem(root, "room_id")->valueint;
    if(r_id < 0 || r_id >= MAX_ROOMS) return;
    ChessRoom *r = &rooms[r_id];
    if (r->is_full == 1) { send_json_response(fd, "error", "房间已满"); return; }
    if (r->red_fd == -1) { r->red_fd = fd; p->side = 0; r->is_full = 0; }
    else { r->black_fd = fd; p->side = 1; r->is_full = 1; }
    p->room_id = r_id;
    send_json_response(fd, "success", "入座成功");
}

void handle_ready(int fd, Player *p) {
    if (p->room_id == -1) return;
    ChessRoom *r = &rooms[p->room_id];
    if (fd == r->red_fd) r->red_ready = 1;
    else r->black_ready = 1;
    if (r->red_ready && r->black_ready) {
        send_json_response(r->red_fd, "start", "game_begin");
        send_json_response(r->black_fd, "start", "game_begin");
    }
}

void handle_move(int fd, cJSON *root, Player *p) {
    if (p->room_id == -1) return;
    ChessRoom *r = &rooms[p->room_id];
    int target = (fd == r->red_fd) ? r->black_fd : r->red_fd;
    if (target != -1) {
        char *out = cJSON_PrintUnformatted(root);
        send(target, out, strlen(out), 0);
        free(out);
    }
}

void handle_disconnect(int fd) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].red_fd == fd) { 
            rooms[i].red_fd = -1; rooms[i].red_ready = 0; 
            rooms[i].is_full = (rooms[i].black_fd == -1) ? -1 : 0; 
        }
        if (rooms[i].black_fd == fd) { 
            rooms[i].black_fd = -1; rooms[i].black_ready = 0; 
            rooms[i].is_full = (rooms[i].red_fd == -1) ? -1 : 0; 
        }
    }
}

void send_json_response(int fd, const char* status, const char* msg) {
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "status", status);
    cJSON_AddStringToObject(reply, "msg", msg);
    char *out = cJSON_PrintUnformatted(reply);
    send(fd, out, strlen(out), 0);
    free(out); cJSON_Delete(reply);
}