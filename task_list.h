/*
 * =====================================================================================
 *
 *       Filename:  task_list.h
 *
 *    Description:  下载队列数据结构设计, 适合单线程环境
 *
 *        Version:  1.0
 *        Created:  2012年11月17日 17时17分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 , lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef TASK_LIST_9DYI5RXI

#define TASK_LIST_9DYI5RXI
#include <stdint.h>
#include "error_code.h"
/*
 * 每个任务的特有结构，客户端以此结构形式传递新任务
 * 到服务器端 
 * */
typedef struct task
{
	char *task_url; 		/* 下载地址 */
	uint8_t  priority; 		/* 优先级，共8位其中|1|2|3|4|5|6|7|8|前四位为外部优先级，后四位为内部优先级 */
	uint16_t task_id;		/* 任务ID号 唯一标识此任务 */
}task_t; 
#define SET_TASK_ID(task_ptr, id) (task_ptr).task_id = (id)
#define SET_TASK_PRIORITY(task_ptr, task_priority) (task_ptr).priority = (task_priority)
#define SET_TASK_URL(task_ptr, url) (task_ptr).task_url = strdup((url)) 
#define GET_TASK_ID(task_ptr) (task_ptr).task_id
#define GET_TASK_PRIORITY(task_ptr) (task_ptr).priority
#define GET_TASK_URL(task_ptr) (task_ptr).task_url 
#define COPY_TASK(des_task, src_task) do {\
des_task = src_task; \
SET_TASK_URL((des_task), GET_TASK_URL(src_task)); \
} while (0)

/* 任务按照优先级组成链表 */
typedef struct task_struct
{
	task_t task; 
	struct task_struct *next_task; 
}task_type_t; 
#define GET_TASK_BY_PTR(task_elem) (task_elem)->task 
#define GET_TASK(task_elem) (task_elem).task
typedef struct 
{
	task_type_t *task_head; 
	uint32_t task_number; 
}list_head_t; 

#define DESTORY_TASK_LIST(head) do {\
	task_type_t *del = (head); \
	(head) = del->next_task; \
	free(GET_TASK_URL(del->task)); \
	free(del); \
} while ((head))
#define GET_TASK_HEAD_BY_PTR(hd_ptr) (hd_ptr)->task_head 
#define GET_TASK_HEAD(hd) (hd).task_head
/* 创建一个新的任务队列 */
list_head_t *task_list_creat(); 
/* 
 * 按task_id号查找制定的任务 
 * HEAD:	 队列
 * TASK_ID:	 任务ID号
 * PREVIOUS_TASK:	指向此任务的前一个任务
 * */
task_type_t *find_task_by_id(const list_head_t *head, const uint16_t task_id, task_type_t **previous_task); 
/*
 * 查找所有优先级为priority的任务.
 * HEAD:	队列
 * PRIORITY: 优先级
 * HEAD_TASK:	指向返回链表的第一个任务
 * TAIL_TASK:	指向返回链表的最后一个任务的后一个任务
 *
 * 警告：由head_task和tail_task所确定的链表是不应被修改的, 
 * 也就是它们指向的队列应该是只读的.它们指向的是任务队列中
 * 的数据.如果被修改可能导致严重错误.
 */
bool find_all_tasks_of_priority(const list_head_t *head, const uint8_t priority,
	   	task_type_t **head_task, task_type_t **tail_task); 
/*
 * 在任务队列中添加新任务，并将按照任务的优先级插入到任务队
 * 列的合适位置.插入后队列中的任务将是TASK任务的副本.因此
 * 释放TASK指针将不会影响到任务队列中的任务.
 * HEAD:	队列
 * TASK:	待插入的新任务
 */
bool copy_insert_task_by_priority(list_head_t *head, const task_t *task); 
/*
 * 删除任务队列中指定ID的任务
 * HEAD:	队列
 * TASK_ID:	任务ID
 */
task_type_t *delete_task_by_id(list_head_t *head, const uint16_t task_id); 
/* 从任务队列头中取出一个新任务
 * HEAD:	任务队列
 * */
task_type_t *task_pop(list_head_t *head); 
/*
 * 销毁任务队列
 * HEAD:	待销毁的任务队列
 */
list_head_t * destroy_task_list(list_head_t *head); 



#endif /* end of include guard: TASK_LIST_9DYI5RXI */
