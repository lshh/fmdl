/*
 * =====================================================================================
 *
 *       Filename:  task_schedue.c
 *
 *    Description:  下载任务管理队列实现
 *
 *        Version:  1.0
 *        Created:  2012年11月19日 14时10分10秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "task_schedue.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#define GET_TASK_LIST_HD(level) task_queue.level
#define GET_HIGHEST_LEVEL_HD   &(GET_TASK_LIST_HD(highest_level_tasks))
#define GET_SINGLE_LEVEL_HD   &(GET_TASK_LIST_HD(single_file_download_tasks))
#define GET_RECURSION_LEVEL_HD   &(GET_TASK_LIST_HD(recursion_download_tasks))
#define GET_HIBERN_LIST_HD &(GET_TASK_LIST_HD(hibernation_tasks))

/* 识别优先级 */
#define IS_HIGH_PRIORITY_BY_PTR(task) ((task)->priority & 0x10)
#define IS_SINGLE_PRIORITY_BY_PTR(task) ((task)->priority & 0x20)
#define IS_RECU_PRIORITY_BY_PTR(task) 	((task)->priority & 0x40)
#define IS_HIBER_PRIORITY_BY_PTR(task)	((task)->priority & 0x80)
//设置外部优先级
#define SET_TO_HIGH_LEVEL(task)		((task)->priority |=  0x10)
#define SET_TO_SIGNLE_LEVEL(task)	((task)->priority |=  0x20)
#define SET_TO_RECU_LEVEL(task)		((task)->priority |= 0x40)
#define SET_TO_HIBER_LEVEL(task)	((task)->priority |= 0x80)
#define CLR_HIBER_LEVEL_BIT(task)	((task)->priority &= 0x7f)
#define CLR_HIGH_LEVEL_BIT(task) 	((task)->priority &= 0xef)

#define DEFAULT_HIGH	"highest"
#define DEFAULT_SINGLE	"single"
#define DEFAULT_RECU	"recursion"
#define DEFAULT_HIBER	"hibernation"
#define MAX_BUFF_LEN	1024
#define CHOMP(str)		do{\
	int len = strlen(str); \
	if ((str)[len - 1] == '\n') (str)[len - 1] = '\0'; \
} while (0)
/* 记录所有任务的全局结构 */
static struct task_queue_head task_queue; 

static task_type_t *find_task_in_queue(uint16_t task_id); 
static bool move_to_hibernation(task_t  *mv_task); 
static bool move_back_to_single(task_t  *mv_task); 
static bool move_back_to_recursion(task_t *mv_task); 
static bool move_to_highest(task_t *mv_task); 
static bool read_task_queue_from_file(); 
static bool write_to_file(const int fd, const task_type_t *hd, const list_type_t flag); 
static task_type_t *alloc_task_type(char *s, size_t len); 
static const char *default_task_file(); 
void init_task_queue()
{
	memset(&task_queue, 0, sizeof(task_queue)); 
	read_task_queue_from_file(); 
}
bool insert_new_task(task_t *new_task)
{
	assert(new_task != NULL); 
	list_head_t *hd; 
	if (IS_HIGH_PRIORITY_BY_PTR(new_task)) {
		hd = GET_HIGHEST_LEVEL_HD; 
		goto EXIT; 
	}
	if (IS_SINGLE_PRIORITY_BY_PTR(new_task)) {
		hd = GET_SINGLE_LEVEL_HD; 
		goto EXIT; 
	}
	if (IS_RECU_PRIORITY_BY_PTR(new_task)) {
		hd = GET_RECURSION_LEVEL_HD; 
		goto EXIT; 
	}
	if (IS_HIBER_PRIORITY_BY_PTR(new_task))
		hd = GET_HIBERN_LIST_HD; 
EXIT:
	return copy_insert_task_by_priority(hd, new_task); 
}
bool move_to_hibernation(task_t *mv_task)
{
	task_type_t *task_to_mv; 
	if (IS_HIGH_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_HIGHEST_LEVEL_HD, GET_TASK_ID(*mv_task)); 
		if (task_to_mv == NULL) return false; 
		CLR_HIGH_LEVEL_BIT(&(GET_TASK_BY_PTR(task_to_mv))); 
		goto EXIT; 
	}
	if (IS_SINGLE_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_SINGLE_LEVEL_HD, GET_TASK_ID(*mv_task)); 
		goto EXIT; 
	}
	if (IS_RECU_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_RECURSION_LEVEL_HD, GET_TASK_ID(*mv_task)); 
	}
