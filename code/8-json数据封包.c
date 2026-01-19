
//{"info":[{"name":"张三","sex":"男","age":20},{"name":"李四","sex":"女","age":19}]}
#include <stdio.h>
#include <string.h>
#include "cJSON.h"

struct Student{
    char name[32];
    char sex[8];
    int age;
};

int main(void)
{
    struct Student stuArray[2] = {{"张三","男",20},{"李四","女",19}};
    //json打包
    //创建最外层的对象
    cJSON* root =  cJSON_CreateObject();
    //创建info对于的值(数组)
    cJSON* array = cJSON_CreateArray();
    for(int i=0; i<2; i++)
    {
        //创建数组array中的对象
        cJSON *arrayObj = cJSON_CreateObject();
        //插入键值对
        cJSON_AddItemToObject(arrayObj, "name", cJSON_CreateString(stuArray[i].name));
        cJSON_AddItemToObject(arrayObj, "sex", cJSON_CreateString(stuArray[i].sex));
        cJSON_AddItemToObject(arrayObj, "age", cJSON_CreateNumber(stuArray[i].age));
        //把对象添加到数组
        cJSON_AddItemToArray(array, arrayObj);
    }
    cJSON_AddItemToObject(root, "info", array);
    //char *str  = cJSON_Print(root);
    char *str  = cJSON_PrintUnformatted(root);
    printf("%d:%s\n", strlen(str),str);
    cJSON_Delete(root);
    return 0;
}