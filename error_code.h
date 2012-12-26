/*
 * =====================================================================================
 *
 *       Filename:  error_code.h
 *
 *    Description:  错误码
 *
 *        Version:  1.0
 *        Created:  2012年11月24日 21时20分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef ERROR_CODE_W7YVQJBN

#define ERROR_CODE_W7YVQJBN
#include "log.h"
#define NO_ERROR	0x0000
/*------------ 错误码 --------------*/
#define ERR_LOG		0x0010		/* 日志错误 */
#define ERR_ALLOC	0x0011		/* 内存错误 */
#define ERR_PIPE	0x0012		/* 收到PIPE信号 */
#define ERR_OPTIONS	0x0013		/* 选项指定错误 */
//URL错误码
#define ERR_UNSUPPORT_SCHEME 0x0020			/* 不支持的协议 */
#define ERR_SCHEME_SET_FAIL	0x0021			/* 添加协议头部失败 */
#define ERR_BADURL			0x0022			/* 错误的URL */
/* ----------调试信息打印接口---------- */
#define DEBUGINFO(arg) 	log_debug(LOG_ERROR, arg)
#define ERR_TIMEOUT		0x0031
#define ERR_NET			0x0032
#define FTP_ERR			0x0033
#endif /* end of include guard: ERROR_CODE_W7YVQJBN */
