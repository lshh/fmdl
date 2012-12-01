/*
 * =====================================================================================
 *
 *       Filename:  iri.h
 *
 *    Description:  本地及远端编码设置
 *
 *        Version:  1.0
 *        Created:  2012年12月01日 14时51分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef IRI_BNTWRZYC

#define IRI_BNTWRZYC

#include <stdio.h>
#include "error_code.h"

typedef struct iri
{
	char *url_encode; 	/* URL的编码方式 */
	char *content_encode; 	/* 正文编码方式 */
	bool  enable_utf8; 		/* 是否启用UTF-8编码 */
}iri_t; 
/* ----------------接口------------------- */

/*
 * 创建新的IRI结构, 此函数应该被封装, 而不应该使用__new_iri
 */
iri_t *__new_iri(char *url_ecd, char*cnt_ecd, bool enable); 
/*
 * 复制一个IRI结构
 */
iri_t  *dup_iri(const iri_t *iri); 
/*
 * 设置URL 的编码方式
 */
void set_url_encode(const iri_t *iri, const char *charset); 
/*
 * 设置正文的编码方式
 */
void set_content_encode(const iri_t *iri, const char *charset); 
/*
 * 对域名编码
 */
char *encode(const iri_t *iri, const char *s); 
/*
 * 域名反编码
 */
char *disencode(const iri_t *iri, const char *s); 
/* 将S转换为UTF-8编码, 在N中返回 */
bool encode_to_utf8(const iri_t *iri, const char *s, char **n); 
/*
 * 销毁IRI
 */
void free_iri(iri_t *iri); 

#endif /* end of include guard: IRI_BNTWRZYC */
