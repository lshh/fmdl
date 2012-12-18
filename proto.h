/*
 * =====================================================================================
 *
 *       Filename:  proto.h
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
#ifndef PROTO_LJEBKEMW

#define PROTO_LJEBKEMW

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
	char *local; 
	file_type_t ft; 
	char ext[0]; 
} dl_t;

typedef struct _proto_t {
	scheme_t scheme; 
	dl_t *(*init_fun)(url_t*, char*); 
	dl_t *(*dl_main)(dl_t*); 
	void (*end_fun)(dl_t*); 
} proto_t;
#endif /* end of include guard: PROTO_LJEBKEMW */
