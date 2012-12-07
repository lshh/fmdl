/*
 * =====================================================================================
 *
 *       Filename:  testip.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年12月07日 12时57分17秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "error_code.h"
bool valid_ipv4_addr(const char *s, size_t len)
{
	assert(s != NULL); 
	assert(len > 0); 

	int cnt = 0; 
	int sum = 0; 
	const char *p = s; 
	bool first = true; 
	while (*p && len--) {
		if (isdigit(*p)) {
		   	sum = sum*10 + *p - '0'; 
			first = false; 
		} else if (*p == '.' && first == false) {
			if (sum > 255) return false; 
			if (++cnt > 3) return false; 
			sum = 0; 
		} else 
			return false; 
		p++; 
	}
	if (cnt != 3 || sum > 255) return false; 
	return true; 
}
bool valid_ipv6_addr(const char *s, size_t len)
{
	assert(s != NULL); 
	assert(len > 0); 
	const char *p = s; 
	int bit_width = 16; 
	bool flag = false; 
	int sum = 0; 
	if (*p == ':' && *(p + 1) == ':') {
		bit_width += 16; 
		flag = true; 
		p += 2; 
		len -= 2; 
	}
	while (*p && len--) {
		if (isdigit(*p)) {
			sum = sum*10 + *p - '0'; 
		} else if (*p == ':') {
			bit_width += 16; 
			if (sum > 0xffff) return false; 
			if (*(p + 1) == ':' && !flag) {
				flag = true; 
				p += 2; 
				sum = 0; 
				continue; 
			}else if (*(p + 1) == ':' && flag) 
				return false; 
			sum = 0; 
		} else if (*p == '.') {
			char *b = p; 
			while (*b != ':' && b != s) b--; 
			if (b == s) return false; 
			if (!isdigit(*b)) return false; 
			++b; 
			if (!valid_ipv4_addr(b, strlen(b))) return false; 
			bit_width += 16; 
			break; 
		} else 
			return false; 
		p++; 
	}
	if (flag && bit_width > 128) return false; 
	if (!flag && bit_width != 128) return false; 
	return true; 
}
int main(int argc, const char *argv[])
{
	const char *s[] = {
		"90:123:23:32:534:32:21:12:234", 	//false
		"90:12:23:43:89:65536:12:34", 		//false
		"123:23:32:534:32:21:12:234", 		//true
		"::1::23:32:45", 					//false
		"::1", 								//true
		"128::233:192.168.0.1", 			//true
		"123:23:534:32::192.168.1.0", 		//true
		"192.168.1.23::1:89:342:32", 
		":1:192.168.1.1:0:192.168.1.1", 
		"123:34:65:42:31:32:56:192.168.1.0", //false
		"65536::1", 						//false
		"65535:32:323:325:534:32::192.168.1.", //true
		"12:23:43:32:32:54:192.168.0.1", 		//ture
		"192.168.1.1", 
		"0::192.168.1.1", 
		"0:::192.148.8.1", 
		"123::1:192.168.0.1" 					//true
	}; 
	int i = 0; 
	for (; i < sizeof(s)/sizeof(s[0]); i++)
		printf("%s %s\n", s[i], valid_ipv6_addr(s[i], strlen(s[i]))?"true":"flase"); 
	return 0;
}
