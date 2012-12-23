/*
 * =====================================================================================
 *
 *       Filename:  url.c
 *
 *    Description:  处理URL模块实现
 *
 *        Version:  1.0
 *        Created:  2012年12月05日 19时46分31秒
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
#include "net.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define NUM_TODIGIT(n) "0123456789ABCDEF"[n]
enum char_type {unsafe_char = 0x01, reserved_char = 0x02}; 
#define IS_UNSAFE(c) (ascii_table[(c)] & unsafe_char)
#define IS_RESERVED(c) (ascii_table[(c)] & reserved_char)
#define U unsafe_char
#define R reserved_char
#define RU U|R
static const unsigned char ascii_table[256] =
{
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
#undef R
#undef U
#undef RU
static supported_t support_proto[] = {
	{"http://", SCHEME_HTTP, 80, scm_has_query|scm_has_frag}, 
	{"ftp://", SCHEME_FTP, 21, scm_has_param|scm_has_frag}
}; 
static uint16_t default_scheme_port(scheme_t scheme); 
static int char_2_digit(char c); 
static char *skip_scheme(const char *url); 
static char *skip_user_info(const char *url, char **b, char**e); 
static char *init_sep(scheme_t scheme); 
static bool need_encode(char *c); 
static bool need_decode(char *c); 
static void get_dir_file(const char *path, char **dir, char **file); 
static void get_name_passwd(const char *s, size_t l, char **n, char **p); 

scheme_t url_scheme(char *url)
{
	assert(url != NULL); 
	char *sep = "://"; 
	char *p; 
	int i = 0; 
	p = strstr(url, sep); 
	if (!p) return SCHEME_NOSCHEME; 
	for (; i < countof(support_proto); i++) {
		char *prefix = support_proto[i].prefix; 
		if (strncasecmp(url, prefix, strlen(prefix)) == 0)
			return support_proto[i].scheme; 
	}
	return SCHEME_UNSUPPORT; 
}
/*
 * www.foo.com.cn[:port]		---->http://www.sina.com.cn[:port]
 * foo.com.cn[:port]/dir/file	---->http://foo.com.cn[:port]/dir/file
 * foo.com.cn:dir/file			---->ftp://foo.com.cn/dir/file
 * foo.com.cn:/dir/file			---->ftp://foo.com.cn/dir/file
 */
