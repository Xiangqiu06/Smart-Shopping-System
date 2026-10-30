#ifndef __OPERATION_H__
#define __OPERATION_H__

extern int RegisterUser(char *pcontent);

extern int LoginCallback(void *arg,
                         int argc,
                         char **argv,
                         char **azColName);

extern int RegisterUser(char *pcontent);
extern int LoginUser(char *pcontent);
extern int UrlDecode(char *src);
extern int SearchProduct(char *keyword);
extern int GetGoodsDetail(char *goods_id);

#endif
