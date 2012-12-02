/*
 * =====================================================================================
 *
 *       Filename:  url.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年12月01日 20时36分18秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "url.h"
#include "error_code.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define HTTP_PORT 	80
#define FTP_PORT	21
#define HTTPS_PORT	443
#define INVALID_PORT  -1
#define count_of_array(array) (sizeof(array)/sizeof((array)[0]))
#define U 0x01
#define R 0x02
#define RU	U|R
#define test_char(c, mask) (ascii_table[(c)]&(mask))
#define unsafe_char(c) test_char((c), U)
#define reserved_char(c) test_char((c), R)
/* ASCII字符表，用来判断字符是否时安全字符或转义转字符 */
static ascii_table[256] = {
  U,  U,  U,  U,   U,  U,  U,  U,   /* NUL SOH STX ETX  EOT ENQ ACK BEL */
  U,  U,  U,  U,   U,  U,  U,  U,   /* BS  HT  LF  VT   FF  CR  SO  SI  */
  U,  U,  U,  U,   U,  U,  U,  U,   /* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
  U,  U,  U,  U,   U,  U,  U,  U,   /* CAN EM  SUB ESC  FS  GS  RS  US  */
  U,  0,  U, RU,   R,  U,  R,  0,   /* SP  !   "   #    $   %   &   '   */
  0,  0,  0,  R,   R,  0,  0,  R,   /* (   )   *   +    ,   -   .   /   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* 0   1   2   3    4   5   6   7   */
  0,  0, RU,  R,   U,  R,  U,  R,   /* 8   9   :   ;    <   =   >   ?   */
 RU,  0,  0,  0,   0,  0,  0,  0,   /* @   A   B   C    D   E   F   G   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* H   I   J   K    L   M   N   O   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* P   Q   R   S    T   U   V   W   */
  0,  0,  0, RU,   U, RU,  U,  0,   /* X   Y   Z   [    \   ]   ^   _   */
  U,  0,  0,  0,   0,  0,  0,  0,   /* `   a   b   c    d   e   f   g   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* h   i   j   k    l   m   n   o   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* p   q   r   s    t   u   v   w   */
  0,  0,  0,  U,   U,  U,  0,  U,   /* x   y   z   {    |   }   ~   DEL */

  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,

  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
}; 
/* 支持的协议 */
typedef enum {
	scheme_have_param = 0x01, 
	scheme_have_query = 0x02, 
	scheme_have_frag = 0x04, 
	scheme_have_invalid = 0x08
}scheme_flag_t; 

struct support_proto
{
	char *index; 
	scheme_t scheme; 
	uint16_t port; 
	scheme_flag_t flag; 
}; 
static struct support_proto default_proto[] = {
	{"http://", SCHEME_HTTP, HTTP_PORT, scheme_have_query|scheme_have_frag}, 
	{"https://", SCHEME_HTTPS, HTTPS_PORT, scheme_have_query|scheme_have_frag}, 
	{"ftp://", SCHEME_FTP, FTP_PORT, scheme_have_param|scheme_have_frag}, 
	{NULL, SCHEME_INVALID, INVALID_PORT, scheme_have_invalid}
}; 
static scheme_t get_url_scheme(const char *); 
static bool have_scheme_head(const char *); 
static char *escape_url(const char *); 
static char *set_scheme_head(char *); 
static bool char_need_escape(const char *); 
static char *skip_scheme(const char *); 
static char *initsep(scheme_t scheme); 
static char *get_user(char *, char**, char**, int *); 
static char *get_host(char*, char**, char**, int*, char*); 
static char *get_path(char*, char**, char**, int*, char*);

