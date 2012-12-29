/*
 * =====================================================================================
 *
 *       Filename:  ftp.h
 *
 *    Description:  FTP封装
 *
 *        Version:  1.0
 *        Created:  2012年12月26日 12时18分52秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef FTP_75I7EGWM

#define FTP_75I7EGWM

#include "net.h"

typedef struct _ftp_t {
	socket_t *conn, *data; 
	int status; 	/* 服务器状态码 */
	char *last_reply; /* 服务器响应 */
	size_t reply_len; /* 缓冲区大小 */
	char *user, *passwd; 
} ftp_t;
/*
 * 新建一个FTP_T结构
 */
ftp_t *ftp_new(const char *user, const char *passwd); 
/*
 * 建立FTP控制连接
 */
bool ftp_connect(ftp_t *ftp, const char *host, const uint16_t port); 
/*
 * 发送FTP命令
 */
int ftp_command(ftp_t *ftp, char *fmt, ...); 
/*
 * 等待FTP服务器响应
 */
void ftp_wait_reply(ftp_t *ftp); 
/*
 * 返回服务器状态码
 */
int ftp_status(ftp_t *ftp); 
/*
 * 关闭FTP的所有链接, 销毁相关信息
 */
void ftp_close(ftp_t *ftp); 
/*
 * 释放FTP_T结构
 */
void ftp_destroy(ftp_t *ftp); 
/*
 * 打开FTP数据连接, 连接信息填写在ftp->data中
 */
int ftp_data(ftp_t *ftp, bool passive); 
/*
 * 使用PORT指令打开数据连接
 */
int ftp_port(ftp_t *f); 
/*
 * 获取FILE文件的大小
 */
long long int ftp_size(ftp_t *f, const char *file, int max_redir); 
/*
 * 答应最近一次的服务器响应
 */
char *ftp_mesg(ftp_t *ftp); 
#endif /* end of include guard: FTP_75I7EGWM */

