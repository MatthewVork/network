#include "cJSON.h"

void send_auth_request(const char *type, const char *user, const char *pwd)
{
    //创建cJSON对象
    cJSON *root = cJSON_CreateObject();

    //填充对象数据
    cJSON_AddStringToObject(root, "type", type);    //register or login
    cJSON_AddStringToObject(root, "user", user);
    cJSON_AddStringToObject(root, "pwd", pwd);

    //创建对象后将对象转换为字符串
    char *json_data = cJSON_PrintUnformatted(root);
}