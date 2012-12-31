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
static bool delete_head_for_req(http_t *h, const char *hd); 
static int get_start_data(http_t *h); 
static int get_heads_data(http_t *h); 
bool search_head(struct http_msg *m, const char *head, char **ret); 
static char *string_lower(char *s, bool dup); 
static void discard_heads(struct http_msg *h); 
static void discard_start(struct http_msg *h); 
static void add_head(struct http_msg *m, char *hd); 
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

	if (user == NULL) user = ""; 
	if (pass == NULL) pass = ""; 

	size_t l = strlen(user) + strlen(pass); 
	char auth[MAX_STRING] = {0}; 
	char base64_encode[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz0123456789+/";

	l = ((l + 1) << 3)/6 ; 
	h->h_auth = calloc(l + 1, sizeof(char)); 
	assert(h->h_auth != NULL); 

	sprintf(auth, "%s:%s", user, pass); 
	/* 对用户名和密码进行BASIC64编码 */
	{
		int i = 0; 
		for (; auth[i*3]; i++) {
			h->h_auth[i*4] = base64_encode[(auth[i*3]>>2)];
			h->h_auth[i*4+1] = base64_encode[((auth[i*3]&3)<<4)|(auth[i*3+1]>>4)];
			h->h_auth[i*4+2] = base64_encode[((auth[i*3+1]&15)<<2)|(auth[i*3+2]>>6)];
			h->h_auth[i*4+3] = base64_encode[auth[i*3+2]&63];
			if( auth[i*3+2] == 0 ) h->h_auth[i*4+3] = '=';
			if( auth[i*3+1] == 0 ) h->h_auth[i*4+2] = '=';
		}
	}
	return h; 
}