char *short_hand_url(char *url)
{
	assert(url != NULL); 
	char sep = ':'; 
	char *p; 
	size_t len; 
	char *new; 
	scheme_t scheme = url_scheme(url); 
	if (scheme != SCHEME_NOSCHEME) return NULL; 
	p = strchr(url, sep); 
	if (p == NULL || isdigit(*(p + 1))) {
		//添加http://
		len = strlen(url) + strlen("http://"); 
		new = malloc(1 + len); 		
		assert(new != NULL); 		//应该时不会失败的
		sprintf(new, "http://%s", url); 
		new[len] = '\0'; 
		return new; 
	}
	//添加ftp://
	len = strlen(url) + strlen("ftp://"); 
	new = malloc(len + 1);
	sprintf(new, "ftp://%s", url); 
	//处理':'字符
	/*
	 * ftp://foo.bar.com:....
	 * 	  ^
	 */
	p = strchr(new, sep); 
	/*
	 * ftp://foo.bar.com:....
	 * 					^
	 */
	p = strchr(++p, sep); 
	if (*(p + 1) == '/') {
		char *b = p + 1; 
		while (*b) *p++ = *b++; 
	*p = '\0'; 
	} else *p = '/'; 
	return new; 
}
url_t *url_parsed(char *url, int *err)
{
	assert(url != NULL); 
	assert(err != NULL); 
	
	scheme_t scheme; 
	uint16_t port; 
	char *host_b = NULL, *host_e = NULL; 
	char *path_b = NULL, *path_e = NULL; 
	char *name_b = NULL, *name_e = NULL; 
	char *param_b = NULL, *param_e = NULL; 
	char *query_b = NULL, *query_e = NULL; 
	char *frag_b = NULL, *frag_e = NULL; 
	char *newurl; 
	char *p; 
	char *sep; 
	url_t *ret = malloc(sizeof(url_t)); 
	assert(ret != NULL); 
	*err = NO_ERROR; 
	scheme = url_scheme(url); 
	newurl = strdup(url); 
	if (scheme == SCHEME_UNSUPPORT) {
		*err = ERR_UNSUPPORT_SCHEME; 
		goto ERROR; 
	}
	if (scheme == SCHEME_NOSCHEME)
		newurl = short_hand_url(url); 
	scheme = url_scheme(newurl); 
#define IF_BADURL(p) if(!(p)) {*err = ERR_BADURL; goto ERROR; }

	port = default_scheme_port(scheme); 
	//跳过协议字段
	p = skip_scheme(newurl);
	/*
	 * http://user:passwd@host:[port]....
	 * 	      ^
	 */
	IF_BADURL(p); 
	p = skip_user_info(p, &(name_b), &(name_e)); 
	/*
	 * http://user:passwd@host:[port]....
	 * 					  ^
	 */
	IF_BADURL(p); 
	sep = init_sep(scheme);
	if (*p == '[') {
		/* IPv6地址 */
		char *e = strchr(p, ']'); 
		IF_BADURL(e); 
		host_b = p + 1; 
		host_e = e; 
		IF_BADURL(valid_ipv6_addr(host_b, host_e - host_b));
		p = e + 1; 
	} else {
		host_b = p; 
		p = strpbrk(p, sep); 
		host_e = p; 
	}
	/*
	 * www.foo.bar[:port]/....
	 * 			   ^ or  ^ or ...
	 */
#define IF_GO_END(p) if(!(p)) goto END
	IF_GO_END(p); 
	p = strpbrk(p, sep++); 
	IF_GO_END(p); 
	if (*p == ':') {
		port = 0; 
		char *e = strpbrk(p, sep); 
		while(p != e && p + 1 != e && *p){
			port = port*10 + *++p - '0'; 
		}	
	}
#define GET_STRING(charset, arg) do{\
	IF_GO_END(p); \
	char *e = strpbrk(p, sep++); \
	IF_GO_END(e); \
	if (*e == charset) arg##_b = e + 1; \
	e = strpbrk(p, sep); \
	arg##_e = e == NULL?arg##_b + strlen(arg##_b):e; \
	p = e; \
} while (0)

	{
		IF_GO_END(p); 
		char *e = strpbrk(p, sep++); 
		IF_GO_END(e); 
		if (*e == '/') path_b= e; 
		e = strpbrk(p, sep); 
		path_e = !e?path_b + strlen(path_b):e; 
		p = e; 
	}
	if (support_proto[scheme].flag & scm_has_param)
		GET_STRING(';', param);
	if (support_proto[scheme].flag & scm_has_query)
		GET_STRING('?', query); 
	if (support_proto[scheme].flag & scm_has_frag)
		GET_STRING('#', frag); 
#undef GET_STRING
#undef IF_GO_END
#undef IF_BADURL
	/* --------end-------- */
END:
	ret->encode_url = encode_url(newurl); 
	ret->host = strndup(host_b, host_e - host_b); 
	ret->port = port; 
	ret->scheme = scheme; 
	ret->path = path_b == NULL?strdup("/"):strndup(path_b, path_e - path_b);
	get_dir_file(ret->path, &(ret->dir), &(ret->file)); 
	ret->param = param_b == NULL?param_b:strndup(param_b, param_e - param_b); 
	ret->query = query_b == NULL?query_b:strndup(query_b, query_e - query_b); 
	ret->frag = frag_b == NULL?frag_b:strndup(frag_b, frag_e - frag_b); 
	if (name_b == NULL) {
		free(newurl); 
		return ret; 
	}
	get_name_passwd(name_b, name_e - name_b, &(ret->name), &(ret->passwd)); 
	free(newurl); 
	return ret; 
