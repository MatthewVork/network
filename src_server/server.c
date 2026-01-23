#include "server.h"

#define PORT 8888       
#define MAX_CLIENTS 50  

int client_fds[MAX_CLIENTS]; 
Player players[MAX_CLIENTS];  
ChessRoom rooms[MAX_ROOMS];

// 基础响应函数
void send_json_response(int fd, const char* status, const char* msg) {
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "status", status);
    cJSON_AddStringToObject(reply, "msg", msg);
    char *out = cJSON_PrintUnformatted(reply);
    send(fd, out, strlen(out), 0);
    cJSON_free(out);
    cJSON_Delete(reply);
}

// 注册处理逻辑
void handle_register(int fd, cJSON *root) {
    cJSON *user_item = cJSON_GetObjectItem(root, "user");
    cJSON *pass_item = cJSON_GetObjectItem(root, "pass");

    if (!user_item || !pass_item || !user_item->valuestring) return;

    char *user = user_item->valuestring;
    char *pass = pass_item->valuestring;

    if (strlen(user) == 0) return;

    FILE *fp = fopen("users.txt", "r");
    int is_exist = 0;
    if (fp) {
        char fu[64], fpw[64];
        // 【核心修复】必须匹配2个字符串才算有效行，防止空格干扰
        while (fscanf(fp, "%s %s", fu, fpw) == 2) { 
            if (strcmp(user, fu) == 0) { is_exist = 1; break; }
        }
        fclose(fp);
    }

    if (is_exist) {
        printf("【注册拒绝】用户 [%s] 已存在\n", user);
        send_json_response(fd, "error", "用户已存在");
    } else {
        fp = fopen("users.txt", "a");
        if (fp) {
            fprintf(fp, "%s %s\n", user, pass);
            fflush(fp);
            fclose(fp);
            printf("【注册成功】新用户: %s\n", user);
            send_json_response(fd, "success", "注册成功");
        }
    }
}

// 登录处理逻辑
void handle_login(int fd, cJSON *root, Player *p) {
    cJSON *u_item = cJSON_GetObjectItem(root, "user");
    cJSON *p_item = cJSON_GetObjectItem(root, "pass");
    if (!u_item || !p_item) return;

    char *user = u_item->valuestring;
    char *pass = p_item->valuestring;

    FILE *fp = fopen("users.txt", "r");
    int found = 0;
    if (fp) {
        char fu[64], fpw[64];
        while (fscanf(fp, "%s %s", fu, fpw) == 2) {
            if (strcmp(user, fu) == 0 && strcmp(pass, fpw) == 0) { found = 1; break; }
        }
        fclose(fp);
    }

    if (found) {
        p->is_authenticated = 1;
        strncpy(p->username, user, 31);
        send_json_response(fd, "success", "登录成功");
    } else {
        send_json_response(fd, "error", "账号或密码错误");
    }
}

// ... handle_get_rooms, handle_disconnect 等函数保持你原有的实现即可 ...

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(PORT);             

    if(bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) exit(1);
    listen(listen_fd, 5);
    printf("象棋服务器已启动，监听端口: %d...\n", PORT);

    for (int i = 0; i < MAX_CLIENTS; i++) client_fds[i] = -1;

    while (1) {
        fd_set read_fds; // 【已确认】必须在循环内定义
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
                if (client_fds[i] == -1) { client_fds[i] = new_fd; break; }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_fds[i];
            if (fd > 0 && FD_ISSET(fd, &read_fds)) {
                char buf[2048] = {0}; 
                int len = recv(fd, buf, sizeof(buf) - 1, 0); 
                if (len <= 0) { close(fd); client_fds[i] = -1; }
                else {
                    buf[len] = '\0';
                    cJSON *root = cJSON_Parse(buf);
                    if (!root) continue;
                    cJSON *type_item = cJSON_GetObjectItem(root, "type");
                    if (type_item) {
                        int type = type_item->valueint;
                        if (type == 1) handle_register(fd, root);
                        else if (type == 2) handle_login(fd, root, &players[i]);
                    }
                    cJSON_Delete(root);
                }
            }
        }
    }
    return 0;
}