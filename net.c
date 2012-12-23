/*
 * =====================================================================================
 *
 *       Filename:  net.c
 *
 *    Description:  网络接口
 *
 *        Version:  1.0
 *        Created:  2012年12月07日 12时46分34秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "net.h"
#include "fmdl.h"
#include "error_code.h"
#include "hash.h"
#include "log.h"
#include "dns.h"

#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <arpa/inet.h>
#include <stdio.h>
/* 网络状态码 */
enum {
	NET_OK, 
	NET_DNS_ERROR, 
	NET_BIND_FAIL, 
	NET_CONN_FAIL, 
	NET_BAD_IP
}; 
/* 套接字状态 */
enum {
	SOCK_TIMEOUT = 0x01, 
	SOCK_RD_ERROR, 
	SOCK_WR_ERROR, 
	SOCK_OK
}; 
static jmp_buf env; 
static void sig_callback(int sig); 
static int bind_local(int fd, const char *ip); 
static int connect_timeout(int fd, sockaddr_t *sa, uint32_t to); 
static int conn_cb(cb_arg_t *arg); 
bool valid_ipv4_addr(const char *s, size_t len)
{
	assert(s != NULL); 
	assert(len > 0); 

	int cnt = 0; 
	int sum = 0; 
	const char *p = s; 
	bool first = true; 
	while (*p && len--) {
		if (isdigit(*p)) {
		   	sum = sum*10 + *p - '0'; 
			first = false; 
		} else if (*p == '.' && first == false) {
			if (sum > 255) return false; 
			if (++cnt > 3) return false; 
			if (cnt == 3 && *(p + 1) == '\0') return false; 
			sum = 0; 
		} else 
			return false; 
		p++; 
	}
	if (cnt != 3 || sum > 255) return false; 
	return true; 
}
bool valid_ipv6_addr(const char *s, size_t len)
{
	assert(s != NULL); 
	assert(len > 0); 
	const char *p = s; 
	int bit_width = 16; 
	bool flag = false; 
	int sum = 0; 
	if (*p == ':' && *(p + 1) == ':') {
		bit_width += 16; 
		flag = true; 
		p += 2; 
		len -= 2; 
	} 
	//如果只有两个::
	if (len == 0) return false; 

	while (*p && len--) {
		if (is_digit(*p)) {
			if (isdigit(*p)) {
				sum = sum*16 + *p - '0'; 
				p++; 
				continue; 
			}
			char c = tolower(*p); 
			if (c < 'a' || c > 'f') return false; 
			sum = sum*16 + c - 'a' + 10; 
		} else if (*p == ':') {
			bit_width += 16; 
			if (sum > 0xffff) return false; 
			if (*(p + 1) == ':' && !flag) {
				flag = true; 
				p += 2; 
				if (*p == ':') return false; 
				sum = 0; 
				continue; 
			}else if (*(p + 1) == ':' && flag) 
				return false; 
			sum = 0; 
		} else if (*p == '.') {
			char *b = p; 
			while (*b != ':' && b != s) b--; 
			if (b == s) return false; 
			++b; 
			if (!valid_ipv4_addr(b, strlen(b))) return false; 
			bit_width += 16; 
			break; 
		} else 
			return false; 
		p++; 
	}
	if (flag && bit_width > 128) return false; 
	if (!flag && bit_width != 128) return false; 
	return true; 
}
bool is_digit(char c)
{
	switch(c) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F':
			return true; 
		default:
			return false; 
	}
	abort(); 
}
socket_t *sock_open_host(const char *host, uint16_t port, net_type_t net)
{
	assert(host != NULL && port >= 0); 
	addrlists_t *al = lookup_host(host); 
	socket_t *sock; 
	int cnt; 
	sockaddr_t *addr; 
	if (al == NULL) {
		log_debug(LOG_ERROR, "Can`t get IP of host(%s)\n", host); 
		return NULL; 
	}
	int i = 0; 
	for (; i < cnt; i++) {
		addr = (sockaddr_t *) addrlist_pos(al, i); 
		sock = sock_open_ip(addr, port); 
		if (sock == NULL) 
			continue; 
		return sock; 
	}
	if (i == cnt) {
		log_debug(LOG_ERROR, "Can`t connect to host(%s)", host); 
		return NULL; 
	}
}
socket_t *sock_open_ip(sockaddr_t *addr, uint16_t port)
{
#define TRY_CONN(type, v)	do{\
	type *v = (type*)addr; \
	(v)->v##_port = htons(port); \
	fd = socket((v)->v##_family, SOCK_STREAM, 0); \
	if (fd < 0) {\
		log_debug(LOG_ERROR, "Can`t creat socket!\n"); \
		return NULL; \
	}\
	if (options.connect_timeout) {\
		int ret = bind_local(fd, options.bind_addr);\
		if (ret != NET_OK){\
			log_debug(LOG_WARNING, "Can`t bind to %s!Ignore it!\n", options.bind_addr); \
		} else {\
			log_debug(LOG_VERBOS, "Bind to %s\n", options.bind_addr); \
		}\
	}\
	log_debug(LOG_VERBOS, "Try to connect to %s:%d\n", \
			inet_ntop((v)->v##_family, &(v)->v##_addr, \
				ip, sizeof(ip)), port); \
	int ret = connect_timeout(fd, addr, options.connect_timeout); \
	if (ret != NET_OK) {\
		close(fd); \
		log_debug(LOG_ERROR, "Can`t connect to %s:%d\n", ip, port); \
		return NULL; \
	}\
	socklen_t len; \
	sock = malloc(sizeof(socket_t)); \
	assert(sock != NULL); \
	sock->fd = fd; \
	sock->status = NET_OK; \
	sock->remote = *addr; \
	memset(&sock->local, 0, sizeof(sockaddr_t)); \
	getsockname(addr->ss_family, (SOCKADDR*)&sock->local, &len); \
	sock->tm = time(NULL); \
	sock->r_bytes = sock->w_bytes = 0; \
	return sock; \
} while (0)
	assert (addr != NULL && port >= 0); 
	int fd; 
	char ip[129] = {0}; 
	socket_t *sock; 
	switch (addr->ss_family) {
		case AF_INET:
			TRY_CONN(struct sockaddr_in, sin); 
			break; 
		case AF_INET6:
			TRY_CONN(struct sockaddr_in6, sin6); 
			break; 
	}
	return NULL; 
#undef TRY_CONN
}
int bind_local(int fd, const char *ip)
{
#define BIND_ADDR(net, type, v) do{\
	type v; \
	(v).v##_port = hntos(0); \
	(v).v##_family = net; \
	inet_pton((v).v##_family, ip, (void*)&(v).v##_addr); \
	ret = bind(fd, (SOCKADDR*)&v, sizeof(v)); \
	if (ret != 0) return NET_BIND_FAIL; \
	return NET_OK; \
} while(0)
	assert (fd >= 0 && ip != NULL); 
	size_t len = strlen(ip); 
	int ret; 
	if (valid_ipv4_addr(ip, len)) {
		BIND_ADDR(AF_INET, struct sockaddr_in, sin); 
	} else if (valid_ipv6_addr(ip, len)) {
		BIND_ADDR(AF_INET6, struct sockaddr_in6, sin6); 
	}
	return NET_BAD_IP; 
#undef BIND_ADDR
}
int connect_timeout(int fd, sockaddr_t *sa, uint32_t to) 
{
	assert(fd >= 0); 
	assert(sa != NULL); 
	int ret; 
	cb_arg_t cb; 
	cb.cb_fd = fd; 
	cb.cb_ext = (void*)sa; 
	ret = run_cb_with_timeout(conn_cb, &cb, to); 
	if (ret == NO_ERROR) return NET_OK; 
	return NET_CONN_FAIL; 
}
int conn_cb(cb_arg_t *arg)
{
	int fd = arg->cb_fd; 
	sockaddr_t *sa = (sockaddr_t *)arg->cb_ext; 
	ret = connect(fd, (SOCKADDR*)sa, sizeof(SOCKADDR)); 
	if (ret != 0) return NET_CONN_FAIL; 
	return NO_ERROR; 
}
int run_cb_with_timeout(int (*fun)(cb_arg_t*), cb_arg_t *arg, uint32_t to) 
{
	assert(fun != NULL); 
	int ret; 
	if (setjmp(env) != 0) {
		signal(SIGALRM, SIG_DFL); 
		return ERR_TIMEOUT; 
	}
	if (to == 0) to = 0xffffffff; 
	signal(SIGALRM, sig_callback); 
	alarm(to); 
	ret = fun(arg); 
	alarm(0); 
	signal(SIGALRM, SIG_DFL); 
	return ret; 
}
void sig_callback(int sig)
{
	if (sig == SIGALRM)
		longjmp(env, 2); 
}
time_t sock_alive_time(socket_t *sock)
{
	assert(sock != NULL); 
	time_t tm = time(NULL); 
	return tm - sock->tm; 
}
void sock_close(socket_t *sock)
{
	assert(sock != NULL); 
	int i = 3; 
	while (i--) if (close(fd) == 0) break; 
	free (sock); 
}
int sock_read(socket_t *sock, char *buf, size_t len)
{
	assert (sock != NULL); 
	assert (buf != NULL && len > 0); 
	int ret; 
	if (sock->fd < 0) return -1; 
	ret = fd_select(sock->fd, WAIT_READ, options.read_timeout); 
	if (ret != FD_CAN_READ) {
		sock->status = SOCK_TIMEOUT; 
		return -1; 
	}
	ret = read(sock->fd, buf, len); 
	if (ret < 0) {
		sock->status = SOCK_RD_ERROR; 
		return -1; 
	}
	sock->r_bytes += ret; 
	return ret; 
}
int sock_write(socket_t *sock, char *buf, size_t len)
{
	assert(sock != NULL); 
	assert(buf != NULL && len > 0); 
	int ret; 
	if (sock->fd < 0) return -1; 
	ret = fd_select(sock->fd, WAIT_WRITE, 0); 
	if (ret != FD_CAN_WRITE) {
		sock->status = SOCK_TIMEOUT; 
		return -1; 
	}
	ret = write(sock->fd, buf, len); 
	if (ret < 0) {
		sock->status = SOCK_WR_ERROR; 
		return -1; 
	}
	sock->w_bytes += ret; 
	return ret; 
}
int sock_peek(socket_t *sock, char *buf, size_t len)
{
	assert(sock != NULL); 
	assert(buf != NULL && len > 0); 
	int ret; 
	if (sock->fd < 0) return -1; 
	ret = fd_select(sock->fd, WAIT_READ, options.read_timeout); 
	if (ret != FD_CAN_READ) {
		sock->status = SOCK_TIMEOUT; 
		return -1; 
	}
	ret = recv(sock->fd, buf, len, MSG_PEEK); 
	return ret; 
}
