#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"

bool Log::init(const char *file_name, int close_log, 
		int log_buf_size, int split_lines, int max_queue_size)
{

	closeLog = close_log;
	logBufSize = log_buf_size;
	buf_ = new char[logBufSize];
	memset(buf_, 0, log_buf_size);
	logMaxLine = split_lines;

	time_t t = time(NULL);
	struct tm * sys_tm = localtime(&t);
	struct tm my_tm = *sys_tm;

	//file_name = "./ServerLog"
	const char *p = strrchr(file_name, '/');
	char log_full_name[256] = {0};
	
	//若file_name 没找到 '/'，则直接将时间+文件名作为日志名
	if(p == NULL)
	{
		snprintf(log_full_name,255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900,
				my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
	}
	else
	{
		//将/的位置向后移动一个位置，然后复制到logname中
		//p - file_name + 1是文件所在路径文件夹的长度
	        //dirname相当于./
		strcpy(logName, p + 1);
		strncpy(dirName, file_name, p - file_name + 1);
		
		//后面的参数跟format有关
		snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dirName, 
				my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday
				, logName);
	}
	
	today = my_tm.tm_mday;
	fp = fopen(log_full_name, "a");
	
	if(max_queue_size > 0)
	{
		ptrqueue->setSize(max_queue_size);
		ptrThread->start();
	}
	
	return (fp == NULL) ? false:true;
}

void Log::writeLog(int level, const char *format, ...)
{

	struct timeval now = {0,0};
	gettimeofday(&now, NULL);
	time_t t = now.tv_sec;
	struct tm *sys_tm = localtime(&t);
	struct tm my_tm = *sys_tm;
	char s[16] = {0};
	
	//日志分级
	switch(level)
	{
		case 0:
			strcpy(s, "[debug]:");
			break;
		case 1:
			strcpy(s, "[info]:");
			break;
		case 2:
			strcpy(s, "[warn]:");
			break;
		case 3:
			strcpy(s, "[erro]:");
			break;
		default:
			strcpy(s, "[info]:");
			break;
	}
	count++;
	//源代码此处加锁
	//日志不是今天，写入的日志行数是最大行的倍数
	if(today != my_tm.tm_mday || count % logMaxLine == 0)	
	{
		muduo::MutexLockGuard lock(mutex_);	
		char new_log[256] = {0};
		fflush(fp); // 强行将缓冲区数据写入到磁盘
		fclose(fp); // 关闭文件指针
		char tail[16] = {0};
		
		//格式化日志名中的时间部分
		snprintf(tail,16,"%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon+1,
			  my_tm.tm_mday);
		
		//如果时间不是今天，则创建今天的日志，更新today和count
		if(today !=  my_tm.tm_mday)
		{
			snprintf(new_log, 255, "%s%s%s", dirName, tail, logName);
 		        today = my_tm.tm_mday;
             		count = 0;
		}		
		else
		{
			//超过了最大行，在之前日志名基础上加后缀 
			snprintf(new_log, 255, "%s%s%s.%lld", dirName, tail, 
						logName, count / logMaxLine);
		}
		fp = fopen(new_log, "a");	
	}

	va_list valst;
	//将传入的format参数赋值给valst,便于格式化输出
	va_start(valst, format);
	
	//写入内容格式：时间 + 内容
        //时间格式化，snprintf成功返回写字符的总数，其中不包括结尾的null字符
        int n = snprintf(buf_, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                      my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                      my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
 
	//内容格式化，用于向字符串中打印数据、数据格式用户自定义，返回写入到字符数组str中的字符个数(不包含终止符)
       	int m = vsnprintf(buf_ + n, logBufSize - 1, format, valst);
       	buf_[n + m] = '\n';
       	buf_[n + m + 1] = '\0';
 
       	string logStr(buf_);
        ptrqueue->push(logStr);
     	va_end(valst);
}


void Log::flush()
{	
  	muduo::MutexLockGuard lock(mutex_);
	//强制刷新写入流缓冲区
    fflush(fp);
}