url_t *url_parsed(char *url, iri_t *iri, bool encode_unsafe, int *err_code)
{
	assert(url != NULL); 
	url_t *u; 
	char *newurl = strdup(url); 
	char *host_b, host_e; 
	char *name_b, *name_e; 
	char *path_b, *path_e; 
	char *param_b, *param_e; 
	char *query_b, *query_e; 
	char *frag_b, frag_e; 
	char *p; 
	char *path; 
	scheme_t scheme = get_url_scheme(newurl); 
	*err_code = NO_ERROR; 
	u = malloc(sizeof(url_t)); 
	if (u == NULL) {
		*err_code = ERR_ALLOC; 
		goto EXIT; 
	}
	if (scheme == SCHEME_INVALID) {
		if (have_scheme_head(newurl)) {
			*err_code = ERR_UNSUPPORT_SCHEME; 
			goto EXIT; 
		}
		char *ourl = newurl; 
		newurl = set_scheme_head(newurl); 
		free(ourl); 
		if (newurl == NULL) {
			*err_code = ERR_SCHEME_SET_FAIL; 
			goto EXIT; 
		}
		/* 此时肯定可以获取正确的scheme */
		scheme = get_url_scheme(newurl); 
	}
	/* 对URL进行编码转换为UTF-8格式 */
	if (iri && iri->enable_utf8) {
		char *new_url; 
		iri->enable_utf8 = encode_to_utf8(iri, newurl, &new_url); 
		if (iri->enable_utf8) {free(newurl); newurl = new_url; }
	}
	/* 对链接中的不安全字符进行编码 */
	if (encode_unsafe) {
		char *ourl = newurl; 
		newurl = escape_url(newurl); 
		if (newurl == NULL) goto EXIT; 
		free(ourl); 
	}
	/*
	 * 跳过协议头
	 * scheme://www.foobar.com/foo/bar
	 * 			^
	 */
	p = skip_scheme(newurl); 
	if (p = NULL) {
		*err_code = ERR_BADURL; 
		goto EXIT; 
	}
	name_b = name_e = NULL; 
	/*
	 * 跳过用户名和密码字段
	 * user:passwd@host[:port]...
	 * ^
	 */
	p = get_user(p, &name_b, &name_e, err_code); 
	/*
	 * user:passwd@host[:port]....
	 * 			   ^
	 * host_b和host_e所确定的范围包含端口号当然是如果有的话
	 */
	char *sep = initsep(scheme); 
	//可能为一个IPv6地址
	if (*p  == '[') {
		host_b = *(++p); 
		host_e = strchr(host_b, ']'); 
		if (host_e == NULL) {
			*err_code = ERR_BADURL; 
			goto EXIT; 
		}
		if (!valid_ipv6_addr(host_b, host_e - host_b)) {
			*err_code = ERR_BADURL; 
			goto EXIT; 
		}
		p = host_e + 1; 
	} else {
		p = get_host(p, &host_b, &host_e, err_code, sep++); 
	}

	path_b = path_e = NULL; 
	param_b = param_e = NULL; 
	query_b = query_e = NULL; 
	frag_b = frag_e = NULL; 

	if (*p != '\0') {
		/*
		 * www.foo.com/path/file..
		 * 			  ^ 
		 */
		p = get_path(p, &path_b, &path_e, err_code, sep++); 
		path = reset_path(path_b, path_e - path_b); 
		/*
		 * www.foo.com/path/file...
		 * 						^
		 */
		if(++p && default_proto[scheme].flag&scheme_have_param)
			p = get_param(p, &param_b, &param_e, err_code, sep++); 
		if (++p && default_proto[scheme].flag&scheme_have_query)
			p = get_query(p, &query_b, &query_e, err_code, sep++); 
		if (++p && default_proto[scheme].flag&scheme_have_frag) {
			frag_b = p; 
			frag_e = strchr(p, '\0'); 
		}
	}
	/* 解析完毕 */
	assert(*p == '\0'); 

	{
		char *user, *pass; 
		uint16_t port; 
		char *host; 
		/* 解析用户名和密码 */
		if (name_b == NULL) user = pass = NULL; 
		get_user_info(name_b, name_e - name_b, &user, &pass); 
		u->user = user; 
		u->passwd = pass; 
		get_host_info(host_b, host_e - host_b, &host, &port); 
		if (strchr(host, '%')) {
			host = unsecape(host); 
			if(iri && iri->enable_utf8) {
				u->host = encode(iri, host); 
				free(host); 
			}
		}
		u->port = port; 
		u->scheme = scheme; 
		u->path = path_b?strndup(path_b, path_e - path_b):strdup("/"); 
		get_dir_file(u->path, &(u->dir), &(u->file)); 
		u->parameters = param_b?strndup(param_b, param_e - param_b):NULL; 
		u->query = query_b?strndup(query_b, query_e - query_b):NULL; 
		u->fragment = frag_b?strndup(frag_b, frag_e - frag_b):NULL; 
		//重构RUL
		u->rebuild_url = rebuild_url(u, false); 
		free(newurl); 
		*err_code = NO_ERROR; 
		return u; 
	}
	abort(); 
EXIT:
	if (newurl) free(newurl); 
	if (u) free(u); 
	return NULL; 
#undef IF_ERROR
}
scheme_t get_url_scheme(const char *url)
{
	assert(url != NULL); 

	int len = count_of_array(default_proto); 
	int i = 0; 
	for (; i < len - 1; i++)
		if (!strncasecmp(url, (default_proto + i)->index, strlen((default_proto + i)->index)))
			return (default_proto + i)->shceme; 
	return SCHEME_INVALID; 
}
/*
 * 为简写形式的URL添加协议头
 * 例如：
 * www.sina.com[:port]			---->http://www.sina.com[:port]
 * foo@bar.com/dir/ 			----->http://foo@bar.com/dir/
 * foo.bar.com:dir/file			---->ftp://foo.bar.com/dir/file
 * foo.bar.com:/die/file		----->ftp://foo.bar.com/dir/file
 */
