/*
 * =====================================================================================
 *
 *       Filename:  http.h
 *
 *    Description:  HTTP模块
 *
 *        Version:  1.0
 *        Created:  2012年12月28日 17时37分17秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef HTTP_JWUY4C82

#define HTTP_JWUY4C82

#include "net.h"

#include <stdint.h>

#define HTTP_PORT	80

struct http_msg {
	char  *start; 		/* 起始字段 */
	char  **heads; 		/* 首部 */
	/* char * body */	/* BODY 字段程序另行处理 */
}; 

typedef struct _http_t {
	struct http_msg  h_req, h_resp; 	/* 存储客户请求和服务器响应信息 */
	int h_status; 					/* HTTP状态 */
	socket_t *h_sock; 				/* 网络连接 */
	time_t h_tm; 					/* 此结构创建时间 */
	char *h_user; 					/* HTTP 用户名 */
	char *h_pass; 					/* HTTP 用户密码 */
} http_t;
http_t *http_new(const char *user, const char *pass); 
bool http_connect(http_t *h, const char *host, uint16_t port); 
bool resp_has_head(http_t *h, char *head); 
int http_status(http_t *h); 
bool add_head_for_req(http_t *h, char *hd, ...); 
void discard_heads_for_req(http_t *h); 
bool http_set_req(http_t *h, char *method, char *req); 
int http_send_request(http_t *h); 
int http_get_reponse(http_t *h); 
int http_get_body(http_t *h, char *buf, size_t len); 
int http_close(http_t *h); 
#define HTTP_CLOSE(h) do{ http_close(h); h = NULL} while (0)
char **http_req_heads(http_t *h); 
char **http_resp_heads(http_t *h); 
#endif /* end of include guard: HTTP_JWUY4C82 */