EXIT:
	if (task_to_mv == NULL) return false; 
	SET_TO_HIBER_LEVEL(&(GET_TASK_BY_PTR(task_to_mv))); 
	bool rnt =  copy_insert_task_by_priority(GET_HIBERN_LIST_HD, &(GET_TASK_BY_PTR(task_to_mv))); 
	free(task_to_mv); 
	return rnt; 
}
bool move_to_highest(task_t *mv_task)
{
	task_type_t *task_to_mv; 
	if (IS_HIBER_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_HIBERN_LIST_HD, GET_TASK_ID(*mv_task)); 
		if (task_to_mv == NULL) return false; 
		CLR_HIBER_LEVEL_BIT(&(GET_TASK_BY_PTR(task_to_mv))); 
		goto EXIT; 
	}
	if (IS_SINGLE_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_SINGLE_LEVEL_HD, GET_TASK_ID(*mv_task)); 
		goto EXIT; 
	}
	if (IS_RECU_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_RECURSION_LEVEL_HD, GET_TASK_ID(*mv_task)); 
	}
EXIT:
	if (task_to_mv == NULL) return false; 
	SET_TO_HIGH_LEVEL(&(GET_TASK_BY_PTR(task_to_mv))); 
	bool rnt =  copy_insert_task_by_priority(GET_HIBERN_LIST_HD, &(GET_TASK_BY_PTR(task_to_mv))); 
	free(task_to_mv); 
	return rnt; 
}
bool move_back_to_single(task_t *mv_task)
{
	task_type_t *task_to_mv; 
	if (IS_RECU_PRIORITY_BY_PTR(mv_task)) return false; 
	if (IS_HIBER_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_HIBERN_LIST_HD, GET_TASK_ID(*mv_task)); 
		if (task_to_mv == NULL) return false; 
		CLR_HIBER_LEVEL_BIT(&(GET_TASK_BY_PTR(task_to_mv))); 
		goto EXIT; 
	}
	if (IS_HIGH_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_HIGHEST_LEVEL_HD, GET_TASK_ID(*mv_task)); 
		if (task_to_mv == NULL) return false; 
		CLR_HIGH_LEVEL_BIT(&(GET_TASK_BY_PTR(task_to_mv))); 
		goto EXIT; 
	}
EXIT:
	SET_TO_SIGNLE_LEVEL(&(GET_TASK_BY_PTR(task_to_mv))); 
	bool rnt = copy_insert_task_by_priority(GET_SINGLE_LEVEL_HD, &(GET_TASK_BY_PTR(task_to_mv))); 
	free(task_to_mv); 
	return rnt; 
}
bool move_back_to_recursion(task_t *mv_task)
{
	task_type_t *task_to_mv; 
	if (IS_SINGLE_PRIORITY_BY_PTR(mv_task)) return false; 
	if (IS_HIBER_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_HIBERN_LIST_HD, GET_TASK_ID(*mv_task)); 
		if (task_to_mv == NULL) return false; 
		CLR_HIBER_LEVEL_BIT(&(GET_TASK_BY_PTR(task_to_mv))); 
		goto EXIT; 
	}
	if (IS_HIGH_PRIORITY_BY_PTR(mv_task)) {
		task_to_mv = delete_task_by_id(GET_HIGHEST_LEVEL_HD, GET_TASK_ID(*mv_task)); 
		if (task_to_mv == NULL) return false; 
		CLR_HIGH_LEVEL_BIT(&(GET_TASK_BY_PTR(task_to_mv))); 
		goto EXIT; 
	}
