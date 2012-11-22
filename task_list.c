/*
 * =====================================================================================
 *
 *       Filename:  task_list.c
 *
 *    Description:  任务队列实现
 *
 *        Version:  1.0
 *        Created:  2012年11月17日 18时59分17秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "task_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define SET_TASKS_LIST_HEAD_BY_PTR(ptr) head->task_head = (ptr)
task_type_t *find_task_by_id(const list_head_t *head, const uint16_t task_id, task_type_t **previous_task)
{
	assert(head != NULL); 
	if (head->task_head == NULL) return NULL; 

	task_type_t *pre; 
	task_type_t *hd = head->task_head; 
	pre = hd; 
	if (GET_TASK_ID(GET_TASK_BY_PTR(hd)) == task_id) {
		if (previous_task != NULL) *previous_task = NULL; 
		return hd; 
	}
	while ((hd = hd->next_task)) {
		if (GET_TASK_ID(GET_TASK_BY_PTR(hd)) == task_id) {
			if (previous_task != NULL) *previous_task = pre; 
			return hd; 
		}
		pre = hd; 
	}
	*previous_task = NULL;
	return NULL; 
}
list_head_t *destroy_task_list(list_head_t *head)
{
	if (head == NULL) return NULL; 
	if (head->task_head == NULL) goto EXIT; 
	DESTORY_TASK_LIST(head->task_head); 
EXIT:
	free(head); 
	return NULL;
}
list_head_t *task_list_creat() 
{
	list_head_t *head; 
	head = calloc(1, sizeof(list_head_t)); 
	return head; 
}
bool copy_insert_task_by_priority(list_head_t *head, const task_t *task)
{
#define INIT_NEW_TASK_TYPE(src) do {\
	new = calloc(1, sizeof(task_type_t)); \
	if (new == NULL) return false; \
	COPY_TASK(GET_TASK_BY_PTR(new), *(src)); \
} while (0) 
	assert(task != NULL); 
	assert(head != NULL); 
	task_type_t *task_hd = NULL; 
	task_type_t *pre_task = NULL; 
	task_type_t *new; 
	if (head->task_head == NULL) {
		INIT_NEW_TASK_TYPE(task); 
		head->task_head = new; 
		goto EXIT; 
	}
	if (GET_TASK_PRIORITY(GET_TASK_BY_PTR(head->task_head)) > GET_TASK_PRIORITY(*task)) {
		INIT_NEW_TASK_TYPE(task); 
		new->next_task = head->task_head; 
		head->task_head = new; 
		goto EXIT; 
	}
	task_hd = head->task_head; 
	pre_task = head->task_head; 
	while ((task_hd = task_hd->next_task) && 
			GET_TASK_PRIORITY(GET_TASK_BY_PTR(task_hd)) <= GET_TASK_PRIORITY(*task))
		pre_task = task_hd; 
	/* 新任务处于pre_task之后task_hd之前 */
	INIT_NEW_TASK_TYPE(task); 
	pre_task->next_task = new; 
	new->next_task = task_hd; 
EXIT:
	head->task_number++; 
	return true; 
#undef INIT_NEW_TASK_TYPE
}
task_type_t *task_pop(list_head_t *head)
{
	assert(head != NULL); 
	if (head->task_head == NULL)
		return NULL; 
	task_type_t *rnt_task = head->task_head; 
	SET_TASKS_LIST_HEAD_BY_PTR(rnt_task->next_task); 
	head->task_number--; 
	rnt_task->next_task = NULL; 
	return rnt_task; 
}
task_type_t *delete_task_by_id(list_head_t *head, const uint16_t task_id)
{
	assert(head != NULL); 
	task_type_t *pre_task = NULL; 
	task_type_t *del_task = find_task_by_id(head, task_id, &pre_task); 
	if (del_task == NULL) return del_task; 
	/* 待删除的任务在队列头部 */
	if (pre_task == NULL) {
		head->task_head = del_task->next_task; 
		goto EXIT;
	}
	pre_task->next_task = del_task->next_task; 
EXIT:
	head->task_number--; 
	del_task->next_task = NULL; 
	return del_task; 
}
bool find_all_tasks_of_priority(const list_head_t *head, const uint8_t priority, 
		task_type_t **head_task, task_type_t **tail_task)
{
	task_type_t *hd = head->task_head; 
	while (hd && (GET_TASK_PRIORITY(GET_TASK_BY_PTR(hd)) != priority))
	   	hd = hd->next_task; 
	*head_task = hd; 
	*tail_task = hd; 
	/* 不存在此优先级的任务 */
	if (hd == NULL) return false; 
	hd = *tail_task; 
	while (hd && GET_TASK_PRIORITY(GET_TASK_BY_PTR(hd)) == priority) 
		hd = hd->next_task; 
	*tail_task = hd; 
	return true; 
}
