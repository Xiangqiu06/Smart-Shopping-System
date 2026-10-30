#include "webserver.h"
#include <stdio.h>
#include <string.h>
#include "head.h"
#include "operation.h"


int CreateListenSocket(int Port)
{
    int ret = 0;
    int sockfd = 0;
    struct sockaddr_in seraddr;
    int opt = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        perror("fail to socket");
        return -1;
    }

    /* 关键修复：允许端口复用，防止服务器重启后 "Address already in use" */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("fail to setsockopt");
        close(sockfd);
        return -1;
    }

    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(Port);
    seraddr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr));
    if (-1 == ret)
    {
        perror("fail to bind");
        close(sockfd);  /* 防止 fd 泄漏 */
        return -1;
    }

    ret = listen(sockfd, 10);
    if (-1 == ret)
    {
        perror("fail to listen");
        close(sockfd);  /* 防止 fd 泄漏 */
        return -1;
    }

    return sockfd;
}

int RecvHttpRequest(int confd, char *precvbuf, int maxlen)
{
    ssize_t nret = 0;

    nret = recv(confd, precvbuf, maxlen - 1, 0);

    if (-1 == nret)
    {
        perror("fail to recv");
        return -1;
    }

    precvbuf[nret] = '\0';

    return nret;
}

/* ==================== 修复拼写 + 增加健壮性 ==================== */
int ParseHttpRequest(char *precvbuf, HttpRequest_t *pTmpRequest)
{
    if (precvbuf == NULL || pTmpRequest == NULL)
    {
        return -1;
    }

    pTmpRequest->pcontent = strstr(precvbuf, "\r\n\r\n");

    if (pTmpRequest->pcontent != NULL)
    {
        pTmpRequest->pcontent += 4;
    }

    pTmpRequest->pmethod = strtok(precvbuf, " ");
    pTmpRequest->purl = strtok(NULL, " ");

    /* 关键修复：防止畸形请求导致空指针崩溃 */
    if (pTmpRequest->pmethod == NULL || pTmpRequest->purl == NULL)
    {
        return -1;
    }

    return 0;
}

/* 兼容旧拼写 */
int PraseHttpRequest(char *precvbuf, HttpRequest_t *pTmpRequest)
{
    return ParseHttpRequest(precvbuf, pTmpRequest);
}