EXIT:
	SET_TO_RECU_LEVEL(&(GET_TASK_BY_PTR(task_to_mv))); 
	bool rnt = copy_insert_task_by_priority(GET_SINGLE_LEVEL_HD, &(GET_TASK_BY_PTR(task_to_mv))); 
	free(task_to_mv); 
	return rnt; 
}
bool move_task_in_queue_by_id(uint16_t task_id, list_type_t flag)
{
	task_type_t *to_mv_task = find_task_in_queue(task_id); 
	if (to_mv_task == NULL) return false; 
	switch (flag) {
		case TO_HIGIH:
			if (IS_HIGH_PRIORITY_BY_PTR(&(GET_TASK_BY_PTR(to_mv_task))))
				return true; 
			return move_to_highest(&(GET_TASK_BY_PTR(to_mv_task))); 
		case TO_SINGLE:
			if(IS_SINGLE_PRIORITY_BY_PTR(&(GET_TASK_BY_PTR(to_mv_task))))
				return true; 
			return move_to_hibernation(&(GET_TASK_BY_PTR(to_mv_task))); 
		case TO_RECU:
			if (IS_RECU_PRIORITY_BY_PTR(&(GET_TASK_BY_PTR(to_mv_task))))
				return true;
			return move_back_to_recursion(&(GET_TASK_BY_PTR(to_mv_task))); 
		case TO_HIBER:
			if (IS_HIGH_PRIORITY_BY_PTR(&(GET_TASK_BY_PTR(to_mv_task))))
				return true;
			return move_back_to_single(&(GET_TASK_BY_PTR(to_mv_task))); 
		default:
			return false; 
	}
	abort(); 
}
task_type_t *find_task_in_queue(uint16_t task_id)
{
	list_head_t *hd = GET_HIGHEST_LEVEL_HD; 
	task_type_t *task; 
	task = find_task_by_id(hd, task_id, NULL); 
	if (task == NULL) {
		hd = GET_SINGLE_LEVEL_HD; 
		task = find_task_by_id(hd, task_id, NULL); 
		if (task == NULL) {
			hd = GET_RECURSION_LEVEL_HD; 
			task = find_task_by_id(hd, task_id, NULL); 
			if (task == NULL) {
				hd = GET_HIBERN_LIST_HD; 
				task = find_task_by_id(hd, task_id, NULL); 
			}
		}
	}
	return task; 
}

