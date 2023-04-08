#ifndef BLOCKQUEUE_H_
#define BLOCKQUEUE_H_

#include <iostream>
#include "muduo/base/Mutex.h"
#include "muduo/base/Condition.h"
#include <assert.h>
using namespace std;

// class noncopyable
// {
// 	protected:
// 		noncopyable() = default;
// 		~noncopyable() = default;
// 	private:
// 		noncopyable(const noncopyable & non) = delete;
// 		noncopyable & operator=(const noncopyable &non) = delete;
// };

template <typename T>
class BlockQueue 
{
	public:
		BlockQueue():arrays(NULL),size(0),maxSizes(1000),
			   tail(-1), front(-1),mutex_(),notFull(mutex_),
			   notEmpty(mutex_)
		{}
		
		~BlockQueue()
		{
			if(arrays != NULL)
			{
				delete [] arrays;
				arrays = NULL;
			}
		}
		
		void setSize(int num)
		{
			if(num > 0)
			{
				maxSizes = num;
				arrays = new T[num];
			}
		}
		
		void push(const T & t);
		T pop();
		
		bool isFull();
		bool isEmpty();
		int sizes();
		
	private:
		T * arrays;
		int size; 
		int maxSizes;
		muduo::MutexLock mutex_; //互斥锁
		muduo::Condition notEmpty;
		muduo::Condition notFull;
		
	public:
		int tail;
		int front;
};

template <typename T>
void BlockQueue<T>::push(const T & t)
{
	muduo::MutexLockGuard lock(mutex_);
 	while(isFull())
	{
		notFull.wait();		
	}
	
	if(size == 0)
	{
		tail++;
		front++;
	}
	else
		tail = (tail + 1) % maxSizes;	
	
	size++;	
	arrays[tail] = t;
	notEmpty.notify();
}

template <typename T>
T BlockQueue<T>::pop()
{
	muduo::MutexLockGuard lock(mutex_);
	while(isEmpty())
	{
		notEmpty.wait();	
	}
	
	assert(!isEmpty());	
	T t = arrays[front];
	if(tail == front)
		front = tail = -1;	
	else
		front = (front + 1 ) % maxSizes;
	size--;
	notFull.notify();
	return t;
}

template <typename T>
bool BlockQueue<T>::isEmpty()
{
	return (size == 0) ? true:false;
}

template <typename T>
int BlockQueue<T>::sizes()
{
	return size;
}

template<typename T>
bool BlockQueue<T>::isFull()
{
	return (size == maxSizes) ? true:false;
}

#endif