bool http_connect(http_t *h, const char *host, uint16_t port)
{
	assert (host != NULL && port >= 0); 
	assert (h != NULL); 
	h->h_sock = sock_open_host(host, port); 
	if (h->h_sock == NULL) return false; 
	h->h_tm = time(NULL); 
	h->h_host = strdup(host); 
	h->h_port = port; 
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

bool search_head(struct http_msg *m, const char *head, char **ret)
{
	assert (m != NULL && head != NULL); 

	char *hd = string_lower_dup((char *)head); 
	char **p = GET_HEADS(*m); 

	if (!p) {
		if (ret) *ret = NULL; 
		return false; 
	}

	while (*p) {
		char *hd1 = *p++; 
		if (strncasecmp(hd1, hd, strlen(hd)) == 0) {
			/*
			 * 找到此首部
			 */
			free(hd); 
			if (ret) *ret = hd1; 
			return true; 
		}
	}

	free(hd); 
	if (ret) *ret = NULL; 
	return false; 
}

bool resp_has_head(http_t *h, char *head, char **ret)
{
	assert (h != NULL && head != NULL); 
	char *p; 
	bool t = search_head(&h->h_resp, head, &p); 
	if (ret) *ret = p ? strdup(p) : p; 
	return t; 
}

int http_status(http_t *h)
{
	assert (h != NULL);
	return h->h_status; 
}

bool  add_head_for_req(http_t *h, char *hd, char *fmt, ...)
{
	assert (h != NULL && hd != NULL && fmt != NULL); 
	
	char *head = calloc(1, MAX_HEAD_LENGTH + 1); 
	if (!head) return false; 
	int wnt = sprintf(head, "%s: ", hd); 
	va_list arg; 
	va_start(arg, fmt); 
	vsnprintf(head + wnt, MAX_HEAD_LENGTH - wnt, fmt, arg); 
	va_end(arg); 
	size_t len = strlen(head); 
	if (head[--len] == '\n') head[len] = '\0'; 
	if (head[--len] == '\r') head[len] = '\0'; 

	if (delete_head_for_req(h, (const char *)hd)) {
		add_head(&h->h_req, head); 
		return true; 
	}

	push_head(&h->h_req, head); 
	return true; 
}

void may_add_head_for_req(http_t *h, char *hd, char *fmt, ...)
{
	assert (h != NULL && hd != NULL && fmt != NULL); 
	size_t len; 
	if (!search_head(&h->h_req, (const char *)hd, NULL)) {
		char *head = calloc(1, MAX_HEAD_LENGTH + 1); 
		assert (head != NULL); 
		int wnt = sprintf(head, "%s: ", hd); 
		va_list arg; 
		va_start(arg, fmt); 
		vsnprintf(head + wnt, MAX_HEAD_LENGTH - wnt, fmt, arg); 
		va_end(arg); 
		len = strlen(head); 
		if (head[--len] == '\n') head[len] = '\0'; 
		if (head[--len] == '\r') head[len] = '\0'; 
		push_head(&h->h_req, head); 
	}
}

void add_head(struct http_msg *m, char *hd)
{
	char **p = GET_HEADS(*m); 
	size_t l = m->used; 
	int i = 0; 
	if (!p) {
		push_head(m, hd); 
		return ; 
	}
	for (; i < l; i++, p++) {
		if (!*p) {
			*p = hd; 
			return; 
		}
	}
	if (i == l) push_head(m, hd); 
}

void push_head(struct http_msg *m, char *hd) 
{
	assert (m != NULL && hd != NULL); 

	size_t length = m->length;  
	size_t used = m->used;   
	if (length == 0) {
		m->heads = calloc(DEF_HD_CNT + 1, sizeof(char *)); 
		assert (m->heads != NULL); 
		length = DEF_HD_CNT; 
		m->length = length; 
		m->used = 0; 
	}
	if (used == length - 1) {
		size_t oldl = length; 
		length += DEF_HD_CNT; 
		m->length = length; 
		m->heads = realloc(m->heads, sizeof(char*)*(length + 1)); 
		memset(m->heads + used, 0, (length - oldl + 1)*(sizeof(char*))); 
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
		while (*p1) {
			free (*p1); 
			*p1++ = NULL; 
		}
		h->used = 0; 
	}
}

bool http_set_req(http_t *h, char *method, char *req)
{
	assert (h != NULL); 
	assert (method != NULL && req != NULL); 

	char *request = calloc(MAX_REQUEST_BUF + 1, sizeof(char)); 
	if (!request) return false; 
	if (GET_START(h->h_req)) free(GET_START(h->h_req)); 
	GET_START(h->h_req) = request; 
	snprintf(request, MAX_REQUEST_BUF, "%s %s %s", 
			method, req, HTTP_VERSION); 
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
		all += strlen(GET_HEADS(h->h_req)[i]) + 2; 
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
	/*
	 * 首先丢弃上次响应的数据
	 */
	discard_start(&h->h_resp); 
	discard_heads(&h->h_resp); 

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
		if (read == 1 || read == 2) {
			free(buf); 
			break; 
		}

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
	char buf[MAX_STRING + 1]; 
	size_t len = MAX_STRING; 
	assert(buf != NULL); 
	int ret = 0; 
	char *p; 
	ret = sock_peek(sock, buf, len); 
	if (ret <= 0) return -1; 
	/*
	 * 注意：如果服务器发送的首部单行长度大于MAX_STRING则
	 * 程序处理结果可能将出现错误！
	 */
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
	if (h->h_host) free(h->h_host); 
	free(h); 
}

void free_msg_data(struct http_msg *m)
{
	assert (m != NULL); 

	if (m->start) free(m->start); 
	if (m->heads) {
		char **p = m->heads; 
		while (*p) {
			free(*p); 
			p++; 
		}
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

const char **http_resp_heads(http_t *h)
{
	if (h == NULL) return NULL; 
	char **p1 = GET_HEADS(h->h_resp); 
	if (p1) {
		/* 响应的首部控制字符需要移除 */
		char **p = p1; 
		while (*p) {
			size_t cnt; 
			cnt = strlen(*p); 
			cnt--; 
			if ((*p)[cnt] == '\n') (*p)[cnt--] = '\0'; 
			if ((*p)[cnt] == '\r') (*p)[cnt] = '\0'; 
			p++; 
		}
	}
	return (const char **)p1; 
}

void print_http_heads(http_t *h, bool type)
{
	if (!h) return ; 
	const char **p = type == true ? http_resp_heads(h) : http_req_heads(h); 
	if (!p) return; 
	while (*p)  log_printf(LOG_VERBOS, "%s\n", *p++); 
}

bool delete_head_for_req(http_t *h, const char *hd)
{
	char *p; 
	if (search_head(&h->h_req, hd, &p)) {
		/* 有HD首部 */
		char **head = GET_HEADS(h->h_req); 
		if (!head) return false; 
		while (*head) {
			if (*head == p) {
				free(*head); 
				*head = 0; 
				return true; 
			}
			head++; 
		}
	}
	return false; 
}
