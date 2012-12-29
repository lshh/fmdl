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
	http_set_req(h, "GET", "www.ifeng.com/index.html"); 
	int i = 10; 
	while (i--)
		add_head_for_req(h, "Accept: text/* image/*"); 
	return 0;
}
