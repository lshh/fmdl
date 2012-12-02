/*
 * =====================================================================================
 *
 *       Filename:  url.h
 *
 *    Description:  处理URL
 *
 *        Version:  1.0
 *        Created:  2012年11月30日 14时23分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef URL_7Q4G91JK

#define URL_7Q4G91JK

#include <stdio.h>
#include <assert.h>
#include "error_code.h"
#include <stdint.h>
#include "iri.h"
#include "log.h"
typedef enum {
	SCHEME_HTTP, 
	SCHEME_HTTPS, 
	SCHEME_FTP, 
	SCHEME_INVALID
} scheme_t; 
typedef struct url
{
	char *rebuild_url; 		/* 根据解析出来的信息重构的URL */
	scheme_t scheme; 		/* URL协议 */
	char *host; 			/* 主机 */
	uint16_t port; 			/* 端口 */

	char *path; 			/* 路径 */
	char *dir; 				/* 目录 */
	char *file; 			/* 文件 */
	char *parameters; 		/* 参数 */
	char *query; 			/* 查询 */
	char *fragment; 		/* 信息片段 */
	/*  */
	char *user; 			/* 用户名 */
	char *passwd; 			/* 密码 */
}url_t; 

url_t *url_parsed(char *url, iri_t *iri, bool encode_unsafe, int *error_code); 
char *rebuild_url(url_t*, bool hide_paswd); 
void url_delete(url_t *); 
char *url_path(url_t*); 
//
#endif /* end of include guard: URL_7Q4G91JK */
