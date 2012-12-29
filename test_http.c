/*
 * =====================================================================================
 *
 *       Filename:  test_http.c
 *
 *    Description:  测试HTTP模块
 *
 *        Version:  1.0
 *        Created:  2012年12月29日 18时34分39秒
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
	if (!http_connect(h, "www.ifeng.com", 80)){
		printf("ERROR\n");
		return 1; 
	}
	http_set_req(h, "GET", "/"); 
	add_head_for_req(h, "Host: www.ifeng.com"); 
	add_head_for_req(h, "User-Agent: wget/1.13"); 
	add_head_for_req(h, "Accept: text/html"); 
	add_head_for_req(h, "Accept-Language: zh-cn,zh;q=0.8, en-us;q=0.5,en;q=0.3"); 
	add_head_for_req(h, "Connection:keep-live"); 
	http_send_request(h); 
	http_get_response(h); 
	printf("Status:%d\n", http_status(h)); 
	const char **p = http_resp_heads(h); 
	http_close(h); 
	return 0;
}
