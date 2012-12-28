/*
 * =====================================================================================
 *
 *       Filename:  http.c
 *
 *    Description:  HTTP接口实现
 *
 *        Version:  1.0
 *        Created:  2012年12月28日 19时56分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "http.h"
#include "net.h"
#include "log.h"
#include "error_code.h"
#include "fmdl.h"

#include <stdio.h>
#include <strings.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_HEAD_LENGTH 256
#define DEF_HD_CNT 8
#define MAX_REQUEST_BUF 1024

#define GET_HEADS(msg) (msg).heads
#define GET_START(msg) (msg).start
static void push_head(struct http_msg *m, char *hd); 

static char *string_lower(char *s, bool dup); 
#define string_lower_dup(s) string_lower(s, true)
#define string_lower_nodup(s) string_lower(s, false)

http_t *http_new(const char *user, const char *pass)
{
	http_t *h; 
	h = calloc(1, sizeof(http_t)); 
	assert(h != NULL); 
	return h; 
}

bool http_connect(http_t *h, const char *host, uint16_t port)
{
	assert (host != NULL && port >= 0); 
	assert (h != NULL); 
	h->h_sock = sock_open_host(host, port); 
	if (h->h_sock == NULL) return false; 
	h->h_tm = time(NULL); 
	return true; 
}

char *string_lower(char *s, bool dup)
{
	assert (s != NULL); 

	char *p = dup == true ? strdup(s) : s; 
	char *p1 = p; 
	while (*p1) {
		*p1 = tolower(*p1); 
		p1++; 
	}
	return p; 
}

bool resp_has_head(http_t *h, char *head)
{
	assert (h != NULL && head != NULL); 
	char *hd = string_lower_dup(head); 
	char **p = GET_HEADS(h->h_resp); 
	while (*p) {
		char *hd1 = *p++; 
		if (strncasecmp(hd1, hd, strlen(hd)) == 0) {
			/*
			 * 找到此首部
			 */
			free(hd); 
			return true; 
		}
	}
	free(hd); 
	return false; 
}

int http_status(http_t *h)
{
	assert (h != NULL);
	return h->h_status; 
}

bool  add_head_for_req(http_t *h, char *hd, ...)
{
	assert (h != NULL && hd != NULL); 

	char *head = calloc(1, MAX_HEAD_LENGTH + 1); 
	if (!head) return false; 
	va_list arg; 
	va_start(arg, hd); 
	vsnprintf(head, MAX_HEAD_LENGTH, hd, arg); 
	va_end(arg); 
	push_head(&h->h_req, head); 
	return true; 
}

void push_head(struct http_msg *m, char *hd) 
{
	assert (m != NULL && hd != NULL); 

	static size_t length;  /* 记录HTTP_MSG结构中HEADS字段大小 */
	static size_t used = 0;   
	if (length == 0) {
		/* 必须多分配一个字段，指示HEADS的结尾 */
		m->heads = calloc(DEF_HD_CNT + 1, sizeof(char *)); 
		assert (m->heads != NULL); 
		length = DEF_HD_CNT; 
	}
	if (used == length) {
		size_t oldl = length; 
		length += DEF_HD_CNT; 
		m->heads = realloc(m->heads, length + 1); 
		memset(m->heads + oldl, 0, length - oldl + 1); 
	}
	int i = 0; 
	char **p = m->heads; 
	for (; i < length; i++) {
		if (p[i] == 0) {
			p[i] = hd; 
			used++; 
			break; 
		}
	}
}

void discard_heads_for_req(http_t *h)
{
	char **p = GET_HEADS(h->h_req); 
	char **p1 = p; 
	if (p) {
		while (*p1) free (*p1++); 
		free(p); 
		GET_HEADS(h->h_req) = NULL; 
	}
}

bool http_set_req(http_t *h, char *method, char *req)
{
	assert (h != NULL); 
	assert (method != NULL && req != NULL); 

	char *type = "HTTP/1.1"; 
	char *request = calloc(MAX_REQUEST_BUF + 1, sizeof(char)); 
	if (!request) return false; 
	if (GET_START(h->h_req)) free(GET_START(h->h_req)); 
	GET_START(h->h_req) = request; 
	snprintf(request, MAX_REQUEST_BUF, "%s %s %s", 
			method, req, type); 
	return true; 
}

int http_send_request(http_t *h)
{
#define SEND_REQ	do {\
	size_t len = strlen(send); \
	size_t total = 0; \
	int ret; \
	while ((ret = sock_write(h->h_sock, send + total, len - total)) >= 0) {\
		all = total + ret; \
		if (total + ret == len) break; \
	}\
	if (ret < 0) return ret; \
} while (0)
	assert (h != NULL); 
	/*
	 * 防止未调用HTTP_CONNECT
	 */
	if (h->h_sock == NULL) {
		return -1; 
	}
	char send[MAX_REQUEST_BUF]; 
	size_t all = 0; 
	sprintf(send, "%s\r\n", GET_START(h->h_req)); 
	/* 发送起始行 */
	SEND_REQ; 
	/* 发送首部 */
	{
		char **p = GET_HEADS(h->h_req); 
		while (*p) {
			sprintf(send, "%s\r\n", *p++); 
			SEND_REQ; 
		}
		sprintf(send, "\r\n"); 
		SEND_REQ; 
	}
	return all; 
}

