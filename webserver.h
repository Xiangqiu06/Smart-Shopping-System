#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "sqlite3.h"

typedef struct {
    char *pmethod;
    char *purl;
    char *pcontent;
} HttpRequest_t;

int CreateListenSocket(int Port);
int RecvHttpRequest(int confd, char *precvbuf, int maxlen);

/* 新接口（正确拼写） */
int ParseHttpRequest(char *precvbuf, HttpRequest_t *pTmpRequest);
int SendHttpResponse(int confd, HttpRequest_t *pTmpRequest);
int SendHttpResponseHeader(int confd, char *pfilename, int status_code);
int SendHttpResponseContent(int confd, char *pfilename);
int SendHttpResponseString(int confd, const char *data, const char *content_type, int status_code);

/* 兼容旧拼写的包装函数（防止其他文件编译失败） */
int PraseHttpRequest(char *precvbuf, HttpRequest_t *pTmpRequest);
int SendHttpRespone(int confd, HttpRequest_t *pTmpRequest);
int SendHttpResponeHeader(int confd, char *pfilename);
int SendHttpResponeContent(int confd, char *pfilename);

int InsertInfo(const char *TmpSqlist, const char *TableName, 
               const char *Column, const char *Infor);

#endif