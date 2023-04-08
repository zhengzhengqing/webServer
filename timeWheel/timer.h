#ifndef TIME_WHEEL_H_
#define TIME_WHEEL_H_

#include <time.h>
#include <iostream>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <functional>
#include "../noncopyable/noncopyable.h"

using namespace std;

const int BUFFER_SIZE = 64;

class tw_timer;

struct client_data
{
    int sockfd;
    sockaddr_in address;
    char buf[BUFFER_SIZE];
    tw_timer * timer;
    client_data()
    {
        sockfd = -1;
        timer = nullptr;
        memset(buf, 0 , BUFFER_SIZE);
    };
};

typedef std::function<void(client_data*)> callBack;

// 定时器类
class tw_timer : public noncopyable
{
    public:
        tw_timer(int ts):
                    next(nullptr),
                    prev(nullptr),
                    rotation(0),
                    time_slot(ts)
        {}
    
    public:
        int rotation;  // 记录定时器在时间轮转多少圈后生效
        int time_slot; // 记录定时器属于时间轮上的哪个槽
        void (*cb_func)(client_data *); // 定时器回调函数
        callBack callback;
        client_data * user_data; // 客户数据
        tw_timer * next; // 只想下一个定时器
        tw_timer * prev;// 只想前一个定时器
};

class Time_wheel : public noncopyable
{
    public:
        Time_wheel():cur_slot(0)
        {
            for(int i = 0; i < N; i++)
            {
                slots[i] = nullptr;
            }
        }

        ~Time_wheel()
        {
            for(int i = 0; i < N; i++)
            {
                tw_timer * tmp = slots[i];
                while(tmp)
                {
                    slots[i] = tmp->next;
                    delete tmp;
                    tmp = slots[i];
                }
            }
        }

    public:
        tw_timer* addTimer(int timeout);
        void delTimer(tw_timer * timer);
        void tick();
        void adjustTimer(tw_timer * timer); 

    private:
        static const int N = 30; // 时间槽数目
        static const int SI = 1; //槽之间的时间间隔
        tw_timer * slots[N];
        int cur_slot; // 时间论的当前槽
};

class Utils : public noncopyable
{
    public:
        Utils():epollfd(-1)
        {}
        ~Utils(){}

        void init(int timeslot);
        
        //信号处理函数
        static void sig_handler(int sig);

        //设置信号函数
        void addsig(int sig, void(handler)(int), bool restart = true);

        //定时处理任务，重新定时以不断触发SIGALRM信号
        void timer_handler();

        void cbFunc(client_data * user);
        void addfd(int fd, int trigMode);
        void setNonBlocking(int fd);

    public:
        static int * p_pipefd;
        Time_wheel  timeWheel;
        int epollfd;
        int timeSlot; // 定时时间
};

#endif