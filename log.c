/*
 * =====================================================================================
 *
 *       Filename:  log.c
 *
 *    Description:  日志实现
 *
 *        Version:  1.0
 *        Created:  2012年11月24日 20时53分36秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "log.h"
#include "error_code.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>

#define DEFALUT_MSG_LEN 128
#define DEFAULT_STORE	24
#define MAX_CACHE_LENGTH	4096
#define MAX_WARNNING_MSG	3
#define LOG_FILE		"fmdl-log"
/* 日志输出接口 */
static int log_fd; 
/* 记录警告信息个数，当警告信息达到日志默认的
 * 的最大限制MAX_WARNNING_MSG时，如果此时缓冲
 * 区还未满，仍然冲洗缓冲区, 使警告信息能够
 * 立即可见 */
static int8_t warning; 
/* 禁止日志输出 */
static bool inhibit; 
/* 启用缓存 */
static bool enable_cache; 
/* 缓存 */
struct msg_cache
{
	char *cache; 
	size_t used; 
	size_t length; 
}; 
static struct msg_cache log_cache; 
static void *log_callback; 
static void *cb_arg; 
/* 重定向指示 
 * 0为尚未收到重定向命令
 * 1为收到通过信号传来的重定向命令
 */
static bool redict_output; 
struct log_stored_msg
{
	char msg[DEFALUT_MSG_LEN]; 
	char *alloc; 
	char *content; 
}; 
#define IS_DYNAMIC_BY_PTR(ptr) ((ptr)->content == (ptr)->alloc) 
#define IS_STATIC_BY_PTR(ptr)	((ptr)->content == (ptr)->msg)
struct store_cache
{
	struct log_stored_msg store[DEFAULT_STORE]; 
	int next_empty_mem; 
	int first_msg; 
}; 
static struct store_cache stored_msg; 
static int default_output_cb(void*); 
static void dicard_all_stored(); 
static void puts_stored_msg(); 
static bool write_to(log_option , char *, va_list ); 
static void default_ouput_by_signal(); 
static void save_to_append(char*); 
static void str_write(int fd, char *s, size_t len); 
static void print_to_cache(char *s, size_t len); 

