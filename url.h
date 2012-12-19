/*
 * =====================================================================================
 *
 *       Filename:  url.h
 *
 *    Description:  处理URL模块
 *
 *        Version:  1.0
 *        Created:  2012年12月05日 19时27分04秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef URL_1SE861HM

#define URL_1SE861HM
#include <stdint.h>

#define countof(array) (sizeof(array)/sizeof((array)[0]))
/* 支持的协议 */
typedef enum {
	SCHEME_HTTP, 		//HTTP协议
	SCHEME_FTP, 		//FTP协议
	SCHEME_UNSUPPORT, 	//协议不支持
	SCHEME_NOSCHEME		//URL无协议字段(一般可能为简写形式)
} scheme_t;
typedef enum {
	scm_has_param = 0, 
	scm_has_query = 2, 
	scm_has_frag = 4, 
	scm_disable = 8
} scm_flag_t;
typedef struct _supported_t {
	char *prefix; 	
	scheme_t scheme; 	
	uint16_t port; 
	scm_flag_t flag; 		
} supported_t;
typedef struct _url_t {
	char *encode_url; 			//编码后的URL
	scheme_t scheme; 			//协议
	uint16_t port; 				//端口号
	char *host; 				//主机
	char *path; 				//路径
	char *dir; 					//目录
	char *file; 				//文件
	char *param; 				//参数
	char *query; 				//查询
	char *frag; 				//信息片段
	char *name; 				//用户名
	char *passwd; 				//用户密码
} url_t;
/*
 * 将URL分割为一个url_t结构
 * *err返回错误码
 */
url_t *url_parsed(char *url, int *err); 
/*
 * 获取URL的协议
 */
scheme_t url_scheme(char *url); 
/*
 * 处理简写的URL返回一个完整格式的URL, 不是简写的URL
 * 则返回NULL
 */
char *short_hand_url(char *url); 
/*
 * 编码URL
 */
char *encode_url(char *orig_url); 
char *decode_url(const char *encode_url); 
#endif /* end of include guard: URL_1SE861HM */
