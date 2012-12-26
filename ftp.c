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
#include <assert.h>

#define FTP_CMD_BUF 128
#define DEF_BUF		512
static bool ftp_login(ftp_t *f, const char *u, const char *p); 
static int ftp_pasv(ftp_t *f); 
static int ftp_epsv(ftp_t *f); 
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
		ftp->conn = sock; 
	}
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
	ftp_command(f, "USER %s", u); 
	ftp_wait_reply(f); 
	if (ftp_status(f)/100 != 2) {
		if (ftp_status(f)/100 == 3) {
			ftp_command(f, "PASS %s", p);
			ftp_wait_reply(f); 
			if (ftp_status(f) != 2) return false; 
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
	strcat(buf, "\r\n"); 
	return sock_write(ftp->conn, buf, strlen(buf)); 
}
void ftp_wait_reply(ftp_t *ftp)
{
	char *msg; 
	size_t rnt; 
	size_t total = 0; 
	if (ftp->last_reply == NULL) {
		ftp->reply_len = DEF_BUF; 
		ftp->last_reply = malloc (ftp->reply_len + 1); 
		assert (ftp->last_reply != NULL); 
	} 
	msg = ftp->last_reply; 
	while ((rnt = sock_read(ftp->conn, msg + total, ftp->reply_len - total))) {
		if (rnt + total > ftp->reply_len -(DEF_BUF >> 2)) {
			ftp->reply_len += DEF_BUF; 
			ftp->last_reply = realloc(ftp->last_reply, ftp->reply_len); 
			msg = ftp->last_reply; 
			assert (msg != NULL); 
		}
		total += rnt; 
	}
	if (rnt == -1) {
		log_debug(LOG_ERROR, "socket read ERROR!(%s)\n", strerror(errno)); 
		return; 
	}
	msg[total] = '\0'; 
	if (options.print_response)
		log_printf(LOG_VERBOS, "%s", msg); 
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
	if (ftp->data != NULL) sock_close(ftp->data); 
		/* 进入被动模式 */
	sockaddr_t *addr = sock_remote(ftp->conn); 
	switch (addr->ss_family) {
		case AF_INET:
			if (passive) {
				if (ftp_pasv(ftp) != NO_ERROR) {
					log_printf(LOG_WARNING, "Server don`t support PASSIVE mode! Try PORT\n"); 
					goto PORT; 
				}
			}
			break; 
		case AF_INET6:
			if (passive) {
				if (ftp_epsv(ftp) != NO_ERROR) {
					log_printf(LOG_WARNING, "Server don`t support PASSIVE mode! Try EPSV\n"); 
					goto PORT; 
				}
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
	assert (f != NULL); 
	char *reply = ftp_mesg(f); 
	int info[6] = {0}; 
	char ip[16]; 
	uint16_t port; 
	int i = 0; 
	for (; reply[i]; i++) {
		if (sscanf(reply + i, "%i,%i,%i,%i,%i,%i", 
					info, info + 1, info + 2, info + 3, 
					info + 4, info + 5) == 6) {
			sprintf(ip, "%d.%d.%d.%d", info[0], info[1], 
					info[2], info[3]); 
			port = (info[4] << 4) + info[5]; 
			assert (port >= 0); 
			break; 
		}
		return FTP_ERR; 
	}

	{
		socket_t *data; 
		struct sockaddr_in addr; 
		addr.sin_family = AF_INET; 
		inet_pton(AF_INET, ip, &addr.sin_addr); 
		data = sock_open_ip((sockaddr_t *)&addr, port); 
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
		log_printf(LOG_ERROR, "FTP PORT command FAILED!(server response:%s)",\
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
	if (ftp_data_socket(f, sock_local(f->conn)) != NO_ERROR) return FTP_ERR; 
	int af; 
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
				while (*p) if (*p == '.') *p++ = ','; 
				sprintf(buf + len, ",%d,%d", port >> 8, port & 0x00ff); 
				FTP_PORT("PORT"); 
			}
			break; 
		case AF_INET6:
			/* 远端为IPv6网络 */
			{
				sockaddr_t *addr = sock_local(f->conn); 
				char ip[128] = {0}; 
				int af; 
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
