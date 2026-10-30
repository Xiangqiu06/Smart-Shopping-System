#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "webserver.h"
#include"head.h"


int LoginCallback(void *arg, int argc, char **argv, char **azColName)
{
    int *login_ok = (int *)arg;

    if (argc > 0)
    {
        *login_ok = 1;
    }

    return 0;
}

typedef struct
{
    FILE *fp;
    int first;
}SearchData_t;

/**
 * @brief JSON 转义函数
 * 
 * @param src 
 * @param dest 
 * @param maxlen 
 */
void JsonEscape(char *src, char *dest, int maxlen)
{
    int i = 0;
    int j = 0;

    while (src[i] != '\0' && j < maxlen - 2)
    {
        if (src[i] == '\"')
        {
            dest[j++] = '\\';
            dest[j++] = '\"';
        }
        else if (src[i] == '\\')
        {
            dest[j++] = '\\';
            dest[j++] = '\\';
        }
        else if (src[i] == '\r' || src[i] == '\n')
        {
            dest[j++] = ' ';
        }
        else
        {
            dest[j++] = src[i];
        }

        i++;
    }

    dest[j] = '\0';
}

int SearchProductCallback(void *data,
                          int argc,
                          char **argv,
                          char **azColName)
{
    SearchData_t *pData = (SearchData_t *)data;

    char name[512] = {0};
    char brief[1024] = {0};
    char image[512] = {0};

    /*
     * 商品名称转义
     */
    JsonEscape(argv[1] ? argv[1] : "",
               name,
               sizeof(name));

    /*
     * 商品简介转义
     */
    JsonEscape(argv[3] ? argv[3] : "",
               brief,
               sizeof(brief));

    /*
     * 商品图片路径
     *
     * 数据库中例如：
     * images/200905/goods_img/4_G_1241422402722.jpg
     *
     * 这里直接使用数据库中的路径
     */
    JsonEscape(argv[5] ? argv[5] : "",
               image,
               sizeof(image));

    /*
     * 如果不是第一条数据，就加逗号
     */
    if (pData->first == 0)
    {
        fprintf(pData->fp, ",\n");
    }

    /*
     * 写入JSON
     */
    fprintf(pData->fp,
            "{"
            "\"id\":%s,"
            "\"name\":\"%s\","
            "\"price\":%s,"
            "\"brief\":\"%s\","
            "\"stock\":%s,"
            "\"image\":\"%s\""
            "}",
            argv[0] ? argv[0] : "0",
            name,
            argv[2] ? argv[2] : "0",
            brief,
            argv[4] ? argv[4] : "0",
            image);

    /*
     * 已经写入第一条数据
     */
    pData->first = 0;

    return 0;
}

/**
 * @brief 查询商品详细信息
 * 
 * @param data 
 * @param argc 
 * @param argv 
 * @param azColName 
 * @return int 
 */
int GoodsDetailCallback(void *data,
                        int argc,
                        char **argv,
                        char **azColName)
{
    FILE *fp = (FILE *)data;

    char name[512] = {0};
    char brief[1024] = {0};
    char image[512] = {0};
    char desc[4096] = {0};
    char goods_sn[256] = {0};
    char keywords[1024] = {0};

    JsonEscape(argv[1] ? argv[1] : "",
               name,
               sizeof(name));

    JsonEscape(argv[3] ? argv[3] : "",
               brief,
               sizeof(brief));

    JsonEscape(argv[5] ? argv[5] : "",
               image,
               sizeof(image));

    JsonEscape(argv[6] ? argv[6] : "",
               desc,
               sizeof(desc));

    JsonEscape(argv[7] ? argv[7] : "",
               goods_sn,
               sizeof(goods_sn));

    JsonEscape(argv[11] ? argv[11] : "",
               keywords,
               sizeof(keywords));

    fprintf(fp,
            "{"
            "\"id\":%s,"
            "\"name\":\"%s\","
            "\"price\":%s,"
            "\"brief\":\"%s\","
            "\"stock\":%s,"
            "\"image\":\"%s\","
            "\"desc\":\"%s\","
            "\"goods_sn\":\"%s\","
            "\"market_price\":%s,"
            "\"promote_price\":%s,"
            "\"goods_weight\":%s,"
            "\"keywords\":\"%s\","
            "\"is_shipping\":%s,"
            "\"add_time\":%s"
            "}",
            argv[0] ? argv[0] : "0",
            name,
            argv[2] ? argv[2] : "0",
            brief,
            argv[4] ? argv[4] : "0",
            image,
            desc,
            goods_sn,
            argv[8] ? argv[8] : "0",
            argv[9] ? argv[9] : "0",
            argv[10] ? argv[10] : "0",
            keywords,
            argv[12] ? argv[12] : "0",
            argv[13] ? argv[13] : "0");

    return 0;
}