void log_init(int (*log_output_callback)(void *),void *arg,  bool no_cache)
{
	static int only_once = 0; 
	if (only_once == 0){
		only_once = 1; 
		enable_cache = true; 
		if (log_output_callback == NULL)
			log_fd = default_output_cb(NULL);  
		else {
			log_fd = log_output_callback(arg); 
			cb_arg = arg; 
			log_callback = (void*)log_output_callback; 
		}
		if (log_fd < 0) {
			fprintf(stderr, "初始化日志系统失败！\n"); 
			exit (ERR_LOG); 
		}
		/* 用户显式禁用缓冲 */
		if (no_cache == true) enable_cache = false; 
		/* 如果为终端则禁用缓冲 */
		if (isatty(log_fd) == 1) enable_cache = false; 

		if (enable_cache) {
			/* 初始化缓存区 */
			log_cache.cache = malloc(MAX_CACHE_LENGTH + 1); 
			if (log_cache.cache == NULL) {
				fprintf(stderr, "初始化日志缓冲区失败！\n"); 
				exit(ERR_ALLOC); 
			}
			log_cache.length = MAX_CACHE_LENGTH; 
			log_cache.used = 0; 
		}
		stored_msg.first_msg =  - 1; 
		stored_msg.next_empty_mem = 0; 
	}
}
int default_output_cb(void *arg)
{
	return STDERR_FILENO; 
}
void inhibit_log()
{
	inhibit = true; 
}
void nohibit_log()
{
	inhibit = false; 
}
void log_fflush()
{
	if (log_cache.used == 0) return; 
	str_write(log_fd, log_cache.cache, log_cache.used); 
	log_cache.used = 0; 
}
bool log_close()
{
	static int only_once = 0; 
	if (only_once == 0) {
		only_once = 1; 
		log_fflush(); 
		if (log_cache.cache != NULL){
			free(log_cache.cache); 
			memset(&log_cache, 0, sizeof(log_cache)); 
		}
		dicard_all_stored(); 
		close (log_fd); 
	}
	return true; 
}
void dicard_all_stored()
{
	struct log_stored_msg *p = stored_msg.store; 
	int i = 0; 
	for (; i < DEFAULT_STORE; i++) {
		if (IS_STATIC_BY_PTR(p)) memset(p->msg, 0, DEFALUT_MSG_LEN); 
		if (IS_DYNAMIC_BY_PTR(p)) free(p->alloc); 
		p->content = NULL; 
	}
	memset(&stored_msg, 0, sizeof(stored_msg)); 
}
void log_printf(log_option option, char *fmt, ...)
{
	va_list args; 
	bool done; 
	static bool is_cache = false; 
	if (inhibit) return; 
	if (enable_cache && is_cache == false) {
		is_cache = true; 
		log_cache.cache = malloc(MAX_CACHE_LENGTH + 1); 
		if (log_cache.cache == NULL) exit(ERR_ALLOC); 
		log_cache.length = MAX_CACHE_LENGTH; 
		log_cache.used = 0; 
	}
	if (redict_output == true) {
		redict_output = false; 
		log_fflush(); 
		default_ouput_by_signal(); 
		puts_stored_msg(); 
	} 
	do {
		va_start(args, fmt); 
		done = write_to(option, fmt, args); 
		if (done && errno == EPIPE) exit (ERR_PIPE); 
		va_end(args); 
	} while (!done); 
}
void log_puts(log_option option, char *msg)
{
	char *m; 
	size_t len = strlen(msg); 
	m = malloc(len + 2); 
	if (m == NULL) exit (ERR_ALLOC); 
	strcpy(m, msg); 
	m[len] = '\n'; 
	m[len + 1] = '\0'; 
	log_printf(option, m); 
	free (m); 
}
void redict_log_output_by_signal(int sig)
{
	redict_output = true; 
}
void redirct_log_ouput(int (*log_output_callback)(void *), void *arg, bool no_cache)
{
	int old_fd = log_fd; 
	log_fflush(); 
	/* 重定向代码 */
	enable_cache = true; 
	if (log_output_callback = NULL)
		return; 
	else {
		log_fd = log_output_callback(arg); 
		if (log_fd < 0) {
			log_fd = old_fd; 
			log_puts(LOG_WARNING, "redict log file failed! still use old output interface!"); 
			return ; 
		}
		close(old_fd); 
		log_callback = (void*)log_output_callback; 
		cb_arg = arg; 
	}
	/* 用户显式禁用缓冲 */
	if (no_cache == true) enable_cache = false; 
	/* 如果为终端则禁用缓冲 */
	if (isatty(log_fd) == 1) enable_cache = false; 

	stored_msg.first_msg =  - 1; 
	stored_msg.next_empty_mem = 0; 
	puts_stored_msg(); 
}
void puts_stored_msg()
{
#define OUT_STORE(ptr) do {\
	str_write(log_fd, (ptr)->content, strlen((ptr)->content)); \
	if (IS_DYNAMIC_BY_PTR((ptr))) {\
		free((ptr)->alloc); \
		(ptr)->alloc = NULL; \
	} else \
	 	memset((ptr)->msg, 0, DEFALUT_MSG_LEN); \
	(ptr)->content = NULL; \
} while (0)
	struct log_stored_msg *p = stored_msg.store; 
	int end = stored_msg.next_empty_mem; 
	int first = stored_msg.first_msg; 
	if (first ==  - 1) return; 
	if (end < first) {
		for (; first < DEFAULT_STORE; first++) 
			OUT_STORE(p + first); 
		for (first = 0; first < end; first++) 
			OUT_STORE(p + first); 
	} else {
		for (; first < end; first++)
			OUT_STORE(p + first); 
	}
	stored_msg.first_msg =  - 1; 
	stored_msg.next_empty_mem = 0; 
