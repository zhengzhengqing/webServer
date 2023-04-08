#ifndef REDISPOOL_H_
#define REDISPOOL_H_

#include "sem.h"
#include <list>
#include <iostream>
#include "../log/log.h"
#include <hiredis/hiredis.h>
#include <muduo/base/Mutex.h>
#include "../noncopyable/noncopyable.h"
using namespace std;

typedef list<redisContext *> redisList;

class RedisPool : public noncopyable
{
    public:
        RedisPool();
        ~RedisPool();
    
    public:
        static RedisPool * getInstance()
        {
            static RedisPool redispool;
            return &redispool;
        }

        void init(const string &ip, const int port, int connNum);
        redisContext * getConnection();
        bool releaseConnection(redisContext * red);
        void destroyRedisPool();
        int getFreeConn();
    
    private:
        Sem sem;
        int maxRedisNum;
        int curRedisNum;
        int freeRedisNUm;
        redisList redislist;
        muduo::MutexLock mutex;
};

class RedisRaii
{
    public:
        RedisRaii(redisContext ** redis, RedisPool * redispool)
        {
            m_redis_pool = redispool;
            m_redis = redispool->getConnection();
            *redis = m_redis;
        }
        ~RedisRaii()
        {
            m_redis_pool->releaseConnection(m_redis);
            m_redis = nullptr;
        }
    
    private:
        redisContext * m_redis;
        RedisPool * m_redis_pool;
};

#endif
