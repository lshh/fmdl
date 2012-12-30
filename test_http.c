/*
 * =====================================================================================
 *
 *       Filename:  test_http.c
 *
 *    Description:  测试HTTP模块
 *
 *        Version:  1.0
 *        Created:  2012年12月29日 22时21分02秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char *argv[])
{
	http_t *h = http_new(NULL, NULL); 
	if (!http_connect(h, "www.ifeng.com", 80)) {
		fprintf(stderr, "Connect to remote FAILED!\n"); 
		return -1; 
	}

	http_set_req(h, "GET", "/"); 
	add_head_for_req(h, "Host: www.ifeng.com"); 
	add_head_for_req(h, "Accept: text/html"); 
	add_head_for_req(h, "User-Agent: fmdl/1.0 beta"); 
	add_head_for_req(h, "Connection: keep-live"); 
	may_add_head_for_req(h, "Accept: img/gif"); 
	printf("添加的首部为：\n"); 
	print_http_heads(h, false); 
	/* 发送请求 */
	http_send_request(h); 
	http_get_response(h); 

	printf("响应首部为:\n"); 
	print_http_heads(h, true); 

	http_close(h); 
	return 0;
}
