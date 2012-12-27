/*
 * =====================================================================================
 *
 *       Filename:  net.h
 *
 *    Description:  处理网络链接模块
 *
 *        Version:  1.0
 *        Created:  2012年12月07日 19时58分54秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef NET_D29XEW7D

#define NET_D29XEW7D

#include "error_code.h"
#include "log.h"

#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <netdb.h>

typedef enum {
	ONLY_IPV4 = 0x01,  /* 只使用IPv4网络 */
	ONLY_IPV6 = 0x02, /* 只使用IPv6网络 */
	ALL_IP = 0x04	 /* 系统自动选择网络 */
}net_type_t; 
/* 通用地址结构 */
typedef struct sockaddr_storage sockaddr_t; 
bool is_digit(char c); 

bool valid_ipv6_addr(const char *s, size_t len); 
bool valid_ipv4_addr(const char *s, size_t len); 
/* **************处理网络连接************* */
/* 套接字状态 */
enum {
	SOCK_TIMEOUT = 0x01, 
	SOCK_RD_ERROR, 
	SOCK_WR_ERROR, 
	SOCK_OK 
}; 
typedef struct _socket_t {
	int fd; 		/* 套接字描述符 */
	int status; 	/* 网络状态 */
	sockaddr_t remote; 	/* 远端地址 */
	sockaddr_t local; 
	time_t tm; 			/* 网络连接创建时间 */
	uint32_t r_bytes; 	/* 已接受的字节数 */
	uint32_t w_bytes; 	/* 已发送的字节数 */
} socket_t;
#define sock_remote(sock) &(sock)->remote
#define sock_local(sock) &(sock)->local
#define sock_read_bytes(sock) (sock)->r_bytes
#define sock_write_bytes(sock) (sock)->r_bytes 
#define sock_status(sock) (sock)->status
#define set_sock_status(sock, st) (sock)->status = (st)
socket_t *sock_open_host(const char *host, uint16_t port); 
socket_t *sock_open_ip(sockaddr_t *addr, uint16_t port); 
//const char *sock_status_str(socket_t *sock); 
time_t sock_alive_time(socket_t *sock); 
int sock_read(socket_t *sock, char *buf, size_t len); 
int sock_write(socket_t *sock, char *buf, size_t len); 
int sock_peek(socket_t *sock, char *buf, size_t len); 
const char *get_sock_ip(const sockaddr_t *addr); 
const char *get_sock_port(const sockaddr_t *addr); 
void sock_close(socket_t *sock); 

typedef struct _cb_arg_t {
	int cb_fd; 
	void *cb_ext; 
} cb_arg_t;
int run_cb_with_timeout(int (*fun)(cb_arg_t*), cb_arg_t *arg, uint32_t to); 
#endif
