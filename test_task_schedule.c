/*
 * =====================================================================================
 *
 *       Filename:  test_task_schedue.c
 *
 *    Description:  测试task_schedue
 *
 *        Version:  1.0
 *        Created:  2012年11月22日 16时08分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "task_schedue.h"
int main(int argc, const char *argv[])
{
	task_t *task = calloc(1, sizeof(task_t)); 
	SET_TASK_ID(*task, 100); 
	SET_TASK_PRIORITY(*task, 0x14); 
	SET_TASK_URL(*task, "hello world"); 
	init_task_queue(); 
	insert_new_task(task); 
	SET_TASK_ID(*task, 200); 
	SET_TASK_PRIORITY(*task, 0x24); 
	SET_TASK_URL(*task, "hello lshh"); 
	insert_new_task(task); 
	SET_TASK_ID(*task, 200); 
	SET_TASK_PRIORITY(*task, 0x15); 
	SET_TASK_URL(*task, "hello lshh"); 
	insert_new_task(task); 
	if (!save_tasks_queue_to_file()) 
		printf("save to file error\n"); 
	return 0;
}
