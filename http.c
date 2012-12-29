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

#define GET_HEADS(msg) ((msg).heads)
#define GET_START(msg) ((msg).start)
#define ENSURE_SOCK(sock, buf, len, fun, ret) do {\
	size_t rnt, total = 0; \
	while ((rnt = (fun)((sock), (buf) + total, (len) - total))) {\
		if (rnt + total == (len)) break; \
		total += rnt; \
	}\
	if (rnt  < 0) return rnt; \
	ret = (len); \
} while (0)

static void push_head(struct http_msg *m, char *hd); 
static int get_start_data(http_t *h); 
static int get_heads_data(http_t *h); 
static char *string_lower(char *s, bool dup); 
static void discard_heads(struct http_msg *h); 
static void discard_start(struct http_msg *h); 
static int check_byte_to_read(socket_t *sock, const char *delim); 
static void free_msg_data(struct http_msg *m); 
/* 从HTTP响应中解析出服务器的响应码 */
static int get_http_status(const char *start); 

#define string_lower_dup(s) string_lower(s, true)
#define string_lower_nodup(s) string_lower(s, false)

http_t *http_new(const char *user, const char *pass)
{
	http_t *h; 
	h = calloc(1, sizeof(http_t)); 
	assert(h != NULL); 
	h->h_user = user == NULL ? NULL : strdup(user); 
	h->h_pass = pass == NULL ? NULL : strdup(pass);
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

	size_t length = m->length;  
	size_t used = m->used;   
	if (length == 0) {
		m->heads = calloc(DEF_HD_CNT, sizeof(char *)); 
		assert (m->heads != NULL); 
		length = DEF_HD_CNT; 
		m->length = length; 
	}
	if (used == length - 1) {
		size_t oldl = length; 
		length += DEF_HD_CNT; 
		m->length = length; 
		m->heads = realloc(m->heads, length); 
		memset(m->heads + oldl, 0, length - oldl + 1); 
	}
	char **p = m->heads; 
	p[used] = hd; 
	m->used++; 
}

