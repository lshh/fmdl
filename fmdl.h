/*
 * =====================================================================================
 *
 *       Filename:  fmdl.h
 *
 *    Description:  支持选项
 *
 *        Version:  1.0
 *        Created:  2012年12月03日 09时25分16秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef FMDL_D4DCG9L5

#define FMDL_D4DCG9L5
#include <stdio.h>
#include <stdint.h>
#include "error_code.h" 	//主要是导入bool
struct fmdl_option
{
	//一般选项
	bool background; 		//进入后台
	char *log_output; 		//日志输出文件
//	bool append_log; 		//是否在已有文件中追加日志
	char *input_file; 		//从input_file中解析URL
	bool as_html; 			//是否将input_file作为HTML文件
	char *merge_url; 		//解析与merge_url相关的URL
	bool quiet;				//是否进入安静模式
	bool only_save; 		//只将链接读入并不下载待下次自动开始
	//下载选项
	uint32_t ntrys; 		//尝试次数, 0默认为无限次
	char *output; 			//将下载数据写入到output
	bool clobber; 			//是否覆盖已下载或同名文件默认不覆盖
	bool print_response; 	//是否打印服务器响应
	bool contin; 			//是否继续上次下载
	int	 timeout; 			//设置超时(所有的均设置为此值)
	int dns_timout; 		//DNS超时
	int	connect_timeout; 	//连接超时
	int read_timeout; 		//读取超时
	int wait_time; 			//在两次下载之间等待时间
	bool radom_wait; 		//随机等待0..wait_time时间
	bool proxy; 			//是否使用代理
	char *bind_addr; 		//绑定到指定的本地IP
	int limit_rate; 		//最大下载速率(默认为0无限制)
	bool only_ipv4; 		//只链接IPv4地址
	bool only_ipv6; 		//只连接IPv6地址
	char *username; 		//用户名
	char *passwd; 			//用户密码
	unsigned long quota; 	//磁盘配额默认已Kb为单位
	bool discard_last_task; //丢弃上次未开始下载的任务
	//HTTP选项
	char *http_user; 	//HTTP用户名
	char *http_passwd; 	//HTTP密码
	bool nocache; 		//是否在服务器上缓存数据
	bool nodnscache; 	//是否关闭DNS缓存
	char *default_page; 	//默认页(默认为index.html)
	char **add_heads; 		//添加的HTTP首部
	int max_redic; 			//最大重定向次数0为无限制为默认值
	char *referurl; 		//REFERER为referurl
	char *proxy_name; 		//代理用户名
	char *proxy_passwd; 	//代理密码
	char *user_agent; 		//AGENT信息默认为fmdl/version
	bool cookie; 			//是否使用cookies
	char *load_cookie;		//载入cookie
	char *save_cookie; 		//保存cookie
	//FTP选项
	bool passive; 				//是否启用passive模式
	char *ftp_user; 			//FTP 用户名
	char *ftp_passwd; 			//FTP 密码
	bool ftp_glob; 				//是否使用通配符
	//递归下载选项
	bool	recursive; 			//是否递归下载
	int 	max_level; 			//最大递归层级
	bool delete_after; 			//是否在下载完成后删除文件
	bool convert_links; 		//下载结束后将HTML和CSS文件指向本地连接
	bool backup; 				//是否在转换链接之前将文件备份
	char *bak; 					//备份后缀
	char **accept_list; 			//可接受的扩展名列表
	char **reject_list; 			//不可接受的扩展名列表
	char **domain_list; 			//可接受的域名列表
	char **domain_reject_list; 		//不可接受的域名列表
	bool follow_ftp; 				//是否追踪HTML中的FTP链接
	char **follow_tags; 			//要追踪的标签
	char **ignore_tags; 			//要忽略的标签
	bool relative; 					//直追踪有关系的链接
	bool robots; 					//是否忽略robots
	char **include_dir; 				//允许目录的列表
	char **exclue_dir; 					//排除的目录列表

}; 
typedef enum {
	OPT_BOOL,
	OPT_LINE, 
	OPT_MUTIL, 
	OPT_VALUE
}opt_type_t; 
struct options_map
{
	opt_type_t type; 		//选项类型
	void (*fun)(void*); 	//对该选项调用的处理函数
	void *arg; 				//函数参数
}; 
struct fmdl_option options; 
void print_help(); 
#endif /* end of include guard: FMDL_D4DCG9L5 */
