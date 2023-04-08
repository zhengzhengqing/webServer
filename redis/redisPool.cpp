#include "redisPool.h"

RedisPool::RedisPool():maxRedisNum(0),
                      curRedisNum(0),
                      freeRedisNUm(0)
{

}

RedisPool::~RedisPool()
{
    destroyRedisPool();
}

void RedisPool::init(const string &ip, const int port, int connNum)
{
    for(int i = 0; i < connNum; i++)
    {
        redisContext * redis = nullptr;
        redis = redisConnect(ip.c_str(),port);
        if(redis == NULL || redis->err)
        {
            if(redis)
            {
                LOG_ERROR("Error: %s", redis->errstr);
            }
            else
            {
                LOG_ERROR("can not allocate redis context");
            }
            return ;
        }
        redislist.push_back(redis);
        freeRedisNUm++;
    }
    maxRedisNum = freeRedisNUm;
    sem.init(maxRedisNum);
}

redisContext * RedisPool::getConnection()
{
    redisContext * redis = nullptr;
    if(redislist.size() == 0)
        return NULL;
    
    sem.wait();
    {
        muduo::MutexLockGuard lock(mutex);
        redis = redislist.front();
        redislist.pop_front();
        freeRedisNUm--;
        curRedisNum++;
    }
    return redis;
}

bool RedisPool::releaseConnection(redisContext * redis)
{
    if(redis == NULL)
        return false;

    {
        muduo::MutexLockGuard lock(mutex);
        redislist.push_back(redis);
        freeRedisNUm++;
        curRedisNum--;
    }

    sem.post();
    return true;
}

int RedisPool::getFreeConn()
{
    return freeRedisNUm;
}

void RedisPool::destroyRedisPool()
{
    list<redisContext *>::iterator it;
    for(it = redislist.begin(); it != redislist.end(); it++)
    {
        redisContext * re = *it;
        redisFree(re);
    }
    curRedisNum = 0;
    freeRedisNUm = 0;
}