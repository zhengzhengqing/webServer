#ifndef LOG_H_
#define LOG_H_

#include <iostream>
#include <stdio.h>
#include <memory>
#include "BlockQueue.h"
#include "muduo/base/Thread.h"
//#include "muduo/base/Mutex.h"
using namespace std;

typedef BlockQueue<string> blockqueue;
typedef shared_ptr<blockqueue> ptrQueue;


class Log
{
	private:
		Log():count(0),logMaxLine(0),logBufSize(0),closeLog(0),fp(NULL),
		      ptrThread(new muduo::Thread(std::bind(&Log::flushLogThread,this))),
		      ptrqueue(new blockqueue)	
		{}

		~Log()
		{
			
			fclose(fp);
		}
	public:
		static void * flushLogThread(void * args) //线程函数
		{
			Log::getInstance()->asyncWriteLog();
		}
		
		static  Log * getInstance()
		{
			static Log instance;
			return &instance;
		}
		
		//可选参数，日志文件，日志缓冲区大小，最大行数及最长日志条队列
		bool init(const char * fileName, int close_log,
			  int log_buf_size = 8192, int split_lines = 5000000,
			  int max_queue_size = 1000);
		
		void writeLog(int level, const char * format, ...);
		void flush();

	private:
		void asyncWriteLog()
		{
			while(true)
			{
				string single_log = ptrqueue->pop();
				muduo::MutexLockGuard lock(mutex_);
            	int res = fputs(single_log.c_str(), fp);
			}
		}
	
	private:
		char dirName[128] ; //路径名
		char logName[128] ; //log文件名
		int  logMaxLine;    //日志最大行
		int  logBufSize;    //日志缓冲区大小
		long long count;    //日志行数记录
		int today;          //记录当前日期
		FILE *fp;            //文件指针
		int closeLog;
		char *buf_;
		ptrQueue ptrqueue;
		shared_ptr<muduo::Thread> ptrThread;
		muduo::MutexLock mutex_;

}; 

#define LOG_DEBUG(format, ...) {Log::getInstance()->writeLog(0, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#define LOG_INFO(format, ...)  {Log::getInstance()->writeLog(1, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#define LOG_WARN(format, ...)  {Log::getInstance()->writeLog(2, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#define LOG_ERROR(format, ...) {Log::getInstance()->writeLog(3, format, ##__VA_ARGS__); Log::getInstance()->flush();}


#endif

