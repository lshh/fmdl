/*
 * =====================================================================================
 *
 *       Filename:  dns.c
 *
 *    Description:  DNS缓存
 *
 *        Version:  1.0
 *        Created:  2012年12月25日 13时59分48秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "dns.h"
#include "fmdl.h"
#include "log.h"
#include "hash.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DNS_ADDRLISTS_ALIVE_TIME   1800	/* DNS缓存存活时间30分钟 */
/* DNS缓存列表 */
static hash_t *dns_host_address_map; 

#define ENSURE_DNS_MAP_EXISTS do {\
	if (dns_host_address_map == NULL) {\
		dns_host_address_map = hash_new(0, NULL, string_nocase_cmp); \
		assert (dns_host_address_map != NULL); \
	}\
} while (0)
#define DNS_HASH_GET(h) hash_get(dns_host_address_map, h)
#define DNS_HASH_PUT(h, v) hash_put(dns_host_address_map, h, (void*)v)
#define DNS_HASH_REMOVE(h) hash_remove(dns_host_address_map, h)
/* DNS缓存列表操作接口 */
static addrlists_t *dns_query(const char *host); 
static void dns_store(const char *host, addrlists_t *al); 

static int getaddrinfo_timeout(const char *node, 
		const char *service, const struct addrinfo *hints, 
		struct addrinfo **res, uint32_t to); 
static int getaddrinfo_cb(cb_arg_t *arg); 
struct getaddrinfo_arg 
{
	char *node, *service; 
	struct addrinfo *hints, **res; 
}; 
static time_t addrlist_live_time(); 
static void addrlist_destroy(addrlists_t *al); 