void discard_heads(struct http_msg *h)
{
	char **p = GET_HEADS(*h); 
	char **p1 = p; 
	if (p) {
		while (*p1) free (*p1++); 
		memset(*p, 0, sizeof(char*)*(h->used)); 
		h->used = 0; 
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
	assert (h != NULL); 
	/*
	 * 防止未调用HTTP_CONNECT
	 */
	if (h->h_sock == NULL) {
		return -1; 
	}
	if (GET_START(h->h_req) == NULL) {
		log_printf(LOG_ERROR, "DOn`t have the request method!\n"); 
		return -1; 
	}

	/* 计算所有要发送的数据的数量 */
	size_t all = 0; 
	int i = 0; 

	all = all + 2 + strlen(GET_START(h->h_req)); 
	for (; i < h->h_req.used; i++) 
		all = strlen(GET_HEADS(h->h_req)[i]) + 2; 
	all += 2; 	/* 结束符\r\n */

	char *send = malloc (all + 1); 
	size_t snd = 0; 
	int ret; 
	assert(send != NULL); 
	snd = sprintf(send, "%s\r\n", GET_START(h->h_req)); 
	for (i = 0; i < h->h_req.used; i++) {
		snd += sprintf(send + snd, "%s\r\n", GET_HEADS(h->h_req)[i]);
	}
	snd += sprintf(send + snd, "\r\n"); 
	if (snd != all) return -1; 
	 
	ENSURE_SOCK(h->h_sock, send, all, sock_write, ret); 
	/* 所有数据发送之后，丢弃存储的所有数据 */
	discard_start(&h->h_req); 
	discard_heads(&h->h_req); 
	return ret; 
}

void discard_start(struct http_msg *h)
{
	assert(h != NULL); 
	if (GET_START(*h)) free(GET_START(*h)); 
	GET_START(*h) = NULL; 
}

int http_get_response(http_t *h)
{
	assert (h != NULL && h->h_sock != NULL); 
	int ret; 
	size_t total = 0; 
	ret = get_start_data(h); 
	if (ret <= 0) return ret; 
	total += ret; 
	ret = get_heads_data(h); 
	if (ret < 0) {
		discard_start(&h->h_resp); 
		return ret; 
	}
	/*
	 * 获取HTTP状态码
	 */
	int status = get_http_status(GET_START(h->h_resp)); 
	if (status <= 0) {
		discard_start(&h->h_resp); 
		discard_heads(&h->h_resp); 
		return -1; 
	}
	h->h_status = status; 
	return total + ret; 
}

int get_start_data(http_t *h)
{
	assert (h != NULL && h->h_sock != NULL); 
	int ret; 
	discard_start(&h->h_resp); 
	ret = check_byte_to_read(h->h_sock, "\r\n");
	if (ret == -1) return ret; 
	if (ret == -2) {
		/* 未找到\r\n可能是服务器发送的结束符为\n */
		ret = check_byte_to_read(h->h_sock, "\n");
		if (ret < 0) return -1; 
	}
	if (GET_START(h->h_resp)) {
		free(GET_START(h->h_resp)); 
	}
	GET_START(h->h_resp) = malloc(ret + 1); 
	char *p = GET_START(h->h_resp); 
	size_t len = ret; 
	assert (p != NULL); 
	ENSURE_SOCK(h->h_sock, p, len, sock_read, ret); 
	return ret; 
}

int get_heads_data(http_t *h)
{
	assert (h != NULL && h->h_sock != NULL); 
	int ret; 
	size_t all = 0; 
	char *mutil = NULL; 
	size_t mutillen = 0; 
	size_t mutiltotal = 0; 
	size_t ctl; 
	bool flag = false; 
	discard_heads(&h->h_resp); 
	while (1) {
		size_t read; 
		ctl = 2; 
		ret = check_byte_to_read(h->h_sock, "\r\n"); 
		if (ret == -1) return ret; 
		if (ret == -2) {
			ctl = 1; 
			ret = check_byte_to_read(h->h_sock, "\n"); 
			if (ret < 0) return -1; 
		}
		char *buf = malloc(ret + 1); 
		assert(buf != NULL); 
		ENSURE_SOCK(h->h_sock, buf, ret, sock_read, read); 
		/*
		 * 首部全部读取，1和2表示控制字符长度
		 * \r\n为2
		 * \n为1
		 */
		if (read == 1 || read == 2) break; 

		all += read; 
		/* 多行首部 */
		if (isspace(*buf)) {
			flag = true; 
			if (!mutil) {
				mutillen = MAX_HEAD_LENGTH; 
				mutil = malloc(mutillen + 1); 
				assert (mutil != NULL); 
				mutiltotal = sprintf(mutil, "%s", buf); 
				/* 覆盖前一次的控制字符 */
				mutiltotal -= ctl; 
			}
			if (mutiltotal + read >= mutillen ) {
				mutillen = mutiltotal + read - mutillen + MAX_HEAD_LENGTH; 
				mutil = realloc(mutil, mutillen); 
				assert(mutil != NULL); 
			}
			mutiltotal += sprintf(mutil + mutiltotal, "%s", buf); 
			mutiltotal -= ctl; 
			free (buf); 
			continue; 
		}

		if (flag) {
			push_head(&h->h_resp, mutil); 
			mutil = NULL; 
			flag = false; 
		}
		push_head(&h->h_resp, buf); 
	}
	return all; 
}

int check_byte_to_read(socket_t *sock, const char *delim)
{
	assert (sock != NULL); 
	assert (delim != NULL); 
	char *buf = malloc(MAX_HEAD_LENGTH + 1); 
	size_t len = MAX_HEAD_LENGTH; 
	assert(buf != NULL); 
	int ret = 0; 
	char *p; 
	ret = sock_peek(sock, buf, len); 
	if (ret <= 0) return -1; 
	p = strstr(buf, delim); 
	if (!p) {
		/* 服务器发送的数据不是以delim分割的 */
		return -2; 
	}
	return p - buf + strlen(delim); 
}

int get_http_status(const char *start)
{
	assert(start != NULL); 
	int ret; 
	sscanf(start, "%*s %d %*s", &ret); 
	return ret; 
}

void http_close(http_t *h)
{
	if (!h) return; 
	free_msg_data(&h->h_req); 
	free_msg_data(&h->h_resp); 
	sock_close(h->h_sock); 
	if (h->h_user) free(h->h_user); 
	if (h->h_pass) free(h->h_pass); 
	free(h); 
}
void free_msg_data(struct http_msg *m)
{
	assert (m != NULL); 

	if (m->start) free(m->start); 
	if (m->heads) {
		char *p = *m->heads; 
		while (p) free(p++); 
		free(m->heads); 
	}
}
int http_get_body(http_t *h, char *buf, size_t len)
{
	assert (h != NULL); 
	assert (buf != NULL && len > 0); 
	return sock_read(h->h_sock, buf, len); 
}
const char **http_req_heads(http_t *h)
{
	return (const char **)GET_HEADS(h->h_req); 
}

const char **http_resp(http_t *h)
{
	char **p = GET_HEADS(h->h_resp); 
	if (p) {
		/* 响应的首部控制字符需要移除 */
		char *hd = *p; 
		while (hd) {
			size_t cnt; 
			cnt = strlen(hd); 
			cnt--; 
			if (hd[cnt] == '\n') hd[cnt--] = '\0'; 
			if (hd[cnt] == '\r') hd[cnt] = '\0'; 
			hd++; 
		}
	}
	return (const char **)p; 
}

