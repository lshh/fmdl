/*
 * =====================================================================================
 *
 *       Filename:  ftp.c
 *
 *    Description:  FTP接口实现
 *
 *        Version:  1.0
 *        Created:  2012年12月26日 13时07分18秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "ftp.h"
#include "error_code.h"
#include "fmdl.h"
#include "net.h"

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#define FTP_CMD_BUF 128
#define DEF_BUF		512

static bool ftp_login(ftp_t *f, const char *u, const char *p); 
static int ftp_pasv(ftp_t *f); 
static int ftp_epsv(ftp_t *f); 
static int ftp_data_socket(ftp_t *f, sockaddr_t *addr); 
static int ftp_accept(ftp_t *f); 

ftp_t *ftp_new(const char *user, const char *passwd)
{
	ftp_t *new = calloc(1, sizeof(ftp_t)); 
	assert(new != NULL); 
	new->user = !user ? strdup("Anonymous") : strdup(user); 
	new->passwd = !passwd ? strdup("Guess") : strdup(passwd); 
	return new; 
}

bool ftp_connect(ftp_t *ftp, const char *host, const uint16_t port)
{
	assert(ftp != NULL); 
	assert (host != NULL && port >= 0); 
	socket_t *sock = sock_open_host(host, port); 
	if (!sock) return false; 
	log_debug(LOG_VERBOS, "Connect to %s SUCCESS!\n", host); 
	if (ftp->conn) {
		sock_close(ftp->conn); 
		ftp->conn = NULL; 
	}
	ftp->conn = sock; 
	if (!ftp_login(ftp, ftp->user, ftp->passwd)) {
		log_printf(LOG_ERROR, "FTP: login Failed!(Server response:%s)\n", ftp_mesg(ftp)); 
		return false; 
	}

	ftp_command(ftp, "SYST"); 
	ftp_wait_reply(ftp); 
	if (ftp_status(ftp)/100 != 2) {
		log_debug(LOG_WARNING, "FTP Command \"SYST\"FAILED!\n"); 
		return false; 
	}

	/* 使用binary模式 */
	ftp_command(ftp, "TYPE I"); 
	ftp_wait_reply(ftp); 
	if (ftp_status(ftp)/100 != 2) {
		log_debug(LOG_WARNING, "FTP Command \"TYPE I\"FAILED!\n"); 
		return false; 
	}
	return true; 
}

