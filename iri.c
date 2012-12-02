/*
 * =====================================================================================
 *
 *       Filename:  iri.c
 *
 *    Description:  编码模块接口实现
 *
 *        Version:  1.0
 *        Created:  2012年12月01日 15时49分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "iri.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <iconv.h>
#include <stdlib.h>
#include <assert.h>
#include <idna.h>
#include <errno.h>

enum encode_type {SET_URL_ENCODE, SET_CONTENT_ENCODE}; 
static void set_encode(const iri_t *iri, const char *charset, enum encode_type flag); 
static bool convert(iconv_t cd, char*src, size_t len, char **n); 

iri_t *__new_iri(char *url_ecd, char *cnt_ecd, bool enable)
{
	iri_t *new = malloc(sizeof(iri_t)); 
	if (new == NULL) {
		log_puts(LOG_ERROR, "malloc a new iri error!"); 
		return NULL; 
	}
	new->url_encode = url_ecd; 
	new->content_encode = cnt_ecd; 
	new->enable_utf8 = enable; 
	return new; 
}
iri_t  *dup_iri(const iri_t *iri)
{
	assert(iri != NULL); 
	assert(iri->url_encode != NULL); 
	assert(iri->content_encode != NULL); 

	return __new_iri(strdup(iri->url_encode), 
					 strdup(iri->content_encode),
					  iri->enable_utf8
					); 
}
void set_url_encode(const iri_t *iri, const char *charset)
{
	assert(iri != NULL); 
	assert(charset != NULL); 

	set_encode(iri, charset, SET_URL_ENCODE); 
}
void set_content_encode(const iri_t *iri, const char *charset)
{
	assert(iri != NULL); 

	set_encode(iri, charset, SET_CONTENT_ENCODE); 
}
static void set_encode(const iri_t *iri, const char *charset,
		enum encode_type flag)
{
	DEBUGINFO(("Set URL encode to %s!The old encode is %s\n", charset , iri->url_encode)); 
	char **s; 
	switch (flag) {
		case SET_CONTENT_ENCODE:
			s = &(iri->content_encode); 
			break; 
		case SET_URL_ENCODE:
			s = &(iri->url_encode); 
			break; 
		default:
			abort(); 
	}
	if (*s) {
		if (charset && strcasecmp(charset, *s) == 0) return; 
		free (*s); 
	}
	*s = charset?strdup(charset):NULL; 
}
char *encode(const iri_t *iri, const char *s)
{
	assert(iri != NULL); 
	assert(s != NULL); 

	char *new; 
	if (iri->enable_utf8 == false) {
		if (!encode_to_utf8(iri, s, &new)) return NULL; 
		s = new; 
	}
	int ret = idna_to_ascii_8z(s, &new, IDNA_USE_STD3_ASCII_RULES); 
	if (ret != IDNA_SUCCESS) {
		log_printf(LOG_VERBOS, "idn_encode failed(%d):%s\n", ret, idna_strerror(ret)); 
		return NULL; 
	}
	return new; 
}

char *disencode(const iri_t *iri, const char *s)
{
	assert(iri != NULL); 
	assert(s != NULL); 

	char *new; 
	int ret; 
	ret = idna_to_unicode_8zlz(s, &new, IDNA_USE_STD3_ASCII_RULES); 
	if (ret != IDNA_SUCCESS) {
		log_printf(LOG_VERBOS, "idn_decode failed(%d):%s\n", ret, idna_strerror(ret)); 
		return NULL; 
	}
	return new; 
}
void free_iri(iri_t *iri)
{
	assert (iri != NULL); 

	if (iri->url_encode) free(iri->url_encode); 
	if (iri->content_encode) free(iri->content_encode); 
	free (iri); 
}
bool encode_to_utf8(const iri_t *iri, const char *s, char**n)
{
	assert(iri != NULL); 
	assert(s != NULL); 
	assert(n != NULL); 
	
	iconv_t cd; 
	bool ret; 
	if (!iri->url_encode) return false; 
	if (iri->enable_utf8 && strcmp(iri->url_encode, "UTF-8") == 0) {
		int i = 0, len = strlen(s); 
		for (; i < len; i++) 
			if (s[i] >= '\200') {
				*n = strdup(s); 
				return true; 
			}
		return false; 
	}
	cd = iconv_open("UTF-8", iri->url_encode); 
	if (cd == (iconv_t)-1) return false; 
	ret = convert(cd, (char*)s, strlen(s), n); 
	iconv_close(cd); 
	/* 查看是否转换 */
	if (!strcmp(s, *n)) {
		//未转换
		free(*n); 
		*n = NULL; 
		return false; 
	}
	return ret; 
}
bool convert(iconv_t cd, char *src, size_t len, char **n)
{
	assert(src != NULL); 
	assert(len > 0); 
	assert(n != NULL); 

	size_t dlen = len << 1; 
	char *dst = malloc(dlen + 1); 
	size_t invaild = 0; /* 无法转换的字节数 */
	if (dst == NULL) {
		log_printf(LOG_ERROR, "malloc error at func: %s\n", __func__); 
		return false; 
	}
	char **o = &dst; 
	size_t olen = dlen; 
	for (; ; ) {
		if (iconv(cd, &src, &len, o, &olen) != (size_t)-1) {
			*n = dst; 
			dst[strlen(dst)] = '\0'; 	/* UTF-8编码可以这样使用？ */
			if (invaild) log_printf(LOG_WARNING,
					"the number of unexpected characters is %d\n", invaild
							     ); 
			return true; 
		}
		if (errno == EINVAL || errno == EILSEQ) {
			/* 未期望的字符 */
			**o = *src; 
			src++; 
			len--; 
			(*o)++; 
			olen--; 
			invaild++; 
		} else if (errno == E2BIG) {
			size_t donelen = dlen; 
			dlen = donelen + (len << 1); 
			char *nm = malloc(dlen + 1); 
			assert (nm != NULL); 
			memcpy(nm, dst, donelen); 
			free(dst); 
			dst = nm; 
			olen = dlen - donelen; 
			*o = dst + donelen; 
		} else break; 
	}
	return false; 
}
