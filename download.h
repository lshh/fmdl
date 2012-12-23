/*
 * =====================================================================================
 *
 *       Filename:  download.h
 *
 *    Description:  协议相关
 *
 *        Version:  1.0
 *        Created:  2012年12月10日 14时00分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef  __DOWNLOAD_H

#define  __DOWNLOAD_H

#include "url.h"
typedef enum {
	F_HTML, 		/* HTML 文件 */
	F_CSS, 			/* CSS 文件` */
	F_TEXT, 		/* 普通文本文件 */
	F_MEDIA			/* 多媒体文件 */
} file_type_t;

typedef struct _dl_t {
	url_t *url_parsed; 
	char *orig_url; 
	bool recuresive; 
	char *proxy; 
	char *redict_url; 
	char *referer; 
	int  err_code; 
	char *local; 		//本地文件
	file_type_t ft; 
	char ext[0]; 
} dl_t;

typedef struct _proto_t {
	scheme_t scheme; 
	dl_t *(*init_fun)(url_t*, char*); 
	dl_t *(*dl_main)(dl_t*); 
	void (*end_fun)(dl_t*); 
} proto_t;

dl_t *start_download(url_t *ut, char *orig_url); 
void recursive_download(url_t *ut, char *orig_url); 
#endif /* end of include guard: __DOWNLOAD_H */
