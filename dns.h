/*
 * =====================================================================================
 *
 *       Filename:  dns.h
 *
 *    Description:  DNS缓存
 *
 *        Version:  1.0
 *        Created:  2012年12月23日 14时30分45秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef DNS_NZTNTPVH

#define DNS_NZTNTPVH

#include "net.h"

#include <stdint.h>
#include <time.h>

#define SOCKADDR struct sockaddr
typedef struct _addrlists_t {
	sockaddr_t *addrs; 
	time_t tm; 
	uint32_t total; 
	uint32_t refcnt; 
} addrlists_t;
addrlists_t *addrlist_new(const struct addrinfo *res); 
addrlists_t *addrlist_new_by_ipaddr(int net, const char *host); 
sockaddr_t *addrlist_pos(const addrlists_t *al, int pos); 
addrlists_t *addrlist_dup(addrlists_t *al); 
bool addrlist_alive(const addrlists_t *al); 
uint32_t addrlist_total(const addrlists_t *al); 
void addrlist_release(addrlists_t *al); 
void addrlist_referer(addrlists_t *al); 
addrlists_t *lookup_host(const char *host, bool cache); 
#endif /* end of include guard: DNS_NZTNTPVH */
