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
	size_t length; 		/* HEADS长度 */
	size_t used; 		/* 已添加的HEAD个数 */
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
/*
 * 创建一个HTTP_T结构
 */
http_t *http_new(const char *user, const char *pass); 
/*
 * 连接远端服务器
 */
bool http_connect(http_t *h, const char *host, uint16_t port); 
/*
 * 在HTTP响应的首部中查找是否存在指定的首部
 */
bool resp_has_head(http_t *h, char *head); 
/*
 * 获取服务器响应的状态码
 */
int http_status(http_t *h); 
/*
 * 将指定的首部数据添加到带发送的请求中
 */
bool add_head_for_req(http_t *h, char *hd, ...); 
/*
 * 设置HTTP请求
 * 例如:
 * http_set_req(h, "GET", "www.foo.com/index.html")
 * 程序会自动在method和req之后添加HTTP/1.1
 */
bool http_set_req(http_t *h, char *method, char *req); 
/*
 * 发送HTTP请求
 */
int http_send_request(http_t *h); 
/*
 * 回去服务器响应，此接口只获取起始行和首部数据
 */
int http_get_response(http_t *h); 
/*
 * 获取服务器发送的主体部分的数据, 
 * 使用方法和read函数类似
 */
int http_get_body(http_t *h, char *buf, size_t len); 
/*
 * 关闭HTTP连接并销毁HTTP_T结构
 */
void  http_close(http_t *h); 
#define HTTP_CLOSE(h) do{ http_close(h); h = NULL} while (0)
/*
 * 下面的两个接口用来获取指向请求或响应报文的首部的指针
 */
const char **http_req_heads(http_t *h); 
const char **http_resp_heads(http_t *h); 
#endif /* end of include guard: HTTP_JWUY4C82 */
