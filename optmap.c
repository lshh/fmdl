/*
 * =====================================================================================
 *
 *       Filename:  optmap.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年12月03日 14时50分08秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "fmdl.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
 
#define HELP_EXIT(id) do {\
	fprintf(stderr, "lost argument!\n"); \
	print_help();\
   	exit((id)); \
} while(0)
#define DEFAULT_HEADS	8
#define SEP				','
static void set_bool(void *arg1); 
static void get_string(void*arg); 
static void get_multi_string(void*arg); 
static void get_val(void*arg); 

struct options_map optmap[] = {
	{OPT_BOOL, set_bool, &(options.background)}, 
	{OPT_LINE, get_string, &(options.log_output)}, 
//	{OPT_BOOL, set_bool, &(options.append_log)}, 
	{OPT_LINE, get_string, &(options.input_file)}, 
	{OPT_BOOL, set_bool, &(options.as_html)}, 
	{OPT_LINE, get_string, &(options.merge_url)}, 
	{OPT_BOOL, set_bool, &(options.quiet)}, 
	{OPT_BOOL, set_bool, &(options.only_save)}, 
	{OPT_VALUE, get_val, &(options.ntrys)}, 
	{OPT_LINE, get_string, &(options.output)}, 
	{OPT_BOOL, set_bool, &(options.clobber)}, 
	{OPT_BOOL, set_bool, &(options.print_response)}, 
	{OPT_BOOL, set_bool, &(options.contin)}, 
	{OPT_VALUE, get_val, &(options.timeout)}, 
	{OPT_VALUE, get_val, &(options.dns_timout)}, 
	{OPT_VALUE, get_val, &(options.connect_timeout)}, 
	{OPT_VALUE, get_val, &(options.read_timeout)}, 
	{OPT_VALUE, get_val, &(options.wait_time)}, 
	{OPT_BOOL, set_bool, &(options.radom_wait)}, 
	{OPT_BOOL, set_bool, &(options.proxy)}, 
	{OPT_LINE, get_string, &(options.bind_addr)}, 
	{OPT_VALUE, get_val, &(options.limit_rate)}, 
	{OPT_BOOL, set_bool, &(options.only_ipv4)}, 
	{OPT_BOOL, set_bool, &(options.only_ipv6)}, 
	{OPT_LINE, get_string, &(options.username)}, 
	{OPT_LINE, get_string, &(options.passwd)}, 
	{OPT_VALUE, get_val, &(options.quota)}, 
	{OPT_BOOL, set_bool, &(options.discard_last_task)}, 
	{OPT_LINE, get_string, &(options.http_user)}, 
	{OPT_LINE, get_string, &(options.http_passwd)}, 
	{OPT_BOOL, set_bool, &(options.nocache)}, 
	{OPT_BOOL, set_bool, &(options.nodnscache)}, 
	{OPT_LINE, get_string, &(options.default_page)}, 
	{OPT_MUTIL, get_multi_string, &(options.add_heads)}, 
	{OPT_VALUE, get_val, &(options.max_redic)}, 
	{OPT_LINE, get_string, &(options.referurl)}, 
	{OPT_LINE, get_string, &(options.proxy_name)}, 
	{OPT_LINE, get_string, &(options.proxy_passwd)}, 
	{OPT_LINE, get_string, &(options.user_agent)}, 
	{OPT_BOOL, set_bool, &(options.cookie)}, 
	{OPT_LINE, get_string, &(options.load_cookie)}, 
	{OPT_LINE, get_string, &(options.save_cookie)}, 
	{OPT_BOOL, set_bool, &(options.passive)}, 
	{OPT_LINE, get_string, &(options.ftp_user)}, 
	{OPT_LINE, get_string, &(options.ftp_passwd)}, 
	{OPT_BOOL, set_bool, &(options.ftp_glob)}, 
	{OPT_BOOL, set_bool, &(options.recursive)}, 
	{OPT_VALUE, get_val, &(options.max_level)}, 
	{OPT_BOOL, set_bool, &(options.delete_after)}, 
	{OPT_BOOL, set_bool, &(options.convert_links)}, 
	{OPT_BOOL, set_bool, &(options.backup)}, 
	{OPT_LINE, get_string, &(options.bak)}, 
	{OPT_MUTIL, get_multi_string, &(options.accept_list)}, 
	{OPT_MUTIL, get_multi_string, &(options.reject_list)}, 
	{OPT_MUTIL, get_multi_string, &(options.domain_list)}, 
	{OPT_MUTIL, get_multi_string, &(options.domain_reject_list)}, 
	{OPT_BOOL, set_bool, &(options.follow_ftp)}, 
	{OPT_MUTIL, get_multi_string, &(options.follow_tags)}, 
	{OPT_MUTIL, get_multi_string, &(options.ignore_tags)}, 
	{OPT_BOOL, set_bool, &(options.relative)}, 
	{OPT_BOOL, set_bool, &(options.robots)}, 
	{OPT_MUTIL, get_multi_string, &(options.include_dir)}, 
	{OPT_MUTIL, get_multi_string, &(options.exclue_dir)}
}; 
void set_bool(void *arg)
{
	bool val = *(bool*)arg; 
	*(bool*)arg = val == false?true:false; 
}
void get_string(void *arg)
{
	char **option = (char**)arg; 
	size_t len; 
	if (optarg == NULL) 
		HELP_EXIT(1); 
	len = strlen(optarg); 
	*option = malloc(len + 1); 
	strncpy(*option, optarg, len); 
	(*option)[len] = '\0'; 
}
void get_val(void *arg)
{
	unsigned long val; 
	errno = 0; 
	char *endptr; 
	char c = 0; 
	if (arg == NULL) 
		HELP_EXIT(1); 
	val = strtol(optarg, &endptr, 10); 
	if (errno == EINVAL || errno == ERANGE) 
		HELP_EXIT(errno); 
	if (endptr != NULL) c = tolower(*endptr); 
	//磁盘配额选项需要特殊处理
	if (arg == &(options.quota)) {
		switch (c) {
			case 'b':
				*(unsigned long*)arg = val < 1024?1:val >> 10; 
				break; 
			case 'm':
				*(unsigned long*)arg = val << 10; 
				break;
			case 'g':
				*(unsigned long*)arg = val << 20; 
				break; 
			default:
				*(unsigned long*)arg = val; 
		}
		return; 
	}
	*(long*)arg = val; 
}
void get_multi_string(void *arg)
{
	char ***s = (char***)arg; 
	char *b, *e; 
	char **p; 
	int i = 0; 
	bool flag = true; 
	size_t len = DEFAULT_HEADS*sizeof(char*); 
	*s = calloc(1, len + 1); 
	p = *s; 
	b = optarg; 
	while ((e = strchr(b, SEP)) && flag) {
		 *(p + i)= malloc(e - b + 1); 
		strncpy(*(p + i), b, e - b); 
		(*(p + i))[e - b] = '\0'; 
		b = e + 1; 
		/* 防止最后一个字符为','导致assert判断失败 */
		if (*b == '\0') flag = false; 
		i++; 
		if (i >= len) {
			*s = calloc(1, (len << 1) + 1); 
			memcpy(*s, p, len); 
			p = *s; 
		}
	}
	if (flag == false) return ; 

	assert(b != NULL); 
	assert(e == NULL);
	*(p + i) = malloc(strlen(b) + 1); 
	strcpy(*(p + i), b); 
	(*(p + i))[strlen(b)] = '\0'; 
}
