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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#define VERSION		fmdl/1.0
static void print_version(); 
void initial(); 
extern struct options_map optmap[]; 
int main(int argc,  char *argv[])
{
	struct option long_option[] = {
		//generic
		{"background", 0, NULL, 0}, 
		{"log-output", 1, NULL, 0}, 
		{"append-log", 0, NULL, 0}, 
		{"input-file", 1, NULL, 0},
		{"as-html", 0, NULL, 0}, 
		{"merge-url", 1, NULL, 0}, 
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
	initial(); 
	if (argc < 2) {
		print_help(); 
		exit(1); 
	}
	while ((c = getopt_long(argc, argv, "", long_option, &index)) != -1) {
		(optmap + index)->fun((optmap + index)->arg); 
	}
	/* 冲突选项 */
	return 0;
}
void initial()
{
	log_init(NULL, NULL, true); 
	memset(&options, 0, sizeof(options)); 
	options.passive = true; 
	options.robots = true; 
}
void print_help()
{
}
void print_version()
{
}
