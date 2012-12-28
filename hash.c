/*
 * =====================================================================================
 *
 *       Filename:  hash.c
 *
 *    Description:  散列实现
 *
 *        Version:  1.0
 *        Created:  2012年11月27日 10时56分39秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "hash.h"
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#define RESIZE_HASH_VAL 0.75

#define CELL_EXIST(c) ((c)->key != NULL)
#define CELL_NOT_EXIST(c) ((c)->key == NULL)
#define SET_CELL_VALUE(c, v) ((c)->value = (void*)(v))
#define SET_CELL_KEY(c, k) ((c)->key = (void*)(k))
#define CLEAR_CELL(c) ((c)->key = NULL)
#define NEXT_CELL(cells, c, size)  ((c) != ((cells) + (size) - 1)?(c) + 1:(cells))
#define HASH_POSITON(k, ht, size) (((ht)->hash_fun)((k))%(size))
#define countof(array) (sizeof((array))/(array)[0])

static uint32_t default_hash_function(const void*); 
static int default_test_function(const void*, const void *); 
static uint32_t prime_size(uint32_t size, uint8_t *prime_offset); 
static hash_t *grow_hash(hash_t *); 
static cell_t *find_cell(hash_t *, const void *); 

hash_t *hash_new(uint32_t hsize, hash_function_t  hfun, test_function_t htest)
{
	uint32_t size; 
	hash_t *new = malloc(sizeof(hash_t)); 
	if (new == NULL) return NULL; 

	new->hash_fun = (hfun == NULL?default_hash_function:hfun); 
	new->test_fun = (htest == NULL?default_test_function:htest); 

	new->cur_offset = 0; 
	size = 1 + hsize/RESIZE_HASH_VAL; 
	new->size = prime_size(size, &(new->cur_offset)); 
	new->reset_size = (new->size)*RESIZE_HASH_VAL; 
	new->cell = calloc(new->size, sizeof(cell_t)); 
	if (new->cell == NULL) {
		free(new); 
		return NULL; 
	}
	new->counts = 0; 
	return new; 
}
void hash_destroy(hash_t *ht)
{
	assert(ht != NULL); 
	if (ht->cell) free (ht->cell); 
	free (ht); 
}
bool hash_put(hash_t *ht, const void *key, void *value)
{
	assert(ht != NULL); 
	assert(key != NULL); 
	assert(value != NULL); 
	cell_t *c = find_cell(ht, key); 
	if (CELL_EXIST(c)) {
		SET_CELL_VALUE(c, value); 
		return true; 
	}
	if (ht->counts >= ht->reset_size) {
		ht = grow_hash(ht); 
		c = find_cell(ht, key); 
	}
	ht->counts++; 
	SET_CELL_VALUE(c, value); 
	SET_CELL_KEY(c, key); 
	return true; 
}
bool hash_put_cell(hash_t *ht, cell_t *cell)
{
	assert(cell != NULL); 
	assert(ht != NULL); 
	return hash_put(ht, cell->key, cell->value); 
}
bool hash_get_pair(hash_t *ht, const void *key, const void **k, const void **v)
{
	assert(ht != NULL); 
	assert(key != NULL); 
	void *p = hash_get(ht, key); 
	if (p != NULL) {
		if (k) *k = key; 
		if (v) *v = p; 
		return true; 
	}
	return false; 
}
void *hash_get(hash_t *ht, const void *key)
{
	cell_t *c = find_cell(ht, key); 
	if(CELL_NOT_EXIST(c)) return NULL; 
	return c->value; 
}
bool hash_get_cell(hash_t *ht, const void *key, cell_t *c)
{
	assert (ht != NULL); 
	assert(key != NULL); 
	assert(c != NULL); 
	void *p = hash_get(ht, key); 
	if (p == NULL)  return false; 
	c->key = (void*)key; 
	c->value = p; 
	return true; 
}
bool hash_contains(hash_t *ht, const void *key)
{
	assert(ht != NULL); 
	assert(key != NULL); 
	cell_t *c = find_cell(ht, key); 
	return CELL_EXIST(c)?true:false; 
}
bool hash_remove(hash_t *ht, const void *key)
{
	assert(ht != NULL); 
	assert(key != NULL); 
	cell_t *c = find_cell(ht, key); 
	if (CELL_NOT_EXIST(c)) return false; 
	CLEAR_CELL(c); 
	ht->counts--; 
	c = NEXT_CELL(ht->cell, c, ht->size); 
	/* 处理键值冲突 */
	for (; CELL_EXIST(c); c = NEXT_CELL(ht->cell, c, ht->size)) {
		void *k2 = c->key; 
		cell_t *nc = ht->cell + HASH_POSITON(k2, ht, ht->size); 
		for (; CELL_EXIST(nc); nc = NEXT_CELL(ht->cell, nc, ht->size)) 
			if (nc->key == k2) break; 
		*nc = *c; 
		CLEAR_CELL(c); 
	}
	return true; 
}
void hash_clear(hash_t *ht)
{
	memset(ht->cell, 0, sizeof(ht->cell)); 
	ht->counts = 0; 
}
uint32_t default_hash_function(const void *p)
{
	uintptr_t key = (uintptr_t)p; 
	key += (key << 12);
	key ^= (key >> 22);
	key += (key << 4);
	key ^= (key >> 9);
	key += (key << 10);
	key ^= (key >> 2);
	key += (key << 7);
	key ^= (key >> 12);
	return (uint32_t)key; 
}
int default_test_function(const void *s1, const void *s2)
{
	return s1 == s2; 
}
hash_t *grow_hash(hash_t *ht)
{
	assert(ht != NULL); 

	cell_t *oc = ht->cell; 
	cell_t *end = oc + ht->size; 
	uint32_t oldsize = ht->size; 
	uint32_t size = prime_size(oldsize, &(ht->cur_offset)); 
	ht->size = size; 
	ht->cell = calloc(size, sizeof(cell_t)); 
	cell_t *c; 
	for (c = oc; c < end;c++) {
		void *k = c->key; 
		cell_t *nc = ht->cell + HASH_POSITON(k, ht, ht->size); 
		*nc = *c; 
	}
	free(oc); 
	return ht; 
}
cell_t *find_cell(hash_t *ht, const void *key)
{
	assert(ht != NULL); 
	assert(key != NULL); 
	cell_t *pos = ht->cell + HASH_POSITON(key, ht, ht->size); 
	cell_t *end = ht->cell + ht->size; 
	if (CELL_NOT_EXIST(pos)) return pos; 
	test_function_t test = ht->test_fun; 
	for (; pos < end; pos++) 
		if (test(pos->key, key)) break; 
	return pos; 
}
uint32_t prime_size(uint32_t size, uint8_t *prime_offset) 
{
  static const int primes[] = {
    13, 19, 29, 41, 59, 79, 107, 149, 197, 263, 347, 457, 599, 787, 1031,
    1361, 1777, 2333, 3037, 3967, 5167, 6719, 8737, 11369, 14783,
    19219, 24989, 32491, 42257, 54941, 71429, 92861, 120721, 156941,
    204047, 265271, 344857, 448321, 582821, 757693, 985003, 1280519,
    1664681, 2164111, 2813353, 3657361, 4754591, 6180989, 8035301,
    10445899, 13579681, 17653589, 22949669, 29834603, 38784989,
    50420551, 65546729, 85210757, 110774011, 144006217, 187208107,
    243370577, 316381771, 411296309, 534685237, 695090819, 903618083,
    1174703521, 1527114613, 1837299131, 2147483647
  };
  uint32_t i;
  for (i = *prime_offset; i < countof (primes); i++)
    if (primes[i] >= size)
      {
        *prime_offset = i + 1;
        return primes[i];
      }
  abort ();
}
int string_nocase_cmp(const void *s1, const void *s2)
{
	assert(s1 != NULL && s2 != NULL); 
	char *str1 = (char *)s1; 
	char *str2 = (char*)s2; 
	return strcasecmp(str1, str2); 
}
