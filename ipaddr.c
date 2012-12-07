/*
 * =====================================================================================
 *
 *       Filename:  ipaddr.c
 *
 *    Description:  判断IP地址
 *
 *        Version:  1.0
 *        Created:  2012年12月07日 12时46分34秒
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

bool valid_ipv6_addr(const char *s, size_t len); 
bool valid_ipv4_addr(const char *s, size_t len); 

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
