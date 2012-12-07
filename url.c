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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>


static supported_t support_proto[] = {
	{"http://", SCHEME_HTTP, scm_has_query|scm_has_frag}, 
	{"ftp://", SCHEME_FTP, scm_has_param|scm_has_frag}
}; 
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
url_t url_parsed(char *url, int *err)
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
	url_t *ret = malloc(sizeof(url_t)); 
	assert(ret == NULL); 
	
	scheme = url_scheme(url); 
	newurl = strdup(url); 
	if (scheme == SCHEME_UNSUPPORT) {
		*err = ERR_UNSUPPORT_SCHEME; 
		goto ERROR; 
	}
	if (scheme == SCHEME_NOSCHEME)
		newurl = short_hand_url(url); 

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
		char *e = strchar(p, ']'); 
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
		while(p != e && *p) port = port*10 + *p++ - '0'; 
	}
	#define GET_STRING(charset, arg) do{\
		IF_GO_END(p); 
		char *e = strpbrk(p, sep++); 
		IF_GO_END(e); 
		if (*e == charset) arg##_b = e; 
		e = strpbrk(p, sep); 
		arg##_e = e == NULL?arg##_b + strlen(arg##_b):e; 
		p = e == NULL?e:e + 1; 
	} while (0)
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
	ret->url = encode_url(newurl); 
	ret->host = strndup(host_b, host_e - host_b); 
	ret->port = port; 
	ret->scheme = scheme; 
	ret->path = path_b == NULL?strdup("/"):strndup(path_b, path_e - path_b);
	get_dir_file(ret->path, &(ret->dir), &(ret->file)); 
	ret->param = param_b == NULL?param_b:strndup(param_b, param_e); 
	ret->query = query_b == NULL?query_b:strndup(query_b, query_e); 
	ret->frag = frag_b == NULL?frag_b:strndup(frag_b, frag_e); 
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
	return NULL; 
}
