#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <queue>
#include <map>
#include <assert.h>
#include <pthread.h>
#include <iostream>
#include <memory>
#include "muduo/base/Mutex.h"
#include "muduo/base/Condition.h"
#include "../httpcontext/httpconn.h"
using namespace std;


template <typename T>
class ThreadPool
{
    public:
        ThreadPool();
        ~ThreadPool();

    public:
        T *  pop();
        void stop();
        void append(T *);
        bool isEmpty();
        bool isFull();
        void start();
        void createThread();
        void init(int threadNums);
        static void *run(void * arg);

    private:
        int threadNum;
        int queueSize;
        queue<T *> taskQueue; //任务队列
        muduo::MutexLock mutex;
        muduo::Condition notFull;
        muduo::Condition notEmpty;
        std::vector<unique_ptr<pthread_t>> threads; // 线程队列
};

template <typename T>
ThreadPool<T>::ThreadPool():threadNum(4),queueSize(10000),
                                        notFull(mutex), notEmpty(mutex)
{}

template <typename T>
ThreadPool<T>::~ThreadPool()
{}

template<typename T>
void ThreadPool<T>::init(int threadNums)
{
    threadNum = threadNums;
}

template <typename T>
void ThreadPool<T>::createThread()
{
    for(int i = 0; i < threadNum; i++)
    {
        threads.emplace_back(new pthread_t);
        int res = pthread_create(&(*threads[i]),NULL, run , this);
        if(res == -1)
            throw::exception();
    }
}

template <typename T>
void ThreadPool<T>::append(T * t)
{
    muduo::MutexLockGuard lock(mutex);
    while(isFull())
    {
        notFull.wait(); // 等待不为满
    }
    assert(!isFull());
    taskQueue.push(t);
    notEmpty.notify(); // 通知第一个等待的线程
}

template <typename T>
T * ThreadPool<T>::pop()
{
    muduo::MutexLockGuard lock(mutex);
    while(isEmpty())
    {
        notEmpty.wait(); // 等待不为空
    }
    assert(!isEmpty());
    T * t = taskQueue.front();
    taskQueue.pop();
    notFull.notify();
    return t;
}

template <typename T>
void ThreadPool<T>::stop()
{

}

template<typename T>
void ThreadPool<T>::start()
{
    threads.reserve(threadNum);
    createThread(); // 创建线程
}

template<typename T>
void * ThreadPool<T>::run(void *arg)
{
    ThreadPool * tp = (ThreadPool*)arg;
    while(true)
    {
        
        // 从任务队列中取出任务
        T * task = tp->pop();
        if(0 == task->actionModel_) // 有数据可读
        {
            task->readData();
        }
        else //有数据要写
        {
            task->writeData();
        }
    }
    
    return tp;
}

template <typename T>
bool ThreadPool<T>::isFull()
{
    return queueSize == taskQueue.size();
}

template<typename T>
bool ThreadPool<T>::isEmpty()
{
    return taskQueue.size() == 0;
}

#endif