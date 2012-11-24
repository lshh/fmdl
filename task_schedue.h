/*
 * =====================================================================================
 *
 *       Filename:  task_schedue.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年11月19日 11时06分34秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef TASK_SCHEDUE_YDKZFUMY
#define TASK_SCHEDUE_YDKZFUMY

#include "task_list.h"
#include <unistd.h>

#define DEFAULT_TASK_QUEUE_FILE		".fmdl_tasks"
typedef enum {
	TO_HIGIH = 0x10,
	TO_SINGLE = 0x20,
	TO_RECU = 0x40, 
	TO_HIBER = 0x80, 
	TO_NOTHING = 0x00
}list_type_t; 

struct task_queue_head 
{
	list_head_t highest_level_tasks; 	/* 强制优先下载队列 */
	list_head_t single_file_download_tasks; /* 单文件下载队列 */
	list_head_t recursion_download_tasks; 	/* 递归文件下载队列 */
	list_head_t hibernation_tasks; 		/* 休眠任务队列 */
}; 

/* 放入一个新任务，此函数会根据任务的优先级将任务添加到合适的下载队列
 * NEW_TASK:	待添加新任务
 * */
bool insert_new_task(task_t *new_task); 
/*
 * 初始化任务结构，读取写入文件中的任务并分配合适的task_id
 * 返回值为下一个可用的task_id
 */
uint16_t init_task_queue(); 
bool move_task_in_queue_by_id(uint16_t task_id, list_type_t flag); 
/* 返回一个TASK_T结构似乎更加合理但是会造成内存泄露或者更多的数据复制 */
task_type_t * get_task_to_dl(); 
bool save_tasks_queue_to_file(); 
bool delete_task_not_start_dl(uint16_t task_id); 
/* 重置所有任务的task_id, 一般在从文件中读入任务后应调用此函数
 * 返回值：下一个可使用的task_id */
uint16_t reset_all_task_id(); 
/*
 * 所有尚未开始下载的任务总和
 */
uint32_t total_tasks(); 
/*
 * 这个函数似乎并无多大作用，在整个程序存活期内任务队列是一直存在的
 * 所以似乎不需要此函数来释放整个任务队列！XXX此函数非常危险，只有当你
 * 确定真的要放弃所有尚未下载任务时方可调用此函数，调用此函数后所有在排队中
 * 的任务都将被放弃，并不可恢复XXX
 */
void destroy_all_tasks_in_queue(); 
#endif /* end of include guard: TASK_SCHEDUE_YDKZFUMY */
