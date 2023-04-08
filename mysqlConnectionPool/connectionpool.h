#ifndef CONNECTIONPOOL_H_
#define CONNECTIONPOOL_H_

#include <iostream>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include "sem.h"
#include "../log/log.h"
#include "muduo/base/Mutex.h"
#include "../noncopyable/noncopyable.h"

using namespace std;

typedef list<MYSQL *> CONNLIST;

class ConnectionPool : public noncopyable
{
	public:
		ConnectionPool();
		~ConnectionPool();

	public:
		static ConnectionPool * getInstance()
		{
			static ConnectionPool connPool;
			return &connPool;
		}

		void init(const string &Url, const string &User, 
				  const string &Password, const string DataBaseName, 
				  int Port,int MaxConn, int close_log);

		MYSQL * getConnection();
		bool releaseConnection(MYSQL *conn);
		int getFreeConn();
		void destroyConnPool();
	
	private:
		int maxConnNum;     //最大连接数
		int curConnNum;     //已使用连接数
		int freeConnNum;    //空闲连接数
		CONNLIST connList;  //数据库连接列表
		Sem semSignal;      //信号量
		muduo::MutexLock mutex_;

	public:
		string url;
		string port;
		string user;
		string password;
		string databaseName;
		int closeLogFlag; //关闭日志的标志位

};

// RAII技术封装数据库连接池
class connectionRAII
{
	
	public:
		connectionRAII( MYSQL **conn, ConnectionPool *connPool)
		{
			conRAII = connPool->getConnection();
			*conn = conRAII;
			poolRAII = connPool;
		}
		~connectionRAII()
		{
			poolRAII->releaseConnection(conRAII);
			conRAII = nullptr;
		}

	public:
		MYSQL * conRAII;
		ConnectionPool * poolRAII;
};

#endif