addrlists_t *addrlist_new(const struct addrinfo *res)
{
	assert (res != NULL); 
	const struct addrinfo *scan = res; 
	addrlists_t *new = malloc(sizeof(addrlists_t)); 
	assert (new != NULL); 
	uint32_t pos = 0; 
	uint32_t cnt = 2; 
	new->total = new->refcnt = 0; 
	new->addrs = calloc (cnt, sizeof(sockaddr_t)); 
	assert (new->addrs != NULL); 
	do {
		if (pos + 1 > cnt) {
			sockaddr_t *p; 
			cnt = cnt > 8 ? cnt + 8:cnt << 1; 
			p = (sockaddr_t *)calloc(cnt, sizeof(sockaddr_t)); 
			assert ( p != NULL); 
			memcpy(p, new->addrs, sizeof(sockaddr_t)*(pos + 1)); 
			free(new->addrs); 
			new->addrs = p; 
			*(new->addrs + pos) = *(sockaddr_t *)scan->ai_addr; 
		}
		*(new->addrs + pos) = *(sockaddr_t*)scan->ai_addr; 
	} while ((scan = scan->ai_next) && pos++);
	new->total = pos + 1; 
	new->tm = time(NULL); 
	return new; 
}
addrlists_t *addrlist_new_by_ipaddr(int net, const char *host)
{
#define ADD_TO_ADDRLIST(type, v) do {\
	type *v = (type *)new->addrs; \
	(v)->v##_family = net; \
	inet_pton(net, host, &((v)->v##_addr)); \
} while (0)
	if (net != AF_INET && net != AF_INET6) return NULL; 
	addrlists_t *new = calloc(1, sizeof(addrlists_t)); 
	assert(new != NULL); 
	new->tm = time(NULL); 
	new->total = 1; 
	new->addrs = calloc(1, sizeof(sockaddr_t)); 
	assert(new->addrs != NULL); 

	switch (net) {
		case AF_INET:
			ADD_TO_ADDRLIST(struct sockaddr_in, sin); 
			break; 
		case AF_INET6:
			ADD_TO_ADDRLIST(struct sockaddr_in6, sin6); 
			break; 
		default:
			log_debug(LOG_ERROR, "Unknow net type!\n"); 
			free(new->addrs); 
			free(new); 
			return NULL; 
	}
	return new; 
#undef	ADD_TO_ADDRLIST
}
sockaddr_t *addrlist_pos(const addrlists_t *al, int pos)
{
	return al->addrs + pos; 
}
addrlists_t *addrlist_dup( addrlists_t *al)
{
	al->refcnt++; 
	return al; 
}
bool addrlist_alive(const addrlists_t *al)
{
	assert (al != NULL); 
	time_t now = time(NULL); 
	if (al->tm + addrlist_live_time() >= now)
		return false; 
	return true; 
}
time_t addrlist_live_time()
{
	return DNS_ADDRLISTS_ALIVE_TIME; 
}
uint32_t addrlist_total(const addrlists_t *al)
{
	return al->total; 
}
void addrlist_release(addrlists_t *al)
{
	assert(al != NULL); 
	if (--al->refcnt <= 0) addrlist_destroy(al); 
}
void addrlist_destroy(addrlists_t *al)
{
	assert(al != NULL); 
	free(al->addrs); 
	free(al); 
}
addrlists_t *lookup_host(const char *host, bool cache)
{
	assert (host != NULL); 
	addrlists_t *ret; 
	size_t len = strlen(host); 

	if (valid_ipv4_addr(host, len)) {
		return addrlist_new_by_ipaddr(AF_INET, host); 
	} else if (valid_ipv6_addr(host, len)) {
		return addrlist_new_by_ipaddr(AF_INET6, host);
	}

	if (cache) {
		ret = dns_query(host); 
		if (ret != NULL) return ret; 
	}

	/*
	 * DNS 查找
	 */
	{
		struct addrinfo hints, *res = NULL; 
		int rnt; 
		memset(&hints, 0, sizeof(hints)); 
		hints.ai_family = AF_UNSPEC; 
		hints.ai_socktype = SOCK_STREAM; 
		rnt = getaddrinfo_timeout(host, NULL, 
				&hints, &res, options.dns_timout); 
		if (rnt != NO_ERROR) {
			log_debug(LOG_ERROR, "DNS search FAILED!\n"); 
			return NULL; 
		}
		ret = addrlist_new(res); 
	}
	if (cache) dns_store(host, ret); 
	return ret; 
}
int getaddrinfo_timeout(const char *node, 
		const char *service, const struct addrinfo *hints, 
		struct addrinfo **res, uint32_t to) 
{
	assert (res != NULL); 
	struct getaddrinfo_arg arg; 
	cb_arg_t cb_arg; 

	int ret; 
	arg.node = node; 
	arg.service = service; 
	arg.hints = hints; 
	arg.res = res; 
	cb_arg.cb_fd = -1; 
	cb_arg.cb_ext = (void *)&arg; 
	ret = run_cb_with_timeout(getaddrinfo_cb, &cb_arg, to); 
	return ret; 
}
int getaddrinfo_cb(cb_arg_t *arg)
{
	assert(arg != NULL); 
	int ret; 
	struct getaddrinfo_arg *cb_arg; 
	cb_arg = (struct getaddrinfo_arg *)arg->cb_ext; 
	ret = getaddrinfo(cb_arg->node, cb_arg->service, 
			cb_arg->hints, cb_arg->res); 
	if (ret == 0) return NO_ERROR; 
	return ERR_NET; 
}
addrlists_t *dns_query(const char *host) 
{
	assert(host != NULL); 
	
	addrlists_t *al; 
	ENSURE_DNS_MAP_EXISTS; 
	al = (addrlists_t *)DNS_HASH_GET(host); 
	if (!al) return NULL; 
	if (!addrlist_alive(al)) {
		DNS_HASH_REMOVE(host); 
		addrlist_release(al); 
		return NULL; 
	}
	return addrlist_dup(al); 
}
void dns_store(const char *host, addrlists_t *al)
{
	assert (host != NULL && al != NULL); 
	ENSURE_DNS_MAP_EXISTS; 
	addrlists_t *old = (addrlists_t *)DNS_HASH_GET(host); 
	if (old) {
		DNS_HASH_REMOVE(host); 
		addrlist_release(old); 
		return ; 
	}
	/*
	 * XXXX此处调用addrlist_dup是必须的XXXX
	 * 假设直接以al为参数，则此时al->refcnt
	 * 等于0, 如果下次调用dns_query查询到此
	 * 地址列表，用户再使用lookup_host后调用
	 * addrlist_release释放则此时内存会被释放，
	 * 但是哈希列表中将依然存有到此内存的地址，
	 * 再次调用dns_query将可能产生严重错误!
	 */
	DNS_HASH_PUT(host, addrlist_dup(al));
}
#undef ENSURE_DNS_MAP_EXISTS