bool ftp_login(ftp_t *f, const char *u, const char *p)
{
	assert (f != NULL); 
	assert (u != NULL && p != NULL); 
	/* 等待服务器信息 */
	ftp_wait_reply(f); 

	ftp_command(f, "USER %s", u); 
	ftp_wait_reply(f); 
	if (ftp_status(f)/100 != 2) {
		if (ftp_status(f)/100 == 3) {
			ftp_command(f, "PASS %s", p);
			ftp_wait_reply(f); 
			if (ftp_status(f)/100 != 2) return false; 
			return true; 
		} else 
			return false; 
	}
	return false; 
}
int ftp_command(ftp_t *ftp, char *fmt, ...)
{
	assert (ftp != NULL && fmt != NULL); 
	char buf[FTP_CMD_BUF + 1] = {0}; 
	va_list args; 
	va_start(args, fmt); 
	vsnprintf(buf, FTP_CMD_BUF, fmt, args); 
	va_end(args); 
	strcat(buf, "\r\n"); 
	return sock_write(ftp->conn, buf, strlen(buf)); 
}
void ftp_wait_reply(ftp_t *ftp)
{
	char *msg; 
	size_t rnt; 
	if (ftp->last_reply == NULL) {
		ftp->reply_len = DEF_BUF; 
		ftp->last_reply = malloc(ftp->reply_len + 1); 
		assert (ftp->last_reply != NULL); 
	} 
	msg = ftp->last_reply; 
	rnt = sock_read(ftp->conn, ftp->last_reply, ftp->reply_len); 
	if (rnt == -1) {
		log_debug(LOG_ERROR, "socket read ERROR!(%s)\n", strerror(errno)); 
		return; 
	}
	msg[rnt] = '\0'; 
	if (options.print_response) 
		log_printf(LOG_VERBOS, "%s", ftp_mesg(ftp)); 

	sscanf(msg, "%d", &ftp->status); 
}
int ftp_status(ftp_t *ftp)
{
	return ftp->status; 
}
void ftp_close(ftp_t *ftp)
{
	if (ftp->conn) sock_close(ftp->conn); 
	if (ftp->data) sock_close(ftp->data); 
	if (ftp->last_reply) free(ftp->last_reply); 
	if (ftp->user) free(ftp->user); 
	if (ftp->user) free(ftp->passwd); 
	memset(ftp, 0, sizeof(ftp_t)); 
}
void ftp_destroy(ftp_t *ftp)
{
	assert(ftp != NULL); 
	ftp_close(ftp); 
	free(ftp); 
}
char *ftp_mesg(ftp_t *ftp)
{
	return ftp->last_reply; 
}
int ftp_data(ftp_t *ftp, bool passive)
{
	assert (ftp != NULL); 
	int err = NO_ERROR; 
	if (ftp->data != NULL) {
		sock_close(ftp->data); 
		ftp->data = NULL; 
	}
	sockaddr_t *addr = sock_remote(ftp->conn); 
	switch (addr->ss_family) {
		case AF_INET:
			if (passive) {
				if (ftp_pasv(ftp) != NO_ERROR) {
					log_printf(LOG_WARNING, "Server don`t support PASSIVE mode! Try PORT\n"); 
					goto PORT; 
				}
				return NO_ERROR; 
			}
			break; 
		case AF_INET6:
			if (passive) {
				if (ftp_epsv(ftp) != NO_ERROR) {
					log_printf(LOG_WARNING, "Server don`t support PASSIVE mode! Try EPRT\n"); 
					goto PORT; 
				}
				return NO_ERROR; 
			}
			break; 
		default:
			log_debug(LOG_ERROR, "Unknown net type!\n"); 
			err = ERR_NET; 
			return err; 
	}
PORT:
	err = ftp_port(ftp); 
	return err; 
}
int ftp_pasv(ftp_t *f)
{
	assert (f != NULL && f->conn != NULL); 
	char *reply; 
	int info[6] = {0}; 
	char ip[16]; 
	uint16_t port; 
	bool ok = false; 
	int i = 0; 

	if (f->data != NULL) {
		sock_close(f->data); 
		f->data = NULL; 
	}

	ftp_command(f, "PASV"); 
	ftp_wait_reply(f); 
	if (ftp_status(f)/100 != 2) return FTP_ERR; 

	reply = ftp_mesg(f); 

	for (; reply[i]; i++) {
		if (sscanf(reply + i, "%i,%i,%i,%i,%i,%i", 
					info, info + 1, info + 2, info + 3, 
					info + 4, info + 5) == 6) {
			sprintf(ip, "%d.%d.%d.%d", info[0], info[1], 
					info[2], info[3]); 
			port = (info[4] << 8) + info[5]; 
			assert (port >= 0); 
			ok = true; 
			break; 
		} 
	}

	if (!ok) return FTP_ERR; 

	{
		socket_t *data; 
		struct sockaddr_in addr; 
		addr.sin_family = AF_INET; 
		inet_pton(AF_INET, ip, &addr.sin_addr); 
		data = sock_open_ip((sockaddr_t *)&addr, port); 
		if (data == NULL) return FTP_ERR; 
		f->data = data; 
	}
	return NO_ERROR; 
}

