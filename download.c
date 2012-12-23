/*
 * =====================================================================================
 *
 *       Filename:  download.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年12月19日 22时14分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  李双航 (), lshh0718@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "download.h"
#include "ftp.h"
#include "http.h"
#include "hash.h"
#include "fmdl.h"
#include "net.h"
#include "error_code.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

/*
 * 记录相关信息的HASH表
 */
hash_t *dl_file_url_map; 		//已下载的文件和URL的映射
hash_t *dl_url_file_map; 		//已下载的URL和文件的映射
//单文件下载
dl_t *start_download(url_t *ut, char *orig_url)
