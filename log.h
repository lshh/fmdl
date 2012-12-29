/*
 * =====================================================================================
 *
 *       Filename:  log.h
 *
 *    Description:  日志模块
 *
 *        Version:  1.0
 *        Created:  2012年11月23日 20时32分29秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef LOG_OYFB6UPY

#define LOG_OYFB6UPY

#include <stdio.h>
#include "error_code.h"
typedef enum{false, true} bool; 
typedef enum{
	LOG_ERROR, 		/* 错误信息 */
	LOG_WARNING, 	/* 警告信息 */
	LOG_VERBOS		/* 详细信息 */
}log_option; 
/*
 * 初始化日志系统，log_callback函数返回一个文件描述符
 * 此描述符将被作为日志的输出地。当日志输出失败时，会
 * 尝试重新调用此函数来打开此接口，但是只会尝试有限次, 
 * NO_CACHE为TRUE则日志输出将不使用缓冲，此标志一般设
 * 置为FALSE ，只有当日志输出到网络时才应设置为TRUE
 * note:此函数只应被调用一次，多次调用无效。
 */
void log_init(int (*log_output_callback)(void*),void *arg, bool no_cache); 
/* 日志打印接口 */
void log_printf(log_option, char *fmt, ...); 
void log_puts(log_option, char*); 
/* 冲洗日志缓冲区 */
void log_fflush(); 
/* 禁止日志输出 */
void inhibit_log(); 
/* 如果禁止了日志输出，此函数可以打开日志输出接口 */
void nohibit_log(); 
/* 重定向日志输出, 非信号方式 */
void redirct_log_ouput(int (*log_output_callback)(void*),void *arg, bool no_cache); 
/* 重定向日志输出，信号方式 
 * 此函数会将日志输出重定向到默认的文件中
 */
void redict_log_output_by_signal(int); 
/* 关闭日志输出接口, 调用此函数意味着系统会丢弃所有日
 * 志包括在缓冲区中的尚未输出的日志 */
bool log_close(); 
/* DEBUG 接口 */
void log_debug(log_option op, char *fmt, ...); 
#endif /* end of include guard: LOG_OYFB6UPY */
