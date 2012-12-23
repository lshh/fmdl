/*
 * =====================================================================================
 *
 *       Filename:  fmdl.c
 *
 *    Description:  主函数
 *
 *        Version:  1.0
 *        Created:  2012年12月03日 09时23分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "log.h"
#include "task_schedue.h"
#include "fmdl.h"
#include "error_code.h"
#include "url.h"
#include "download.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#define VERSION		fmdl/1.0
static void print_version(); 
void initial(); 
static int open_file(void*); 
extern struct options_map optmap[]; 
int main(int argc,  char *argv[])
{
	struct option long_option[] = {
		//generic
		{"background", 0, NULL, 0}, 
		{"log-output", 1, NULL, 0}, 
//		{"append-log", 0, NULL, 0}, 
		{"input-file", 1, NULL, 0},
		{"as-html", 0, NULL, 0}, 
		{"merge-url", 1, NULL, 0}, 
		{"quiet", 0, NULL, 0}, 
		{"only-save", 0, NULL, 0}, 
		//download
		{"try-times", 1, NULL, 0}, 
		{"output", 1, NULL, 0}, 
		{"clobber", 0, NULL, 0}, 
		{"print-response", 0, NULL, 0}, 
		{"continue", 0, NULL, 0}, 
		{"timeout", 1, NULL, 0}, 
		{"dns-timeout", 1, NULL, 0}, 
		{"connect-timeout", 1, NULL, 0}, 
		{"read-timeout", 1, NULL, 0}, 
		{"wait-time", 1, NULL, 0}, 
		{"random-time", 0, NULL, 0}, 
		{"proxy", 0, NULL, 0}, 
		{"bind-address", 1, NULL, 0}, 
		{"limit-rate", 1, NULL, 0}, 
		{"only-ipv4", 0, NULL, 0}, 
		{"only-ipv6", 0, NULL, 0}, 
		{"username", 1, NULL, 0}, 
		{"passwd", 1, NULL, 0}, 
		{"quota", 1, NULL, 0}, 
		{"discard-last-task", 0, NULL, 0}, 
		//HTTP
		{"http-user", 1, NULL, 0}, 
		{"http-passwd", 1, NULL, 0}, 
		{"no-cache", 0, NULL, 0}, 
		{"no-DNS-cache", 0, NULL, 0}, 
		{"default-page", 1, NULL, 0}, 
		{"add-head", 1, NULL, 0}, 
		{"max-redictor", 1, NULL, 0}, 
		{"referer", 1, NULL, 0}, 
		{"proxy-name", 1, NULL, 0}, 
		{"proxy-passwd", 1, NULL, 0}, 
		{"user-agent", 1, NULL, 0}, 
		{"cookie",  0, NULL, 0}, 
		{"load-cookie", 1, NULL, 0}, 
		{"save-cookie", 1, NULL, 0}, 
		//FTP
		{"no-passive", 0, NULL, 0}, 
		{"ftp-user", 1, NULL, 0}, 
		{"ftp-passwd", 1, NULL, 0}, 
		{"ftp-global", 0, NULL, 0}, 
		//递归
		{"recursive", 0, NULL, 0}, 
		{"limit-level", 1, NULL, 0}, 
		{"delete-after", 0, NULL, 0}, 
		{"convert-links", 0, NULL, 0}, 
		{"backup", 0, NULL, 0}, 
		{"back-suffix", 1, NULL, 0}, 
		{"accept-list", 1, NULL, 0}, 
		{"reiect-list", 1, NULL, 0}, 
		{"domain-list", 1, NULL, 0}, 
		{"domain-reject-list", 1, NULL, 0}, 
		{"follow-tag", 1, NULL, 0}, 
		{"ingore-tags", 1, NULL, 0}, 
		{"relative", 0, NULL, 0}, 
		{"robots", 0, NULL, 0}, 
		{"include-dir", 1, NULL, 0}, 
		{"exclude-dir", 1, NULL, 0}, 
		{NULL, 0, NULL, 0}
	}; 
	int c; 
	int index = 0; 
	int nurls; 
	uint16_t task_id = 1; 
	initial(); 
	opterr = 0; 
	while ((c = getopt_long(argc, argv, "", long_option, &index)) != -1) {
		(optmap + index)->fun((optmap + index)->arg); 
	}
	nurls = argc - optind; 	/* 要下载的链接数目 */
	/* 冲突选项 */
	if (options.only_ipv4 && options.only_ipv6) {
		fprintf(stderr, "can`t specify both only-ipv4 and only-ipv6\n"); 
		exit(ERR_OPTIONS); 
	}

	if (options.input_file) {
		/* 日志重定向到文件，启用缓存 */
		redirct_log_ouput(open_file, options.log_output, false); 
	}
	//进入安静模式
	if (options.quiet) inhibit_log(); 
	task_id = init_task_queue(!options.discard_last_task); 

	if (nurls) {
		int i = 0; 
		for (; i < nurls; i++, optind++) {
			char *rewrite = reset_short_url(argv[optind]); 
			task_t task; 
			task.task_url = rewrite?rewrite:argv[optind]; 
			/* 应该具有最高优先级 */
			if (options.recursive) SET_TASK_PRIORITY(task,TO_RECU|0x00); 
			else SET_TASK_PRIORITY(task, TO_SINGLE|0x00); 
			SET_TASK_ID(task, task_id); 
			if (insert_new_task(&task)) 
				task_id++; 
			else 
				log_printf(LOG_ERROR, "can`t insert a new task in queue!discard the url(%s)\n", 
						GET_TASK_URL(task)); 
		}
	}
	if (options.input_file) {
		//从输入文件中读入链接
	}
	if (total_tasks() == 0) {
		fprintf(stderr, "There is no URL to download! Exit normally\n"); 
		exit(0); 
	}
	if(options.only_save) {
		fprintf(stderr, "NOW save all the tasks! program exit normally!\n"); 
		save_tasks_queue_to_file(); 
		exit(0); 
	}
	if (options.username && !options.passwd) {
		ask_for_passwd(); 
		if (options.passwd == NULL || options.passwd[0] == '\0')
			exit(ERR_OPTIONS); 
	}
	if (options.background) fork_to_background(); 
	//开始下载
	task_type_t *task; 
	while ((task = get_task_to_dl())) {
		char *url = GET_TASK_URL(GET_TASK_BY_PTR(task)); 
		int err; 
		url_t *nurl = url_parsed(url, &err); 
		if (err != NO_ERROR) {
			log_printf(LOG_WARNING, "bad url(%s) discard it\n", url); 
			continue; 
		}
		//递归下载
		uint8_t type = GET_TASK_PRIORITY(GET_TASK_BY_PTR(task)); 
		/* 因为暂时不使用其他优先级队列因此这也判断是没有问题的 */
		if (type & TO_RECU) {
			recursive_download(nurl, url); 
		} else if (type & TO_SINGLE) {
			start_download(nurl, url); 
		} else {
			log_printf(LOG_WARNING, "unknown URL(%s) level\n", url); 
		}
		free(task); 
		free(url); 
	}
	/*
	 * 清理工作
	 */
	//(1) 打印信息
	//(2) 是否保存cookie
	//(3) 如果装换链接则转换
	//(4)其他清理工作
	log_close(); 
	return 0;
}
void initial()
{
	log_init(NULL, NULL, true); 
	memset(&options, 0, sizeof(options)); 
	options.passive = true; 
	options.robots = true; 
	options.max_level = -1; 	//默认递归下载时递归层次无限制
	options.follow_ftp = true; 
}
void print_help()
{
}
void print_version()
{
}
int open_file(void*arg)
{
	assert(arg != NULL); 
	return open(arg, O_WRONLY|O_CREAT, 0622); 
}
