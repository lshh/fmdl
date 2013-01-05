/*
 * =====================================================================================
 *
 *       Filename:  process.h
 *
 *    Description:  下载状态输出接口
 *
 *        Version:  1.0
 *        Created:  2013年01月04日 14时48分00秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef PROCESS_TGR5YX9N

#define PROCESS_TGR5YX9N

#include "log.h"

#include <stdint.h>
/*
 * 创建一个新的process_t 结构 total为数据总量
 */
typedef struct _process_t {
	uint64_t total; 	/*  */
	uint64_t already; 
	char *line; 
} process_t;

process_t *process_init(uint64_t total); 
void process_update(uint64_t dl_byte); 
void process_destroy(process_t *p); 

#endif /* end of include guard: PROCESS_TGR5YX9N */