task_type_t*get_task_to_dl()
{
	if (GET_TASK_HEAD_BY_PTR(GET_HIGHEST_LEVEL_HD) == NULL) {
		if (GET_TASK_HEAD_BY_PTR(GET_SINGLE_LEVEL_HD) == NULL){
			if (GET_TASK_HEAD_BY_PTR(GET_RECURSION_LEVEL_HD) == NULL) 
				/* 休眠队列如果用户不唤醒那么不会进行下载 */
				return NULL; 
			else 
				return task_pop(GET_RECURSION_LEVEL_HD); 
		} else
			return task_pop(GET_SINGLE_LEVEL_HD); 
	} else 
		return task_pop(GET_HIGHEST_LEVEL_HD); 
}
bool save_tasks_queue_to_file()
{
	const char *local_file = getenv("FMDL_TASK_FILE"); 
	task_type_t *hd; 
	if (local_file == NULL) local_file = default_task_file(); 
	int fd = open(local_file, O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR); 
	if (fd < 0) return false; 
	if (GET_TASK_HEAD_BY_PTR(GET_HIGHEST_LEVEL_HD)) {
		/* 最高优先级 */
		hd = GET_TASK_HEAD_BY_PTR(GET_HIGHEST_LEVEL_HD); 
		write_to_file(fd, hd, TO_HIGIH); 
	}
	if (GET_TASK_HEAD_BY_PTR(GET_SINGLE_LEVEL_HD)) {
		/* 单文件 */
		hd = GET_TASK_HEAD_BY_PTR(GET_SINGLE_LEVEL_HD); 
		write_to_file(fd, hd, TO_SINGLE); 
	}
	if (GET_TASK_HEAD_BY_PTR(GET_RECURSION_LEVEL_HD)) {
		/* 镜像 */
		hd = GET_TASK_HEAD_BY_PTR(GET_RECURSION_LEVEL_HD); 
		write_to_file(fd, hd, TO_RECU); 
	}
	if (GET_TASK_HEAD_BY_PTR(GET_HIBERN_LIST_HD)) {
		/* 休眠 */
		hd = GET_TASK_HEAD_BY_PTR(GET_HIBERN_LIST_HD); 
		write_to_file(fd, hd, TO_HIBER); 
	}
	close(fd); 
	return true; 
}
bool write_to_file(const int fd, const task_type_t *hd, const list_type_t flag)
{
	char *buf = malloc(MAX_BUFF_LEN*sizeof(char) + 1); 
	char *nxt = buf; 
	size_t buflen = MAX_BUFF_LEN; 
	size_t cur_len = 0; 
	nxt[buflen] = '\0'; 
	if (fd < 0 || hd == NULL)
		return false; 
	switch (flag) {
		case TO_HIGIH:
			sprintf(buf, "[%s]\n", DEFAULT_HIGH); 
			cur_len = strlen(DEFAULT_HIGH) + 3; 
			nxt += cur_len; 
			break; 
		case TO_SINGLE:
			sprintf(buf, "[%s]\n", DEFAULT_SINGLE); 
			cur_len = strlen(DEFAULT_SINGLE) + 3;
			nxt += cur_len; 
			break; 
		case TO_RECU:
			sprintf(buf, "[%s]\n", DEFAULT_RECU); 
			cur_len = strlen(DEFAULT_RECU) + 3; 
			nxt += cur_len; 
			break; 
		case TO_HIBER:
			sprintf(buf, "[%s]\n", DEFAULT_HIBER); 
			cur_len = strlen(DEFAULT_HIBER) + 3; 
			nxt += cur_len; 
			break; 
		default:
			return false; 
	}
	const task_type_t *task = hd; 
	size_t len; 
	do {
		int bit = 1; 
		const char *url = GET_TASK_URL(GET_TASK_BY_PTR(task)); 
		const uint8_t priority = GET_TASK_PRIORITY(GET_TASK_BY_PTR(task)); 
		int tmp = priority; 
		//\t%d:url\n
		while ((tmp = tmp/10)) bit++; 
		len = 1		//\t
			+ bit	//优先级占用的字节数
		   	+ 1 	//:
			+ 1 	//\n
			+ strlen(url); //url
		if ((nxt + len) > (buf + buflen)) {
			buf = realloc(buf, buflen << 1); 
			nxt = buf + cur_len; 
		}
		sprintf(nxt, "\t%d:%s\n", priority, url); 
		cur_len += len; 
		nxt += len; 
	} while ((task = task->next_task));
	buf[cur_len] = 0; 
	/* 将所有信息写入文件 */
	{
		ssize_t cnt = 0; 
		ssize_t wnt = 0; 
		char *p = buf; 
AGAIN:
		while ((wnt = write(fd, p + cnt, cur_len - cnt))) {
			if ((wnt + cnt) != cur_len) {
				cnt += wnt; 
				continue; 
			}
			break; 
		}
		if (wnt < 0)
			if(errno == EINTR) /* 任务被信号中断 */
				goto AGAIN; 
			else
			   	return false; 
		free(buf); 
	}
	return true; 
}
bool read_task_queue_from_file()
{
	const char *local_file = getenv("FMDL_TASK_FILE"); 
	if (local_file == NULL) local_file = default_task_file(); 

	FILE *fp = fopen(local_file, "r"); 
	list_type_t flag = TO_NOTHING; 
	task_type_t **task_ptr; 
	if (fp == NULL) return false; 
	char buf[MAX_BUFF_LEN] = {0}; 
	while (fgets(buf, MAX_BUFF_LEN - 1, fp)) {
		CHOMP(buf); 
		if (buf[0] == '[') {
			char *ptr = buf + 1; 
			ptr[strlen(ptr) - 1] = '\0'; 
			if (strcmp(ptr, DEFAULT_HIGH) == 0) {
				flag = TO_HIGIH; 
				task_ptr = &(GET_TASK_HEAD_BY_PTR(GET_HIGHEST_LEVEL_HD)); 
				memset(buf, 0, strlen(buf)); 
				continue; 
			}
			if (strcmp(ptr, DEFAULT_SINGLE) == 0) {
				flag = TO_SINGLE; 
				task_ptr = &(GET_TASK_HEAD_BY_PTR(GET_SINGLE_LEVEL_HD)); 
				memset(buf, 0, strlen(buf)); 
				continue; 
			}
			if (strcmp(ptr, DEFAULT_RECU) == 0) {
				flag = TO_RECU; 
				task_ptr = &(GET_TASK_HEAD_BY_PTR(GET_RECURSION_LEVEL_HD)); 
				memset(buf, 0, strlen(buf)); 
				continue; 
			}
			if (strcmp(ptr, DEFAULT_HIBER) == 0) {
				flag = TO_HIBER; 
				task_ptr = &(GET_TASK_HEAD_BY_PTR(GET_HIBERN_LIST_HD)); 
				memset(buf, 0, strlen(buf)); 
				continue; 
			}
		}

		if (buf == NULL) goto EXIT; 

		if (flag != TO_NOTHING){
			*task_ptr = alloc_task_type(buf, strlen(buf)); 
			task_ptr = &((*task_ptr)->next_task); 
		}
	}
EXIT:
	fclose(fp); 
	return true; 
}
task_type_t *alloc_task_type(char *s, size_t len)
{
	assert(s != NULL); 
	assert(len != 0); 
	char *ptr; 
	task_type_t *task = calloc(1, sizeof(task_type_t)); 
	if (task == NULL) return task; 
	while (isspace(*ptr)) ptr++; 	/* 跳过\t */
	/* 获取优先级 */
	while (isdigit(*ptr))
	   	GET_TASK_PRIORITY(GET_TASK_BY_PTR(task)) +=
		   	10*(GET_TASK_PRIORITY(GET_TASK_BY_PTR(task))) + *ptr++ - '0'; 
	ptr++; /* 跳过':' */
	/* 计算为需要为url分配的空间 */
	len = len - (ptr - s); 
	GET_TASK_URL(GET_TASK_BY_PTR(task)) = calloc(1, len); 
	if (GET_TASK_URL(GET_TASK_BY_PTR(task)) == NULL) {free(task); return NULL; }
	strcpy(GET_TASK_URL(GET_TASK_BY_PTR(task)), ptr); 
	return task; 
}
bool delete_task_not_start_dl(uint16_t task_id)
{
	task_type_t *task = delete_task_by_id(GET_HIGHEST_LEVEL_HD, task_id); 
	if (task != NULL) {free(task); return true; }
	task = delete_task_by_id(GET_SINGLE_LEVEL_HD, task_id); 
	if (task != NULL) {free(task); return true; }
	task = delete_task_by_id(GET_RECURSION_LEVEL_HD, task_id); 
	if (task != NULL) {free(task); return true; }
 	task = delete_task_by_id(GET_HIBERN_LIST_HD, task_id); 
	if (task != NULL) {free(task); return true; }
	return false; 
}
uint16_t reset_all_task_id()
{
#define SET_TASK_ID_OF_HEAD(hd) {\
	task_type_t *task = GET_TASK_HEAD_BY_PTR((hd)); \
	do {\
		GET_TASK_ID(GET_TASK_BY_PTR(task)) = task_id++; \
	} while ((task = task->next_task));\
}
	/* 此函数保证只会设置一次task_id */
	static bool seted = false; 

	uint16_t task_id = 1; 
	if (seted == false){
		SET_TASK_ID_OF_HEAD(GET_HIGHEST_LEVEL_HD); 
		SET_TASK_ID_OF_HEAD(GET_SINGLE_LEVEL_HD); 
		SET_TASK_ID_OF_HEAD(GET_RECURSION_LEVEL_HD); 
		SET_TASK_ID_OF_HEAD(GET_HIBERN_LIST_HD); 
		seted = true; 
	}
	return task_id; 
#undef SET_TASK_ID_OF_HEAD
}
const char *default_task_file()
{
	static char conf_file[128] = {0}; 
	char *home = getenv("HOME"); 
	if (home == NULL) return home; 
	snprintf(conf_file, 128, "%s/%s", home, DEFAULT_TASK_QUEUE_FILE); 
	return conf_file; 
}
