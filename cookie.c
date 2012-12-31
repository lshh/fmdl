/*
 * =====================================================================================
 *
 *       Filename:  cookie.c
 *
 *    Description:  处理COOKIE的接口实现
 *
 *        Version:  1.0
 *        Created:  2012年12月31日 13时00分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "cookie.h"
#include "http.h"
#include "hash.h"

#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#define SKIP_SPACE(c) while (isspace(*c)) c++

#define GET_WORD(s, b, e) do {\
	SKIP_SPACE(s); \
	b = s; \
	while (!isspace(s++)); \
	e = s; \
} while (0)
#define COOKIE_HASH_GET(k)  hash_get(cookie_jar->cookie_hash, (const char *)k)
#define COOKIE_HASH_PUT(k, v)  (cookie_t *)hash_put(cookie_jar->cookie_hash, (const void *)k, (void*)v)
static cookie_jar_t *cookie_jar; 
/*
 * 从SET-COOKIE首部中解析出COOKIE
 */
static cookie_t *cookie_from_setcookie(const char *set_cookie); 
static void free_cookie(cookie_t *c); 
static time_t GMT2time(const char *s); 
static void delete_cookie_chain(cookie_t *c); 
static char *find_cookies_by_abs_domain(char *d, size_t l, char *p, bool secflag); 

/*
 * 查找指定的cookie如果找到返回此cookie同是prec设置为
 * 在链表中的前一个cookie 如果*prec等于NULL则此cookie
 * 在链表的头部
 */
static cookie_t *find_cookie(cookie_t *c, cookie_t **prec); 

void  cookie_jar_new()
{
	/* 如果已存在则无需再次创建 */
	if (cookie_jar) return; 

	cookie_jar = calloc(1, sizeof(cookie_jar_t)); 
	assert (cookie_jar != NULL); 

	cookie_jar->cookie_hash = hash_new(0, hash_string_nocase, string_nocase_cmp); 
	assert (cookie_jar->cookie_hash != NULL); 
}

void cookie_jar_delete()
{
	if (!cookie_jar) return; 
	hash_iterator_t *ite = hash_iterator(cookie_jar->cookie_hash); 
	while (hash_iterator_next(ite)) {
		cookie_t *c = (cookie_t *)(ite->v); 
		if (c) delete_cookie_chain(c); 
	}

	hash_destroy(cookie_jar->cookie_hash); 
	free(cookie_jar); 
	cookie_jar = NULL; 
}

cookie_t *cookie_from_setcookie(const char *set_cookie)
{
#define GET_VAL(n, v) do {\
	char *p; \
	p = wd_b; \
	while (p < wd_e) {\
		if (*p++ == '=') break; \
	}\
	if (p == wd_e) goto BAD_COOKIE; \
	if (n != NULL) *(n) = strndup(wd_b, p - wd_b); \
	p++; \
	*(v) = strndup(p, wd_e - p); \
} while (0)

#define SKIP_SETCOOKIE(sc) do {\
	char *hd = "set-cookie:"; \
	if (strncasecmp(sc, hd, strlen(hd)) == 0) {\
		sc += strlen(hd); \
	} \
} while (0)
	assert (set_cookie != NULL); 
	char *sc = (char *)set_cookie; 
	char *wd_b = NULL, *wd_e = NULL; 
	char *name = NULL, *value = NULL; 
	char *expires = NULL, *domain = NULL; 
	char *path = NULL, *secure = NULL; 
	SKIP_SETCOOKIE(sc); 
	GET_WORD(sc, wd_b, wd_e); 
	/*
	 * 解析NAME=VALUE
	 */
	GET_VAL(&name, &value); 
	while ((sc = strchr(sc, ';'))) {
		char **cmd = NULL; 
		sc++; 
		GET_WORD(sc, wd_b, wd_e); 
		if (strncasecmp("expires", wd_b, wd_e - wd_b) == 0) {
			GET_VAL(cmd, &expires); 
		} else if (strncasecmp("path", wd_b, wd_e - wd_b) == 0) {
			GET_VAL(cmd, &path); 
		} else if (strncasecmp("domain", wd_b, wd_e - wd_b) == 0) {
			GET_VAL(cmd, &domain); 
		} else if (strncasecmp("secure", wd_b, wd_e - wd_b) == 0) {
			GET_VAL(cmd, &secure); 
		} else {
			/*
			 * 其他字段忽略
			 */
		}
	}
	/*
	 *  创建一个新的COOKIE_T结构
	 */
	cookie_t *n = calloc(1, sizeof(cookie_t)); 
	if (!n) goto BAD_COOKIE; 
	n->domain = domain; 
	n->path = path; 
	n->name = name; 
	n->value = value; 
	n->secure = strcasecmp(secure, "true") == 0 ? true : false; 
	n->session = expires == NULL ? true : false; 
	n->expires = expires == NULL ? GMT2time(expires) : 0; 
	return n; 
