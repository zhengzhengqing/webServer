#include "timer.h"

int *Utils::p_pipefd = 0;

tw_timer* Time_wheel::addTimer(int timeout)
{
    if(timeout < 0)
    {
        return nullptr;
    }

    int ticks = 0;
    // 根据待插入的定时器的超时值计算它将在时间轮转动多少个滴答后被触发。
    // 并将该滴答书存储与变量ticks中，如果待插入的定时器的值小于时间轮的槽间隔SI。
    // 则将ticks止为1,否则ticks = timeout/SI

    if(timeout < SI)
    {
        ticks = 1;
    }
    else
    {
        ticks = timeout / SI;
    }


    // 计算待插入的定时器应该被插入哪个槽中
    int ts = ((cur_slot -1 ) + (ticks % N)) % N;

    // 创建新的定时器，在时间轮转动rotation圈后被触发，且位于第ts个槽上
    tw_timer * timer = new tw_timer(ts);

    // 如果第ts个槽中无任何定时器，则把新建的定时器插入其中，并将该定时器设置为该槽的头节点
    if( !slots[ts])
    {
        slots[ts] = timer;
    }
    else
    {
        timer->next = slots[ts];
        slots[ts]->prev = timer;
        slots[ts] = timer;
    }
    return timer;
}

void Time_wheel::delTimer(tw_timer * timer)
{
    if( !timer)
    {
        return ;
    }

    int ts = timer->time_slot;
    
    // slots[ts]是目标定时器所在的槽的头结点，如果目标定时器就是该头节点，
    // 则需要重置第ts个槽的头节点

    if( timer == slots[ts])
    {
        slots[ts] = slots[ts]->next;
        if(slots[ts])
        {
            slots[ts]->prev = nullptr;
        }
        delete timer;
    }
    else
    {
        timer->prev->next = timer->next;
        if(timer->next)
        {
            timer->next->prev = timer->prev;
        }
        delete timer;
    }
}

void Time_wheel::tick()
{
    tw_timer * tmp = slots[cur_slot];
    
    while(tmp)
    {
        tmp->callback(tmp->user_data);
        if(tmp == slots[cur_slot])
        {
            slots[cur_slot] = tmp->next;
            delete tmp;
            if(slots[cur_slot])
            {
                slots[cur_slot]->prev = nullptr;
            }
            tmp = slots[cur_slot];
        }
        else
        {
            tmp->prev->next = tmp->next;
            if(tmp->next)
            {
                tmp->next->prev = tmp->prev;
            }
            tw_timer * tmp2 = tmp->next;
            delete tmp;
            tmp = tmp2;
        }
    }
    cur_slot = ++cur_slot % N; // 更新时间轮的当前槽
}

void Time_wheel::adjustTimer(tw_timer * timer)
{
    if(timer)
    {
        int index = (cur_slot - 1 + N) % N;

        // 说明次定时器是在头结点
        cout <<"timer->time_slot = " <<timer->time_slot <<endl;
        if(timer == slots[timer->time_slot])
        {
            cout <<"head " <<endl;
            slots[timer->time_slot] = nullptr;
            //slots[timer->time_slot]->prev = nullptr;

            if(!slots[index])
            {
                slots[index] = timer;
            }
            else
            {
                timer->next = slots[index];
                slots[index]->prev = timer;
                slots[index] = timer;
            }
        }
        else
        {
            // timer 在尾结点
            if(!timer->next)
            {
                timer->prev->next = nullptr;
            }
            else
            {
                timer->prev->next = timer->next;
                timer->next->prev = timer->prev;
            }

            
            if(!slots[index])
            {
                slots[index] = timer;
            }
            else
            {
                
                timer->next = slots[index];
                slots[index]->prev = timer;
                slots[index] = timer;
            }
        }
        timer->time_slot = index;
        cout <<"cur_slot = " <<cur_slot <<endl;
        cout <<" alter timer->time_slot = " <<timer->time_slot <<endl;
    }
}

void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if(restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::cbFunc(client_data * user)
{
    epoll_ctl(Utils::epollfd, EPOLL_CTL_DEL, user->sockfd, 0);
    assert(user);
    cout <<"sock id = " <<user->sockfd << " close" <<endl;
    close(user->sockfd);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    timeWheel.tick();
    alarm(timeSlot);
}

//信号处理函数
void Utils::sig_handler(int sig)
{

    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    int res = send(p_pipefd[1], (char *)&msg, 1, 0);
    if(res == -1)
    {
        cout <<"return a -1 value"<<endl;
        cout <<"errno = " <<errno <<endl;
    }
    errno = save_errno;
}

void Utils::addfd(int fd, int trigMode)
{
    epoll_event event;
    event.data.fd = fd;
    
    if(trigMode == 0) // ET模式
    {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP ;
    }
    else // LT模式
    {
        event.events = EPOLLIN | EPOLLRDHUP;
    }

    setNonBlocking(fd);
    int res = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    if(res == 0)
    {
        //LOG_INFO("add fd successfully !");
    }
    else
    {
        //LOG_ERROR("add fd failed ! ");
    }
}

void Utils::setNonBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}

void Utils::init(int timeslot)
{
    timeSlot = timeslot;
}