/**
 * @brief 注册用户信息，将用户注册的信息插入到数据库中
 * 
 * @param pcontent 请求报文中处理的正文内容
 * @return int 
 */
int RegisterUser(char *pcontent)
{
    char *p = NULL;
    char *username = NULL;
    char *password = NULL;
    char Infor[512] = {0};

    p = strtok(pcontent, "&");
    username = strchr(p, '=') + 1;

    p = strtok(NULL, "&");
    password = strchr(p, '=') + 1;

    printf("username = %s\n", username);
    printf("password = %s\n", password);

    sprintf(Infor,
            "(SELECT IFNULL(MAX(user_id),0)+1 FROM admin_user),"
            "\"%s\","
            "\"123@163.com\","
            "\"%s\","
            "\"\","
            "%ld,"
            "0,"
            "\"0.0.0.0\","
            "\"\","
            "\"\","
            "\"\","
            "0,"
            "0,"
            "\"\","
            "0",
            username,
            password,
            time(NULL));

    return InsertInfo(
        "./123.db",
        "admin_user",
        "user_id,user_name,email,password,ec_salt,add_time,last_login,last_ip,action_list,nav_list,lang_type,agency_id,suppliers_id,todolist,role_id",
        Infor);
}

/**
 * @brief 根据数据库信息比对，判断是否可以跳转下一级页面
 * 
 * @param pcontent 请求报文中处理的正文内容
 * @return int 
 */

int LoginUser(char *pcontent)
{
    sqlite3 *pdb = NULL;
    char cmd[512] = {0};
    char *errmsg = NULL;
    char *p = NULL;
    char *username = NULL;
    char *password = NULL;
    int ret = 0;
    int login_ok = 0;

    // 获取用户名
    p = strtok(pcontent, "&");

    if (p == NULL)
    {
        return -1;
    }

    username = strchr(p, '=') + 1;

    // 获取密码
    p = strtok(NULL, "&");

    if (p == NULL)
    {
        return -1;
    }

    password = strchr(p, '=') + 1;

    // 打开数据库
    ret = sqlite3_open("./123.db", &pdb);

    if (ret != SQLITE_OK)
    {
        printf("sqlite3_open failed: %s\n", sqlite3_errmsg(pdb));
        sqlite3_close(pdb);
        return -1;
    }

    // 查询用户名和密码
    sprintf(cmd,
            "SELECT user_id FROM admin_user "
            "WHERE user_name=\"%s\" AND password=\"%s\";",
            username,
            password);

    printf("SQL: %s\n", cmd);

    // 查询数据库
    ret = sqlite3_exec(pdb,
                       cmd,
                       LoginCallback,
                       &login_ok,
                       &errmsg);

    if (ret != SQLITE_OK)
    {
        printf("sqlite3_exec failed: %s\n", errmsg);

        sqlite3_free(errmsg);
        sqlite3_close(pdb);

        return -1;
    }

    sqlite3_close(pdb);

    // 判断是否登录成功
    if (login_ok == 1)
    {
        printf("登录成功：%s\n", username);
        return 0;
    }
    else
    {
        printf("用户名或密码错误\n");
        return -1;
    }
}
/**
 * @brief URL 解码
 * 
 * @param src 
 * @return int 
 */
