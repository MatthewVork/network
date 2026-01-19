
//{"info":[{"name":"张三","sex":"男","age":20},{"name":"李四","sex":"女","age":19}]}
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
int main(void)
{
    char str[]="{\"info\":[{\"name\":\"张三\",\"sex\":\"男\",\"age\":20},{\"name\":\"李四\",\"sex\":\"女\",\"age\":19}]}";

    //把字符串转对象
    cJSON* root = cJSON_Parse(str);

    //用info解析出数组
    cJSON* array = cJSON_GetObjectItem(root,"info");
    //遍历数组
    int size = cJSON_GetArraySize(array);
    for(int i=0; i<size; i++)
    {
        cJSON* arrayObj = cJSON_GetArrayItem(array, i);
        //解析对象name, sex,age
        cJSON* item = cJSON_GetObjectItem(arrayObj, "name");
        if(item != NULL)
        {
            printf("name:%s\n", item->valuestring);
        }
        item = cJSON_GetObjectItem(arrayObj, "sex");
        if(item != NULL)
        {
            printf("sex:%s\n", item->valuestring);
        }
        item = cJSON_GetObjectItem(arrayObj, "age");
        if(item != NULL)
        {
            printf("age:%lf\n", item->valuedouble);
        }
    }
    cJSON_Delete(root);
}