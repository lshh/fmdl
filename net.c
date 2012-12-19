/*
 * =====================================================================================
 *
 *       Filename:  net.c
 *
 *    Description:  网络接口
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
#include "net.h"
#include "error_code.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

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
			if (cnt == 3 && *(p + 1) == '\0') return false; 
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
	//如果只有两个::
	if (len == 0) return false; 

	while (*p && len--) {
		if (is_digit(*p)) {
			if (isdigit(*p)) {
				sum = sum*16 + *p - '0'; 
				p++; 
				continue; 
			}
			char c = tolower(*p); 
			if (c < 'a' || c > 'f') return false; 
			sum = sum*16 + c - 'a' + 10; 
		} else if (*p == ':') {
			bit_width += 16; 
			if (sum > 0xffff) return false; 
			if (*(p + 1) == ':' && !flag) {
				flag = true; 
				p += 2; 
				if (*p == ':') return false; 
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
bool is_digit(char c)
{
	switch(c) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F':
			return true; 
		default:
			return false; 
	}
	abort(); 
}