/*
 * =====================================================================================
 *
 *       Filename:  hash.h
 *
 *    Description:  散列结构以及接口
 *
 *        Version:  1.0
 *        Created:  2012年11月27日 10时47分30秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef HASH_T0QUF7L5

#define HASH_T0QUF7L5

#include <stdio.h>
#include <stdint.h>
#include "error_code.h"
typedef struct cell
{
	void *key; 
	void *value; 
}cell_t; 
typedef uint32_t (*hash_function_t)(const void*); 
typedef int (*test_function_t)(const void*, const void*); 

typedef struct hash_tb
{
	hash_function_t	 hash_fun; 
	test_function_t test_fun; 
	cell_t *cell; 
	uint32_t counts; 		/* 元素个数 */
	uint32_t size; 		/* 散列大小 */
	uint32_t reset_size; 	/* 重新分配大小的阀值 */
	uint8_t cur_offset; 
} hash_t; 

/* ***********************************接口*************************************** */
/*
 * 创建新的散列
 */
 hash_t *hash_new(uint32_t hsize, hash_function_t hfun, test_function_t htest ); 
 void hash_destroy(hash_t *ht); 
 bool hash_put(hash_t *ht, const void *key, void *value); 
 bool hash_put_cell(hash_t *ht, cell_t *cell); 
 void *hash_get(hash_t *ht, const void *key); 
 bool  hash_get_pair(hash_t *ht, const void *key, const void **k, const void **v); 
 bool hash_get_cell(hash_t *ht, const void *key, cell_t *cell); 
 bool hash_contains(hash_t *ht, const void *key); 
 bool hash_remove(hash_t *ht, const void *key); 
 void hash_clear(hash_t *ht); 
 int string_nocase_cmp(const void *s1, const void *s2); 
#endif /* end of include guard: HASH_T0QUF7L5 */