int UrlDecode(char *src)
{
    char *p = src;
    char *q = src;
    char hex[3] = {0};

    while (*p)
    {
        if (*p == '%')
        {
            if (p[1] && p[2])
            {
                hex[0] = p[1];
                hex[1] = p[2];

                *q = (char)strtol(hex, NULL, 16);

                p += 3;
                q++;
            }
            else
            {
                *q = *p;
                p++;
                q++;
            }
        }
        else if (*p == '+')
        {
            *q = ' ';
            p++;
            q++;
        }
        else
        {
            *q = *p;
            p++;
            q++;
        }
    }

    *q = '\0';

    return 0;
}

/**
 * @brief 从数据库中搜索数据
 * 
 * @param keyword 
 * @return int 
 */
int SearchProduct(char *keyword)
{
    sqlite3 *pdb = NULL;
    char sql[1024] = {0};
    char *errmsg = NULL;
    int ret = 0;

    FILE *fp = NULL;
    SearchData_t data;

    /*
     * URL解码
     */
    UrlDecode(keyword);

    printf("Search keyword: %s\n", keyword);

    /*
     * 打开数据库
     */
    ret = sqlite3_open("./123.db", &pdb);

    if (ret != SQLITE_OK)
    {
        printf("sqlite3_open failed: %s\n",
               sqlite3_errmsg(pdb));

        sqlite3_close(pdb);

        return -1;
    }

    /*
     * 打开JSON文件
     */
    fp = fopen("./search_result.json", "w");

    if (fp == NULL)
    {
        perror("fopen search_result.json failed");

        sqlite3_close(pdb);

        return -1;
    }

    /*
     * JSON开始
     */
    fprintf(fp, "[\n");

    data.fp = fp;
    data.first = 1;

    /*
     * SQL查询
     */
    sprintf(sql,
        "SELECT goods_id,goods_name,shop_price,goods_brief,goods_number,goods_img "
        "FROM goods "
        "WHERE goods_name LIKE \"%%%s%%\" "
        "AND is_on_sale=1 "
        "AND is_delete=0 "
        "ORDER BY goods_id;",
        keyword);

    printf("Search SQL: %s\n", sql);

    /*
     * 执行SQL
     */
    ret = sqlite3_exec(pdb,
                       sql,
                       SearchProductCallback,
                       &data,
                       &errmsg);

    if (ret != SQLITE_OK)
    {
        printf("sqlite3_exec failed: %s\n",
               errmsg);

        sqlite3_free(errmsg);

        fclose(fp);
        sqlite3_close(pdb);

        return -1;
    }

    /*
     * JSON结束
     */
    fprintf(fp, "\n]\n");

    fclose(fp);

    sqlite3_close(pdb);

    return 0;
}




/**
 * @brief 根据商品ID查询商品详细信息
 * 
 * @param goods_id 
 * @return int 
 */
int GetGoodsDetail(char *goods_id)
{
    sqlite3 *pdb = NULL;
    char sql[1024] = {0};
    char *errmsg = NULL;
    int ret = 0;

    FILE *fp = NULL;

    ret = sqlite3_open("./123.db", &pdb);

    if (ret != SQLITE_OK)
    {
        printf("sqlite3_open failed: %s\n",
               sqlite3_errmsg(pdb));

        sqlite3_close(pdb);

        return -1;
    }

    fp = fopen("./goods_detail.json", "w");

    if (fp == NULL)
    {
        perror("fopen goods_detail.json failed");

        sqlite3_close(pdb);

        return -1;
    }

    sprintf(sql,
            "SELECT goods_id,goods_name,shop_price,goods_brief,goods_number,goods_img,goods_desc,"
            "goods_sn,market_price,promote_price,goods_weight,keywords,is_shipping,add_time "
            "FROM goods "
            "WHERE goods_id=%s "
            "AND is_on_sale=1 "
            "AND is_delete=0;",
            goods_id);

    printf("Goods Detail SQL: %s\n", sql);

    ret = sqlite3_exec(pdb,
                       sql,
                       GoodsDetailCallback,
                       fp,
                       &errmsg);

    if (ret != SQLITE_OK)
    {
        printf("sqlite3_exec failed: %s\n",
               errmsg);

        sqlite3_free(errmsg);

        fclose(fp);
        sqlite3_close(pdb);

        return -1;
    }

    fclose(fp);

    sqlite3_close(pdb);

    return 0;
}