BAD_COOKIE:
	if (name) free(name); 
	if (value) free(value); 
	if (expires) free(expires); 
	if (domain) free(domain); 
	if (path) free(path); 
	if (secure) free(secure); 
	return NULL; 
#undef SKIP_SETCOOKIE
#undef GET_VAL
}

time_t GMT2time(const char *s)
{
	time_t ret = 0; 
	char *time_fmt[] = {
		"%a, %d-%b-%y %T", 
		"%a, %d-%b-%Y %T", 
		"%A, %d-%b-%y %T", 
		"%A, %d-%b-%Y %T"
	}; 
	int l = sizeof(time_fmt)/sizeof(time_fmt[0]); 

	/*保存时间的本地格式*/
	char *oldloc = setlocale(LC_TIME, NULL); 
	char svloc[256] = {0}; 
	if (oldloc) {
		size_t len = strlen(oldloc);
		if (len <= 255) {
			memcpy(svloc, oldloc, len); 
		} 
	} 

	setlocale(LC_TIME, "C"); 

	int i = 0; 
	for (; i < l; i++) {
		struct tm tm; 
		memset(&tm, 0, sizeof(tm)); 

		if (strptime(s, time_fmt[i], &tm)) {
			ret = timegm(&tm);
			break; 
		}
	}
	setlocale(LC_TIME, svloc); 
	return ret; 
}

void delete_cookie_chain(cookie_t *chain)
{
	assert (chain != NULL); 
	cookie_t *c = chain->next; 
	cookie_t *p = chain; 
	if (c) {
		while (c) {
			free_cookie(p); 
			cookie_jar->cookie_count--; 
			p = c; 
			c = p->next; 
		}
		free_cookie(p); 
			cookie_jar->cookie_count--; 
	} else 
		free_cookie(chain); 
	cookie_jar->cookie_count--; 
}

void free_cookie(cookie_t *c)
{
	assert(c != NULL); 
	if (c->domain) free(c->domain); 
	if (c->path) free(c->path); 
	if (c->name) free(c->name); 
	if (c->value) free(c->value); 
	free(c); 
}

cookie_t *find_cookie(cookie_t *c, cookie_t **prec) 
{
	if (cookie_jar == NULL) return NULL; 

	cookie_t *ret = COOKIE_HASH_GET(c->domain); 
	cookie_t *pre = NULL; 
	if (!ret) goto NO_MATCH; 
	while (ret) {
		if (strcmp(c->path, ret->path) == 0
				 && strcmp(c->name, ret->name) == 0) {
			if (prec) *prec = pre; 
			return ret; 
		}
		pre = ret; 
		ret = ret->next; 
	}
NO_MATCH:
	if (prec) *prec = pre; 
	return NULL; 
}