int ftp_port(ftp_t *f)
{
#define FTP_PORT(cmd)	do {\
	ftp_command(f, cmd" %s", buf); \
	ftp_wait_reply(f); \
	if (ftp_status(f)/100 != 2) {\
		log_printf(LOG_ERROR, "FTP PORT command FAILED!(server response: %s )",\
				ftp_mesg(f)); \
		return FTP_ERR; \
	}\
} while (0)
#define ADDR_TRANSPORT(type, v) do {\
	type *v = (type *)sock_local(f->data); \
	inet_ntop(v->v##_family, &v->v##_addr, ip, sizeof(ip)); \
	port = ntohs(v->v##_port); \
	af = v->v##_family == AF_INET ? 1 : 2; \
} while (0)
	assert (f != NULL); 
	char buf[DEF_BUF + 1] = {0}; 
	uint16_t port; 
	int af; 
	if (ftp_data_socket(f, sock_local(f->conn)) != NO_ERROR) 
		return FTP_ERR; 
	sockaddr_t *remote = sock_remote(f->conn); 
	af = remote->ss_family; 
	switch (af) {
		case AF_INET:
			/* 远端为IPv4网络 */
			{
				char *p; 
				/* 如果远端为IPv4网络则可以保证
				 * 本地亦为IPv4
				 */
				struct sockaddr_in *sin = (struct sockaddr_in *)sock_local(f->data); 
				size_t len; 
				port = ntohs(sin->sin_port); 
				inet_ntop(sin->sin_family, &sin->sin_addr, buf, DEF_BUF); 
				p = buf; 
				len = strlen(buf); 
				while (*p) {
					if (*p == '.'){
						*p = ','; 
					}	
					p++; 
				}
				sprintf(buf + len, ",%d,%d", port >> 8, port & 0x00ff); 
				FTP_PORT("PORT"); 
			}
			break; 
		case AF_INET6:
			/* 远端为IPv6网络 */
			{
				sockaddr_t *addr = sock_local(f->conn); 
				char ip[128] = {0}; 
				int af = addr->ss_family == AF_INET ? 1 : 2; 
				if (addr->ss_family == AF_INET) 
					ADDR_TRANSPORT(struct sockaddr_in, sin); 
				if (addr->ss_family == AF_INET6)
					ADDR_TRANSPORT(struct sockaddr_in6, sin6); 
				sprintf(buf, "|%d|%s|%d", af, ip, port); 
				FTP_PORT("EPRT"); 
			}
			break; 
		default:
			return FTP_ERR; 
	}
	return ftp_accept(f); 
#undef FTP_PORT
#undef ADDR_TRANSPORT
}
int ftp_data_socket(ftp_t *f, sockaddr_t *addr)
{
#define CREAT_SOCKET(type, v) do {\
	type v = *(type *)addr; \
	v.v##_port = htons(ntohs(v.v##_port) + 1); \
	socklen_t l = sizeof(v); \
	ret = socket(v.v##_family, SOCK_STREAM, 0); \
	if (ret < 0) return FTP_ERR; \
	ret = bind(ret, (struct sockaddr*)&v, l); \
	listen(ret, 1); \
	if (ret < 0) return FTP_ERR; \
	sock->fd = ret; \
	sock->tm = time(NULL); \
	sock->status = NO_ERROR; \
	sock->local = *((sockaddr_t*)&v); \
	f->data = sock; \
} while (0)
	assert (f != NULL && addr != NULL); 
	int ret; 
	if (f->data != NULL) {
		sock_close(f->data); 
		f->data = NULL; 
	}
	socket_t *sock = calloc(1, sizeof(socket_t)); 

	switch (addr->ss_family) {
		case AF_INET:
			CREAT_SOCKET(struct sockaddr_in, sin); 
			break; 
		case AF_INET6:
			CREAT_SOCKET(struct sockaddr_in6, sin6); 
			break; 
		default:
			return FTP_ERR; 
	}
	return NO_ERROR; 
#undef CREAT_SOCKET
}
int ftp_accept(ftp_t *f)
{
	assert (f != NULL && f->data != NULL); 

	socklen_t len; 
	int ret; 
	ret = accept(f->data->fd, (struct sockaddr *)sock_remote(f->data), &len); 
	if (ret < 0) return FTP_ERR; 

	log_debug(LOG_VERBOS, "Accept connection\n"); 
	return NO_ERROR; 
}

