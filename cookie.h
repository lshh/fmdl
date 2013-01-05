/*
 * =====================================================================================
 *
 *       Filename:  cookie.h
 *
 *    Description:  处理COOKIE
 *
 *        Version:  1.0
 *        Created:  2012年12月30日 15时51分36秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef COOKIE_BYFI3LCC

#define COOKIE_BYFI3LCC

#include "hash.h"

#include <stdint.h>
#include <time.h>

typedef struct _cookie_jar {
	hash_t *cookie_hash; 
	size_t cookie_count; 
} cookie_jar_t;

typedef struct _cookie_t {
	char *domain; 		/* 域名 */
	char *path; 		/* 路径 */
	time_t expires; 	/* 生存期 */
	char *name; 		/*  */
	char *value; 		/*  */
	bool secure; 		/* 安全 */
	bool session; 		/* 会话COOKIE还是持久COOKIE */
	struct _cookie_t *next; 
} cookie_t;
/*
 * 初始化COOKIE_JAR
 */
void cookie_jar_new(); 
/*
 * 删除COOKIE_JAR
 */
void cookie_jar_delete(); 
/*
 * 查找指定的COOKIE
 */
bool delete_cookie(cookie_t *ch, cookie_t *c); 
/*
 * 从SET-COOKIE首部中解析出COOKIE并保存
 */
bool save_setcookie(const char *set_cookie); 
/*
 * 生成符合domain和path以及secflag条件的COOKIE首部
 */
char *cookie_to_head(const char *domain, const char *path, bool secflag); 
/* 将所有获得的COOKIE存入文件 */
bool save_cookies_to_file(const char *file); 
/*
 * 从文件中加载COOKIE
 */
int load_cookies_from_file(const char *file); 
#endif /* end of include guard: COOKIE_BYFI3LCC */
