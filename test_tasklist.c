/*
 * =====================================================================================
 *
 *       Filename:  test_tasklist.c
 *
 *    Description:  测试task_list
 *
 *        Version:  1.0
 *        Created:  2012年11月22日 15时09分34秒
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
#include "task_list.h"
#define PRINT_LIST do{\
	task_type_t *task = GET_TASK_HEAD_BY_PTR(hd); \
	while (task) {\
		printf("new task:id:%d priority:0x%x url:%s\n",\
				GET_TASK_ID(GET_TASK_BY_PTR(task)), \
				GET_TASK_PRIORITY(GET_TASK_BY_PTR(task)), \
				GET_TASK_URL(GET_TASK_BY_PTR(task))); \
			task = task->next_task; \
	}\
} while(0)
int main(int argc, const char *argv[])
{
	char *url[] = {
		"hello world", 
		"test url", 
		"lshh hello"
	}; 
	//测试task_list_creat
	list_head_t *hd = task_list_creat(); 
	if (hd == NULL) {
		fprintf(stderr, "creat new task list error"); 
		return 1; 
	}
	srand(time(NULL)); 
	int i = 0; 
	char priority = 1; 
	//测试copy_insert_task_by_priority
	for (; i < 11; i++) {
		int j = 16 - i; 
		task_type_t *task = calloc(1, sizeof(task_type_t)); 
		GET_TASK_ID(GET_TASK_BY_PTR(task))=i; 
		GET_TASK_URL(GET_TASK_BY_PTR(task)) = url[i%3]; 
		GET_TASK_PRIORITY(GET_TASK_BY_PTR(task)) =0x10 + rand()%6; 
		printf("new task:id:%d priority:0x%x url:%s\n",
				GET_TASK_ID(GET_TASK_BY_PTR(task)), 
				GET_TASK_PRIORITY(GET_TASK_BY_PTR(task)), 
				 GET_TASK_URL(GET_TASK_BY_PTR(task))); 
		 if (copy_insert_task_by_priority(hd, &(task->task)) == false) {
			 fprintf(stderr, "insert new task error!\n"); 
			 destroy_task_list(hd); 
		 }
	}
#ifdef TEST_TASK_POP
	//测试task_pop
	printf(" <<  <<  <<  << \n"); 
	for (i = 0; i < 10; i++) {
		task_type_t *task = task_pop(hd); 
		if (task == NULL) {
			fprintf(stderr, "task pop error!\n"); 
			destroy_task_list(hd); 
			return 1; 
		}
		 printf("new task:id:%d priority:0x%x url:%s\n",
				 GET_TASK_ID(GET_TASK_BY_PTR(task)), 
				 GET_TASK_PRIORITY(GET_TASK_BY_PTR(task)), 
				 GET_TASK_URL(GET_TASK_BY_PTR(task))); 
	}
#else
	//测试delete_task_by_id
	printf(" <<  <<  < \n"); 
	int j = rand()%16; 
	printf("rand:%d\n", j); 
	task_type_t *task = delete_task_by_id(hd, j); 
	if (task == NULL) {printf("can1t delet\n"); }else
		printf("dele_id: %d\n", GET_TASK_ID(GET_TASK_BY_PTR(task))); 
	PRINT_LIST; 
	free(task); 
	destroy_task_list(hd); 
#endif
	return 0;
}