ERROR:
	free(newurl); 
	free(ret); 
	return (url_t*)NULL; 
}
uint16_t default_scheme_port(scheme_t scheme)
{
	return support_proto[scheme].port; 
}
char *skip_scheme(const char *url)
{
	assert(url != NULL); 
	
	char *sep = "://"; 
	char *p = strstr(url, sep); 
	if (!p) return NULL;
	return p + 3; 
}
char *skip_user_info(const char *url, char **b, char **e)
{
	assert(url != NULL); 
	assert(b != NULL && e != NULL); 

	char sep = '@'; 
	char *p = strchr(url, sep); 
	*b = url; 
	/* 没有用户信息字段 */
	if (!p) {*b = NULL; *e = NULL; return url; }
	*e = p; 
	return ++p; 
}
char *init_sep(scheme_t scheme)
{
	static char sep[8] = ":/"; 
	char *p = sep + 2; 
	if (support_proto[scheme].flag & scm_has_param)
		*p++ = ';'; 
	if (support_proto[scheme].flag & scm_has_query)
		*p++ = '?'; 
	if (support_proto[scheme].flag & scm_has_frag)
		*p++ = '#'; 
	*p = '\0'; 
	return sep; 
}
char *encode_url(char *orig_url)
{
	assert(orig_url != NULL); 
	size_t encode_count = 0; 
	char *p = orig_url; 
	char *new, *n; 
	size_t len; 
	while (*p != '\0') if (need_encode(p++)) encode_count++; 
	if (encode_count == 0)  return strdup(orig_url); 
	len = strlen(orig_url) + encode_count*2; 
	new = malloc(len + 1); 
	assert(new != NULL); 
	p = orig_url; 
	n = new; 

	while (*p != '\0') {
		if (need_encode(p)) {
			*n++ = '%'; 
			*n++ = NUM_TODIGIT(*p >> 4);
			*n++ = NUM_TODIGIT(*p & 0x0f); 
		}else 
			*n++ = *p; 
		p++; 
	}
	new[len] = '\0'; 
	return new; 
}
bool need_encode(char *c)
{
	if (*c == '%') {
		if (is_digit(*(c + 1)) && is_digit(*(c + 1))) 
			return false; 
		else return true; 
	}
	if (IS_UNSAFE(*c) && !IS_RESERVED(*c))
		return true; 
	return false; 
}
void  get_dir_file(const char *path, char **dir, char **file)
{
	assert(path != NULL); 
	assert(dir != NULL); 
	assert(file != NULL); 

	const char *p = path; 
	char sep = '/'; 
	char *dir_b = NULL, *dir_e = NULL; 
	char *file_b = NULL, *file_e = NULL; 
	file_b = strrchr(p, sep); 
	if (file_b == NULL) return; 
	if (*++file_b == '\0') {
		*file = NULL; 
		*dir = strdup(path); 
		return; 
	}
	file_e = file_b + strlen(file_b); 
	dir_e = file_b; 
	dir_b = path; 
	*dir = strndup(dir_b, dir_e - dir_b); 
	*file = strndup(file_b, file_e - file_b); 
}
void get_name_passwd(const char *s, size_t l, char **n, char **p) 
{
	assert (n != NULL && p != NULL); 
	char sep = ':'; 
	char *ptr; 
	size_t len = l; 
	char *name_b, *name_e; 
	char *pass_b, *pass_e; 
	if (s == NULL || l == 0) {
		*n = NULL; 
		*p = NULL; 
		return; 
	}
	ptr = s; 
	while (*ptr != sep && l--) ptr++; 
	if (*ptr != sep) {
		/* 无密码字段 */
		*p = NULL; 
		*n = strndup(s, len); 
		return ; 
	}
	name_e = ptr; 
	name_b = s; 
	*n = strndup(name_b, name_e - name_b); 
	pass_b = ptr + 1; 
	/* name:@...... */
	if (*pass_b == '@') {
		*p = NULL; 
		return; 
	}
	pass_e = pass_b + l - 1; 
	*p = strndup(pass_b, pass_e - pass_b); 
}
char *decode_url(const char *encode_url)
{
	assert (encode_url != NULL); 

	const char *p = encode_url; 
	size_t encode_cnt = 0; 
	char *new; 
	size_t newlen; 
	while (*p != '\0') if (need_decode(p++)) encode_cnt++; 

	if (encode_cnt == 0) return strdup(encode_url); 
	newlen = strlen(encode_url) - (encode_cnt << 1); 
	new = malloc(newlen + 1); 
	assert(new != NULL); 
	p = encode_url; 
	while (*p != '\0') {
		if (need_decode(p++)) {
			char c = char_2_digit(*p) << 4; 
			c += char_2_digit(*++p); 
			*new++ = c; 
			++p; 
		} else *new++ = *p++; 
	}
	new[newlen] = '\0'; 
	return new; 
}
int char_2_digit(char c)
{
	switch (c) {
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9':
			return c - '0'; 
		case 'a': case 'b': case 'c': case 'd': case 'e': 
		case 'f':
			return c - 'a' + 10; 
		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F':
			return c - 'A' + 10; 
		default:
			abort(); 
	}
}
bool need_decode(char *c)
{
	assert(c != NULL); 
	if (*c == '%' && is_digit(*(c + 1)) && is_digit(*(c + 2))) 
		return true; 
	return false; 
}
void free_url(url_t *url)
{
	assert (url != NULL); 

	if (url->encode_url) free(url->encode_url); 
	if (url->host) free(url->host); 
	if (url->path) free(url->path); 
	if (url->dir) free(url->dir);
	if (url->file) free(url->file); 
	if (url->param) free(url->param);
	if (url->query) free(url->query); 
	if (url->frag) free(url->frag); 
	if (url->name) free(url->name); 
	if (url->passwd) free(url->passwd); 
	free(url); 
}
