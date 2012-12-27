/*
 * =====================================================================================
 *
 *       Filename:  test_ftp.c
 *
 *    Description:  测试FTP模块
 *
 *        Version:  1.0
 *        Created:  2012年12月27日 13时45分32秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "fmdl.h"
#include "dns.h"
#include "net.h"
#include "hash.h"
#include "ftp.h"
#include "log.h"
#include "error_code.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, const char *argv[])
{
	char buf[4096] = {0};
	log_init(NULL, NULL, false); 
	options.print_response = true; 
	ftp_t *f = ftp_new("lshh", "071818"); 
	ftp_connect(f, "127.0.0.1", 21); 
	int ret = ftp_data(f,false); 
	if (ret != NO_ERROR) return 1; 
	ret = ftp_command(f, "LIST ./code"); 
	while ((ret = sock_read(f->data, buf, 4096))); 
	printf("%s", buf); 
	printf("%d\n", ret); 
	return 0;
}
