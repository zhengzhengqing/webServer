#ifndef SERVER_H_
#define SERVER_H_

#include <memory>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../log/log.h"
#include "../redis/redisPool.h"
#include "../timeWheel/timer.h"
#include "../threadpool/threadpool.h"
#include "../httpcontext/httpconn.h"
#include "../noncopyable/noncopyable.h"
#include "../mysqlConnectionPool/connectionpool.h"

using namespace std;

const int MAX_FD            = 65535;  // 最大文件描述符
const int MAX_EVENT_NUMBER  = 10000;  // 最大事件数
const int TIMESLOT          = 5;      // 最小超时单位
const int READBUF_SIZE      = 2048;
const int WRITEBUF_SIZE     = 1024;


/*
    LT如果程序使用Level-Triggered 水平出发方式，那么什么时候关注EPOLLOUT事件？ 会不会造成busy-loop?
        流程
        1：
            listenfd EPOLLIN 事件到来
            connfd = accept(...)
            关注connfd的EPOLLIN事件
        2：
            处理有事件发生的套接字
            connfd EPOLLIN事件发生
            read(connfd,...)
            ret = write(connfd, buf, 10000)
            if(ret < 10000)
            {
                将未发送完成的数据添加到应用层发送缓冲区OutBuffer
                关注connfd的EPOLLOUT事件
            }
            connfd EPOLLOUT事件到来
            取出应用层发送缓冲区的数据发送 write(connfd, ...)
            如果应用层发送缓冲区的数据发送完毕，取消关注EPOLLOUT事件

    ET模式下工作过程
        流程
        1：
            listenfd EPOLLIN 事件到来
            connfd = accept(...)
            关注connfd的EPOLLIN事件与EPOLLOUT事件
        2：
            处理有事件发生的套接字
            connfd EPOLLIN事件发生
            read(connfd,...)
            read直到返回EAGAIN错误
            ret = write(connfd, buf, 10000)
            if(ret < 10000)
            {
                将未发送完成的数据添加到应用层发送缓冲区OutBuffer
            }

            connfd EPOLLOUT 事件到来
            取出应用层发送缓冲区的数据发送 write(connfd, ...);
            知道应用层缓冲区数据发送完毕，或者发送返回EAGAIN

            
*/
const int MAX_CONN_SIZE = 65535;


class WebServer : public noncopyable
{
    public:
        WebServer();
        ~WebServer();
        void init(int port, int trigMode, int threadNum, 
                    int sqlPoolNum, int openLog);

        void setNonBlocking(int fd);
        void eventLoop();
        void eventListen();
        void addfd(int fd);
        void deleteFd(int fd);
        void newConnection();
        void dealWithSignal();

    public:
        shared_ptr<ThreadPool<httpConn>> threadpoolPtr;
    
    private:
        int m_epollfd;
        int m_sockfd;
        int m_port;
        int m_trigMode;
        int m_openLog;
        int m_threadNum;
        int m_sqlPoolNum;
        int m_listenfd;
        int m_pipefd[2];
        int m_alarmInterval; // 定时器间隔
        bool m_timeOut;
        bool m_stopServer;
        client_data users[MAX_FD];
        shared_ptr<Utils> utilsPtr;
        epoll_event m_events[MAX_EVENT_NUMBER];
        vector<shared_ptr<httpConn>> httpConnList; // 客户端连接列表
};

#endif