bool have_scheme_head(const char *url)
{
	assert(url != NULL); 

	char *type = "://"; 
	if (strcasestr(url, type)) return true; 
	return false; 
}
char *set_scheme_head(char *url)
{
#define ADD_PROTO_HEAD(head, url) do{\
	n = malloc(strlen((url))\
			+ strlen((head))\
			+ 1); \
	if (n == NULL) {\
		log_printf(LOG_ERROR, "malloc failed at func(%s)\n", __func__); \
		return NULL; \
	}\
	sprintf(n, head"%s", url);\
} while (0)
	assert(url != NULL);

	char *type = ":"; 
	char *p; 
	char *n; 
	if (have_scheme_head(url)) return url; 
	p = strpbrk(url, type); 
	if (p && *p == ':') {
		if (p[1] == '/' && p[2] == '/') return NULL; 
		int digit = strspn(p + 1, "01233456789"); 
		if (digit  && (*(p + digit) == '/' || *(p + digit) == '\0')) {
			ADD_PROTO_HEAD("http://", url); 
		}
		ADD_PROTO_HEAD("ftp://", url); 
		p = strchr(n, ':'); 
		p++; 
		p = strchr(p, ':'); 
		if (p[1] != '/') *p = '/'; 
		else {
			char *p1 = p + 1; 
			while (*p && (*p = *p1)) p++, p1++; 
		}
	} else 
		ADD_PROTO_HEAD("http://", url); 
	return n; 
#undef ADD_PROTO_HEAD
}
/*
 * 对URL中的不安全字符进行编码
 *
 */
char *escape_url(const char*url)
{
#define NUM2DIGIT(c) "0123456789abcdef"[(c)]
	assert(url != NULL); 

	size_t need_esc = 0; 
	char *p = url; 
	char *new; 
	do {
		if (char_need_escape(p)) need_esc++; 
	} while(*(++p)); 
	if (need_esc == 0) return strdup(url); 
	new = malloc(strlen(url) + 1 + need_esc); 
	if (new == NULL) {
		log_printf(LOG_ERROR, "malloc failed at func(%s)\n", __func__); 
		return NULL; 
	}
	p = new; 
	do {
		if (char_need_escape(url)) {
			*p++ = '%'; 
			*p++ = NUM2DIGIT(*url >> 4); 	//高四位
			*p++ = NUM2DIGIT(*url&0x0f); 	//低四位
		} else *p++ = *url; 
	} while(*(++url)); 
	*p = '\0'; 
	return new; 
#undef NUM2DIGIT
}
bool char_need_escape(const char *c)
{
	assert(c != NULL); 
	if (*c == '%') {
		if (isdigit(*(c + 1)) && isdigit(*(c + 2)))
			return false; 
		return true;
	} else if (unsafe_char(*c) && !reserved_char(*c))
		return true; 
	return false; 
}
char *skip_scheme(const char *url)
{
	assert(url != NULL); 

	char *p; 
	if (!have_scheme_head(url)) return NULL; 
	p = strstr(url, "://"); 
	if (p == NULL) return NULL; 
	p += 3; 
	return p; 
}
char *get_user(char *url, char **b, char **e, int *err)
{
	assert(url != NULL); 
	assert(b != NULL && e != NULL && err != NULL); 

	*b = url; 
	*e = strchr(*b, '@'); 
	if (*e == NULL) {
		*e = *b; 
		*err = ERR_NOUSER; 
		return url; 
	}
	*err = NO_ERROR; 
	return (*e + 1); 
}
char *initsep(scheme_t scheme)
{
	static char sep[8] = "/"; 
	char *p = sep + 1; 
	scheme_flag_t flag = default_proto[scheme].flag; 
	if (flag&scheme_have_param)
		*p++ = ';'; 
	if (flag&scheme_have_query)
		*p++ = '?'; 
	if (flag&sheme_have_frag)
		*p++ = '#'; 
	*p = '\0'; 
	return sep; 
}
char *get_host(char*url, char**b, char**e, int*err, char*sep)
{
	assert(url != NULL); 
	assert(b != NULL && e != NULL); 
	assert(err != NULL && sep != NULL); 
	*b = url; 
	*e = strpbrk(url, sep); 
	*err = NO_ERROR; 
	return *e ; 
}
char *get_path(char*url, char**b, char**e, int*err, char*sep)
{
	return get_host(url, b, e, err, sep); 
}
char *get_param(char*url, char**b, char**e, int*err, char*sep)
{
	return get_host(url, b, e, err, sep); 
}
char *get_query(char*url, char**b, char**e, int*err, char*sep)
{
	return get_host(url , b, e, err, sep); 
}
#undef U
#undef R