int ftp_epsv(ftp_t *f)
{
	assert (f != NULL && f->conn != NULL); 

	if (f->data != NULL) {
		sock_close(f->data); 
		f->data = NULL; 
	}
	ftp_command(f, "EPSV %s", (sock_local(f->conn))->ss_family == AF_INET? "1":"2"); 
	ftp_wait_reply(f); 
	if (ftp_status(f)/100 != 2) {
		return FTP_ERR; 
	}

	char *reply = ftp_mesg(f); 
	char *p = strchr(reply, '('); 
	char delim; 

	if (p == NULL) return FTP_ERR; 
	delim = *++p; 
	if (delim < 33 || delim > 126) return FTP_ERR; 

	p++; 
	int i = 0; 
	for (; i < 2; i++) {
		if (*p++ != delim) return FTP_ERR; 
	}
	
	/* 解析端口号 */
	uint16_t port; 
	while (isdigit(*p)) {
		port = port*10 + (*p++ - '\0'); 
	}
	f->data = sock_open_ip(sock_remote(f->conn), port); 
	if (f->data == NULL) return FTP_ERR; 
	return NO_ERROR; 
}

long long int ftp_size(ftp_t *f, const char *file, int max_redir)
{
	assert (f != NULL && f->conn != NULL); 
	assert (file != NULL); 
	char *pfile = strdup(file); 
	int status; 
	long long size; 
LINK:
	/*
	 * 首先使用SIZE命令
	 */
	ftp_command(f, "SIZE %s", pfile); 
	ftp_wait_reply(f); 

	status = ftp_status(f)/100; 
	if (status == 2) {
		sscanf (ftp_mesg(f), "%*i %lld", &size); 
		return size; 
	} else if (status == 5) {
		/*
		 * 未找到文件
		 */
		return -1; 
	}
	if (max_redir < 0) {
		log_printf(LOG_ERROR, "Too many redirect in FTP!\n"); 
		return -1; 
	}
	/*
	 * 尝试使用LIST命令
	 */
	if (ftp_data(f, options.passive) != NO_ERROR) {
		log_printf(LOG_ERROR, "Open FTP data connection FAILED!(SERVER:%s)\n",
			   	ftp_mesg(f)); 
		return -1; 
	}
	ftp_command(f, "LIST %s", pfile); 
	ftp_wait_reply(f); 
	status = ftp_status(f)/100; 
	if (status != 1) {
		return -1; 
	}
	/*
	 * 读取LIST数据并提取文件大小字段
	 */
	{
		char *list = malloc(DEF_BUF + 1); 
		size_t len = DEF_BUF; 
		size_t total = 0; 
		ssize_t rnt = 0; 
		assert (list != NULL); 
		while ((rnt = sock_read(f->data, list + total, len - total))) {
			if (rnt + total >= len - 1) {
				len += DEF_BUF; 
				list = realloc(list, len + 1); 
				assert (list != NULL); 
			}
			total += rnt; 
	}

		list[total] = '\0'; 
		sock_close(f->data); 

		ftp_wait_reply(f); 
		status = ftp_status(f)/100; 
		if (status != 2) {
			free(list); 
			log_printf(LOG_ERROR, "FTP reply unknown!(SERVER:%s)\n", ftp_mesg(f)); 
			return -1; 
		}
		char *p = list + total; 
		char *delim; 
		if (p[-1] == '\n') {
			delim = p[-2] == '\r' ? p - 2 : p - 1; 
		}
		p = strstr(list, delim); 
		p = strstr(p + strlen(delim), delim); 
		if (p) {
			/* 有多行数据 */
			free (list); 
			log_printf(LOG_ERROR, "List file reply is multi line!\n"); 
			return -1; 
		}
		p = list; 
		if (*p == '-') {
			/* 普通文件 直接获取文件大小字段*/
			sscanf(list, "%*s %*d %*s %*s %lld %*s %*s %*s %*s", &size); 
			return size; 

		} else if (*p == 'l') {
			/* 链接文件 */
			p = strstr(list, "->"); 
			if (!p) {
				log_printf(LOG_ERROR, "GET orignal filename from link FAILED!"); 
				free (list); 
				return -1; 
			}
			p += 3; 
			memset(delim, 0, strlen(delim)); 
			free(pfile); 
			pfile = strdup(p); 
			max_redir--; 
			goto LINK; 
		} else {
			free (list); 
			log_printf(LOG_ERROR, "%s is NOT a file!\n", pfile); 
			return -1; 
		}
	}
}
