/*
 * =====================================================================================
 *
 *       Filename:  net.h
 *
 *    Description:  处理网络链接模块
 *
 *        Version:  1.0
 *        Created:  2012年12月07日 19时58分54秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef NET_D29XEW7D

#define NET_D29XEW7D

#include "error_code.h"
#include "log.h"

bool valid_ipv6_addr(const char *s, size_t len); 
bool valid_ipv4_addr(const char *s, size_t len); 
#endif /* end of include guard: NET_D29XEW7D */