bool save_setcookie(const char *set_cookie)
{
	assert(set_cookie != NULL); 
	cookie_t *c = cookie_from_setcookie(set_cookie); 
	if (c == NULL) return false; 

	cookie_t *pre = NULL; 
	cookie_t *fn = (cookie_t *)COOKIE_HASH_GET(c->domain); 
	if (!fn) {
		/* 不存在此domain的COOKIE */
		cookie_jar->cookie_count++; 
		goto RET; 
	}
	/* 存在此domain的COOKIE */
	cookie_t *fn1 = find_cookie(c, &pre); 
	if (!fn1) {
		/* 不存在和c->name相同的COOKIE, 在链表头插入新的COOKIE */
		cookie_jar->cookie_count++; 
		c->next = fn1; 
		goto RET; 
	} 
	if (!pre) {
		/* 要替换的COOKIE在链表头部 */
		c->next = fn1->next; 
	} else {
		pre->next = fn1->next; 
	}
	c->next = fn; 
	free_cookie(fn1); 
RET:
	COOKIE_HASH_PUT(c->domain, c); 
	return true; 
}

char *cookie_to_head(const char *domain, const char *path, bool secflag)
{
#define COOKIE_STIRNG_CPY do {\
	if (cookie_tmp) {\
		if (strlen(cookie_tmp) > c_len) {\
			c_len += MAX_STRING; \
			cookie = realloc(cookie, c_len + 1); \
		}\
		len += sprintf(cookie + len, " %s", cookie_tmp); \
		free(cookie_tmp);\
	}\
}while (0)
	assert (domain != NULL); 
	char *w_b, *w_e = (char *)(domain + strlen(domain)); 
	char *cookie = NULL, *cookie_tmp = NULL; 
	int dot = 0; 
	bool flag = false; 
	size_t len; 
	size_t c_len = MAX_STRING; 
	char *allowed[] = {
		".com", ".edu", ".net", ".org", ".gov", ".mil", 
		".int", ".biz", ".info", ".name", ".museum", ".coop", 
		".aero", ".pro"
	};
	w_b = w_e; 
	w_b = strchr(w_b, '.'); 
	if (!w_b || w_b == domain) {
		/*
		 * 错误的域名或者如.com .edu等需要两个或三个'.'的域名
		 */
		return NULL; 
	}

	cookie = malloc(c_len + 1); 
	len = sprintf(cookie, "Cookie: "); 
	c_len -= len; 
	assert(cookie != NULL); 
	w_b -= 2; 
	dot++; 
	while ((w_b = strrchr(w_b, '.'))) {
		/*
		 * www.foo.bar.com
		 * 			   ^
		 */
		dot++; 
		if (dot == 2  && (w_b == domain || !strrchr(w_b - 1, '.'))) {
			/*
			 * 查看只有两个'.'的域名是否符合要求
			 */
			size_t l = sizeof(allowed)/sizeof(allowed[0]); 
			int i; 
			w_b = strrchr(domain, '.'); 
			for (i = 0; i < l; i++) {
				if (strcmp(w_b, allowed[i]) == 0) break; 
			}
			if (i == l) {
				/* DOMAIN为不被允许只有两个'.'的域名 */
				free(cookie); 
				return NULL; 
			}
			cookie_tmp = find_cookies_by_abs_domain((char *)domain,
				   	strlen(domain), (char *)path, secflag); 
			flag = true; 
			break; 
		}
		cookie_tmp = find_cookies_by_abs_domain(w_b, w_e - w_b, (char *)path, secflag); 
		COOKIE_STIRNG_CPY; 
		if (w_b == domain) break; 
		w_b--; 
	}
	if (w_b == NULL && !flag) {
		cookie_tmp = find_cookies_by_abs_domain((char *)domain, 
				strlen(domain), (char *)path, secflag); 
	}
	COOKIE_STIRNG_CPY; 
	return cookie; 
}

char *find_cookies_by_abs_domain(char *d, size_t l, char *p, bool secflag)
{
	assert(d != NULL && p != NULL && l > 0); 
	cookie_t *c = (cookie_t *)COOKIE_HASH_GET(d); 
	if (!c) return NULL; 
	/*
	 * 找到符合此域的COOKIE
	 */
}
