#include <stdio.h>
#include <string.h>
#include "head.h"
#include "webserver.h"

int main(void)
{
    int sockfd = 0;
    int confd = 0;
    char recvbuff[4096] = {0};
    HttpRequest_t pTmpRequest = {0};

    sockfd = CreateListenSocket(8080);
    if (sockfd == -1)
    {
        fprintf(stderr, "Failed to create listen socket on port 8080\n");
        return -1;
    }

    printf("Server listening on port 8080...\n");

    while (1)
    {
        confd = accept(sockfd, NULL, NULL);
        if (confd == -1)
        {
            perror("fail to accept");
            continue;  /* 关键修复：不要退出，继续接受下一个连接 */
        }

        /* 关键修复：每次请求前清零，防止残留数据干扰 */
        memset(recvbuff, 0, sizeof(recvbuff));
        memset(&pTmpRequest, 0, sizeof(pTmpRequest));

        RecvHttpRequest(confd, recvbuff, sizeof(recvbuff));

        printf("============================== RECV ==============================\n");
        printf("%s\n", recvbuff);
        printf("==================================================================\n");

        /* 处理 HTTP 请求报文 */
        if (ParseHttpRequest(recvbuff, &pTmpRequest) == 0)
        {
            SendHttpResponse(confd, &pTmpRequest);
        }
        else
        {
            const char *bad_request = 
                "HTTP/1.1 400 Bad Request\r\n"
                "Server: TinyServer\r\n"
                "Content-Type: text/plain; charset=utf-8\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Bad Request";
            send(confd, bad_request, strlen(bad_request), 0);
        }

        close(confd);
    }

    close(sockfd);
    return 0;
}