#undef OUT_STORE
}
bool write_to(log_option option, char *fmt, va_list args)
{
	static size_t size = 128; 
	char *buff = malloc(size); 
	if (buff == NULL) return false; 
	int num = 0; 
	num = vsnprintf(buff,size - 1, fmt, args); 
	if (num ==  - 1) {
		free(buff); 
		 size <<= 1; 
		 return false; 
	}
	if (num >= size) {
		free(buff); 
		size = num + 1; 
		return false; 
	}
	size_t len = strlen(buff); 
	bool flag = true; 
	buff[len] = '\0'; 
	switch (option) {
		case LOG_ERROR:
			log_fflush(); 
			str_write(log_fd, buff, len); 
			save_to_append(buff); 
			break; 
		case LOG_WARNING:
			if (++warning == MAX_WARNNING_MSG) {
				warning = 0; 
				flag = false; 
				log_fflush(); 
				str_write(log_fd, buff, len); 
				break; 
			}
			if (flag && enable_cache && warning < MAX_WARNNING_MSG)
				print_to_cache(buff, len); 
			if (!enable_cache && flag) str_write(log_fd, buff, len); 
			save_to_append(buff); 
			break; 
		case LOG_VERBOS:
			save_to_append(buff); 
			if (enable_cache)
				print_to_cache(buff, len); 
			else 
				str_write(log_fd, buff, len); 
			break; 
		default:
			break; 
	}
	return true; 
}
void default_ouput_by_signal()
{
	char path[128]; 
	size_t len; 
	close(log_fd); 
	getcwd(path, 126 - strlen(LOG_FILE)); 
	len = strlen(path); 
	if (path[len - 1] != '/') path[len] = '/'; 
	strcat(path, LOG_FILE); 
	path[strlen(path)] = '\0'; 
	log_fd = open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR); 
}
void save_to_append(char *s)
{
	assert(s != NULL); 
	struct log_stored_msg *p = stored_msg.store; 
	int end = stored_msg.next_empty_mem; 
	int first = stored_msg.first_msg; 
	int len  = strlen(s);
	if (first == end)
		if (++(stored_msg.first_msg) == DEFAULT_STORE) stored_msg.first_msg = 0; 
	if (len >= DEFALUT_MSG_LEN) {
		if (IS_DYNAMIC_BY_PTR((p + end))) free((p + end)->alloc); 
		(p + end)->alloc = malloc(len + 1); 
		if ((p + end)->alloc == NULL){
			exit (ERR_ALLOC); 
		}
		strcpy((p + end)->alloc, s); 
		((p + end)->alloc)[len] = '\0'; 
		(p + end)->content = (p + end)->alloc; 
		goto EXIT; 
	}
	strcpy((p + end)->msg, s); 
	((p + end)->msg)[len] = '\0'; 
	(p + end)->content = (p + end)->msg; 
	if (stored_msg.first_msg ==  - 1) stored_msg.first_msg = 0; 
EXIT:
	if (++(stored_msg.next_empty_mem) == DEFAULT_STORE)
		stored_msg.next_empty_mem = 0; 
	return ; 
}
void str_write(int fd, char *s, size_t len)
{
	assert(s != NULL); 
	assert(len != 0); 
	size_t wnt = 0; 
	size_t total = 0; 
	if (len == 0) return; 
AGAIN:
	while ((wnt = write(log_fd, s, len))) {
		if ((wnt + total) < len)
			continue; 
		total += wnt; 
		return; 
	}
	if (wnt < 0) {
		bool flag = true; 
		if (errno == EINTR) goto AGAIN; 
		/* 文件描述符可能失效或被意外关闭 */
		if (errno == EBADF) {
			if (log_callback == NULL) {
				log_fd = default_output_cb(NULL); 
				if (isatty(log_fd)) enable_cache = false; 
			} else {
				int j = 0; 
				for (; j < 3; j++) {
					if ((log_fd = ((int(*)(void*))log_callback)(cb_arg)) >= 0) {
						flag = true; 
						break; 
					}
				}
				if (flag == false) abort(); 
				goto AGAIN; 
			}
		}
	}
}
void print_to_cache(char *s, size_t len)
{
	assert(s != NULL); 
	assert(len >= 0); 
	struct msg_cache *p = &log_cache; 
	if (p->cache == NULL) abort(); 
	if ((p->used + len) >= p->length) log_fflush(); 
	/* 超过缓存的最大大小, 直接输出 */
	if ((p->used + len) >= p->length) str_write(log_fd, s, len); 
	strcpy(p->cache + p->used, s); 
	(p->cache)[p->used + len] = '\0'; 
	p->used += len; 
}