/* ==================== 重构响应头发送 ==================== */
int SendHttpResponseHeader(int confd, char *pfilename, int status_code)
{
    char tmpbuff[4096] = {0};
    ssize_t nret = 0;
    const char *status_text = "200 OK";

    char *content_type = "text/html; charset=utf-8";

    if (pfilename != NULL)
    {
        if (strstr(pfilename, ".jpg") != NULL ||
            strstr(pfilename, ".jpeg") != NULL)
        {
            content_type = "image/jpeg";
        }
        else if (strstr(pfilename, ".png") != NULL)
        {
            content_type = "image/png";
        }
        else if (strstr(pfilename, ".gif") != NULL)
        {
            content_type = "image/gif";
        }
        else if (strstr(pfilename, ".json") != NULL)
        {
            content_type = "application/json; charset=utf-8";
        }
    }

    switch (status_code)
    {
        case 404: status_text = "404 Not Found"; break;
        case 500: status_text = "500 Internal Server Error"; break;
        case 400: status_text = "400 Bad Request"; break;
        case 204: status_text = "204 No Content"; break;
        case 403: status_text = "403 Forbidden"; break;
        case 405: status_text = "405 Method Not Allowed"; break;
        default:  status_text = "200 OK"; break;
    }

    snprintf(tmpbuff, sizeof(tmpbuff),
             "HTTP/1.1 %s\r\n"
             "Server: TinyServer\r\n"
             "Content-Type: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             status_text, content_type);

    nret = send(confd, tmpbuff, strlen(tmpbuff), 0);

    if (-1 == nret)
    {
        perror("fail to send");
        return -1;
    }

    return 0;
}

/* 兼容旧两参数接口 */
int SendHttpResponeHeader(int confd, char *pfilename)
{
    return SendHttpResponseHeader(confd, pfilename, 200);
}

/* ==================== 新增：直接发送字符串响应 ==================== */
int SendHttpResponseString(int confd, const char *data, const char *content_type, int status_code)
{
    char header[4096] = {0};
    const char *status_text = "200 OK";
    ssize_t nret = 0;

    switch (status_code)
    {
        case 200: status_text = "200 OK"; break;
        case 404: status_text = "404 Not Found"; break;
        case 500: status_text = "500 Internal Server Error"; break;
        case 400: status_text = "400 Bad Request"; break;
        case 204: status_text = "204 No Content"; break;
        default:  status_text = "200 OK"; break;
    }

    snprintf(header, sizeof(header),
             "HTTP/1.1 %s\r\n"
             "Server: TinyServer\r\n"
             "Content-Type: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             status_text,
             content_type ? content_type : "text/plain; charset=utf-8");

    nret = send(confd, header, strlen(header), 0);
    if (nret == -1)
    {
        perror("fail to send header");
        return -1;
    }

    if (data != NULL && strlen(data) > 0)
    {
        nret = send(confd, data, strlen(data), 0);
        if (nret == -1)
        {
            perror("fail to send data");
            return -1;
        }
    }

    return 0;
}

int SendHttpResponseContent(int confd, char *pfilename)
{
    int fd = 0;
    char tmpbuff[4096] = {0};
    ssize_t nret = 0;

    fd = open(pfilename, O_RDONLY);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    while (1)
    {
        memset(tmpbuff, 0, sizeof(tmpbuff));
        nret = read(fd, tmpbuff, sizeof(tmpbuff));
        if (-1 == nret)
        {
            perror("fail to read");
            close(fd);
            return -1;
        }
        else if (0 == nret)
        {
            break;
        }

        nret = send(confd, tmpbuff, nret, 0);
        if (-1 == nret)
        {
            perror("fail to send");
            close(fd);
            return -1;
        }
    }

    close(fd);
    return 0;
}

/* 兼容旧拼写 */
int SendHttpResponeContent(int confd, char *pfilename)
{
    return SendHttpResponseContent(confd, pfilename);
}

/* ==================== 新增：路径安全检查 ==================== */
static int is_path_safe(const char *path)
{
    if (path == NULL)
        return 0;
    
    /* 禁止目录遍历 */
    if (strstr(path, "..") != NULL)
        return 0;
    
    return 1;
}

/* ==================== 新增：简单的 URL 解码 ==================== */
static void url_decode(char *str)
{
    char *src = str;
    char *dst = str;
    while (*src)
    {
        if (*src == '%' && src[1] && src[2])
        {
            int hex;
            sscanf(src + 1, "%2x", &hex);
            *dst++ = (char)hex;
            src += 3;
        }
        else if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/*
 * 函数功能：向指定 SQLite 数据库的指定数据表中插入数据
 * （保持原样）
 */
int InsertInfo(const char *TmpSqlist,
               const char *TableName,
               const char *Column,
               const char *Infor)
{
    sqlite3 *pdb = NULL;
    char cmd[1024] = {0};
    int ret = 0;

    ret = sqlite3_open(TmpSqlist, &pdb);

    if (ret != SQLITE_OK)
    {
        printf("sqlite3_open failed: %s\n",
               sqlite3_errmsg(pdb));

        sqlite3_close(pdb);
        return -1;
    }

    sprintf(cmd,
            "INSERT INTO %s (%s) VALUES (%s);",
            TableName,
            Column,
            Infor);

    printf("SQL: %s\n", cmd);

    ret = sqlite3_exec(pdb,
                       cmd,
                       NULL,
                       NULL,
                       NULL);

    if (ret != SQLITE_OK)
    {
        printf("sqlite3_exec failed: %s\n",
               sqlite3_errmsg(pdb));

        sqlite3_close(pdb);
        return -1;
    }

    sqlite3_close(pdb);

    return 0;
}

/* ==================== 核心修复：SendHttpResponse ==================== */
int SendHttpResponse(int confd, HttpRequest_t *pTmpRequest)
{
    char filename[256] = {0};
    int ret = 0;

    if (pTmpRequest == NULL || pTmpRequest->pmethod == NULL || pTmpRequest->purl == NULL)
    {
        SendHttpResponseHeader(confd, NULL, 400);
        return -1;
    }

    /* ==================== 处理 GET 请求 ==================== */
    if (0 == strcmp(pTmpRequest->pmethod, "GET"))
    {
        if (0 == strcmp(pTmpRequest->purl, "/"))
        {
            snprintf(filename, sizeof(filename), "./homepage.html");
        }
        else if (0 == strcmp(pTmpRequest->purl, "/register.html"))
        {
            snprintf(filename, sizeof(filename), "./register.html");
        }
        else if (0 == strcmp(pTmpRequest->purl, "/search.html"))
        {
            snprintf(filename, sizeof(filename), "./search.html");
        }
        else if (0 == strcmp(pTmpRequest->purl, "/favicon.ico"))
        {
            /* 关键修复：原来直接 return 0 不发送任何数据，浏览器会挂起等待 */
            SendHttpResponseHeader(confd, NULL, 204);
            return 0;
        }
        else if (strncmp(pTmpRequest->purl, "/goods?id=", 10) == 0)
        {
            snprintf(filename, sizeof(filename), "./goods_detail.html");
        }
        else if (strncmp(pTmpRequest->purl, "/goods_detail?id=", 17) == 0)
        {
            char *goods_id = NULL;
            goods_id = pTmpRequest->purl + 17;

            ret = GetGoodsDetail(goods_id);

            if (ret == 0)
            {
                snprintf(filename, sizeof(filename), "./goods_detail.json");
            }
            else
            {
                /* 
                 * 关键修复：商品不存在时，原来返回 search.html（HTML 页面），
                 * 但前端 AJAX 期望 JSON。现在返回 404 JSON，前端正确显示错误。
                 */
                SendHttpResponseString(confd,
                    "{\"error\":\"商品不存在或查询失败\",\"code\":404}",
                    "application/json; charset=utf-8",
                    404);
                return 0;
            }
        }
        else
        {
            /* 关键修复：防止路径遍历攻击，如 GET ../../etc/passwd */
            if (!is_path_safe(pTmpRequest->purl))
            {
                SendHttpResponseHeader(confd, NULL, 403);
                return -1;
            }
            snprintf(filename, sizeof(filename), ".%s", pTmpRequest->purl);
        }
    }
    /* ==================== 处理 POST 请求 ==================== */
    else if (0 == strcmp(pTmpRequest->pmethod, "POST"))
    {
        /* 注册 */
        if (0 == strcmp(pTmpRequest->purl, "/register"))
        {
            ret = RegisterUser(pTmpRequest->pcontent);

            if (ret == 0)
            {
                snprintf(filename, sizeof(filename), "./homepage.html");
            }
            else
            {
                snprintf(filename, sizeof(filename), "./register.html");
            }
        }
        /* 登录 */
        else if (0 == strcmp(pTmpRequest->purl, "/login"))
        {
            ret = LoginUser(pTmpRequest->pcontent);

            if (ret == 0)
            {
                snprintf(filename, sizeof(filename), "./search.html");
            }
            else
            {
                snprintf(filename, sizeof(filename), "./homepage.html");
            }
        }
        /* 搜索 */
        else if (0 == strcmp(pTmpRequest->purl, "/search"))
        {
            char *keyword = NULL;

            if (pTmpRequest->pcontent == NULL)
            {
                SendHttpResponseString(confd,
                    "{\"error\":\"请求体为空\",\"code\":400}",
                    "application/json; charset=utf-8",
                    400);
                return 0;
            }

            /* 复制请求体，避免破坏原始缓冲区 */
            char content_buf[2048] = {0};
            strncpy(content_buf, pTmpRequest->pcontent, sizeof(content_buf) - 1);

            /* 关键修复：正确提取 keyword 字段，而不是假设它在第一个位置 */
            char *p = strtok(content_buf, "&");
            while (p != NULL)
            {
                if (strncmp(p, "keyword=", 8) == 0)
                {
                    keyword = p + 8;
                    break;
                }
                p = strtok(NULL, "&");
            }

            if (keyword == NULL || strlen(keyword) == 0)
            {
                SendHttpResponseString(confd,
                    "{\"error\":\"缺少 keyword 参数\",\"code\":400}",
                    "application/json; charset=utf-8",
                    400);
                return 0;
            }

            /* 对 URL 编码进行解码（前端 encodeURIComponent 过） */
            url_decode(keyword);

            ret = SearchProduct(keyword);

            if (ret == 0)
            {
                snprintf(filename, sizeof(filename), "./search_result.json");
            }
            else
            {
                /*
                 * 关键修复：原来搜索失败返回 search.html（HTML 页面），
                 * 前端 fetch 期望 JSON，解析失败导致逻辑错乱。
                 * 现在返回空数组 []，前端正确显示"没有找到相关商品"。
                 */
                SendHttpResponseString(confd, "[]",
                    "application/json; charset=utf-8",
                    200);
                return 0;
            }
        }
        else
        {
            SendHttpResponseHeader(confd, NULL, 404);
            return -1;
        }
    }
    else
    {
        SendHttpResponseHeader(confd, NULL, 405);
        return -1;
    }

    /* ==================== 发送文件响应 ==================== */
    if (filename[0] != '\0')
    {
        if (0 == access(filename, F_OK))
        {
            SendHttpResponseHeader(confd, filename, 200);
            SendHttpResponseContent(confd, filename);
        }
        else
        {
            /* 文件不存在，返回 404 */
            SendHttpResponseHeader(confd, NULL, 404);
            const char *not_found = 
                "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>404</title></head>"
                "<body><h1>404 Not Found</h1>"
                "<p>请求的资源不存在。</p></body></html>";
            send(confd, not_found, strlen(not_found), 0);
        }
    }

    return 0;
}

/* 兼容旧拼写 */
int SendHttpRespone(int confd, HttpRequest_t *pTmpRequest)
{
    return SendHttpResponse(confd, pTmpRequest);
}