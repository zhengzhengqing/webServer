#include "connectionpool.h"

ConnectionPool::ConnectionPool():curConnNum(0),freeConnNum(0)
{}

ConnectionPool::~ConnectionPool()
{
    destroyConnPool();
}

void ConnectionPool::init(const string &Url, const string &User, 
				          const string &Password, const string DataBaseName, 
				          int Port,int MaxConn, int close_log)
{
    url  = Url;
    port = Port;
    user = User;
    password = Password;
    databaseName = DataBaseName;
    closeLogFlag = close_log;

    for(int i = 0; i < MaxConn; i++)
    {
        MYSQL *conn = NULL;
        conn = mysql_init(conn);
        if(conn == NULL)
        {
            LOG_ERROR("MYSQL INIT FAILED");
            cout <<"NULL NULL 1" <<endl;
            return ;
        }

        conn = mysql_real_connect(conn, url.c_str(), User.c_str(), 
                                Password.c_str(), DataBaseName.c_str(), Port, NULL, 0);

        if(conn == NULL)
        {
            cout <<"NULL NULL 2"<<endl;
            cout <<"mysql error = " <<mysql_error(conn) <<endl;
            LOG_ERROR("Mysql connect failed");
            return ;
        }
        
        connList.push_back(conn);
        freeConnNum++;
    }
    maxConnNum = freeConnNum;
    semSignal.init(maxConnNum);
}

MYSQL * ConnectionPool::getConnection()
{
    MYSQL *conn = NULL;
    if(connList.size() == 0)
        return NULL;
    
    // 减小临界区域
    semSignal.wait();
    {
        muduo::MutexLockGuard lock(mutex_);
        conn = connList.front();
        connList.pop_front();
        freeConnNum--;
        curConnNum++;
    }
    return conn;
}

bool ConnectionPool::releaseConnection(MYSQL * conn)
{
    if(conn == NULL)
        return false;
    
    // 减小临界区域
    {
        muduo::MutexLockGuard lock(mutex_);
        connList.push_back(conn);
        freeConnNum++;
        curConnNum--;
    }

    semSignal.post();
    return true;
}

int ConnectionPool::getFreeConn()
{
    return freeConnNum;
}

void ConnectionPool::destroyConnPool()
{
    list<MYSQL *> ::iterator it;
    for(it = connList.begin(); it != connList.end(); it++)
    {
        MYSQL *conn = *it;
        mysql_close(conn);
    }

    curConnNum = 0;
    freeConnNum = 0;
}