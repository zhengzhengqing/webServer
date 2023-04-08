#include "server.h"


WebServer::WebServer():threadpoolPtr(new ThreadPool<httpConn>),
                        utilsPtr(new Utils)

{
    httpConnList.reserve(MAX_CONN_SIZE);
}

WebServer::~WebServer()
{

}

void WebServer::init(int port, int trigMode, int threadNum, int sqlPoolNum,
                     int openLog)
{
    m_timeOut       = false;
    m_port          = port;
    m_trigMode      = trigMode;
    m_openLog       = openLog;
    m_threadNum     = threadNum;
    m_sqlPoolNum    = sqlPoolNum;
    m_stopServer      = false;
    m_alarmInterval = 1;
    threadpoolPtr->init(m_threadNum);

    ConnectionPool::getInstance()->init("localhost", "root", "123456", "userdb", 3306,10, 0);
    
    string ip("127.0.0.1");
    RedisPool::getInstance()->init(ip, 6379, 10);
    
    if(m_trigMode == 0)
    {
        LOG_INFO("et model");
    }
    else
    {
        LOG_INFO("lt model");
    }
    
}

void WebServer::eventListen()
{
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    int ret = 0;
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr*)&address, sizeof(address));
    
    assert(ret >= 0);

    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    m_epollfd = epoll_create(5);
    if(m_epollfd == -1)
    {
        LOG_ERROR("epoll_create failed!");
    }
    assert(m_epollfd > 0);
    addfd(m_listenfd);

    
    utilsPtr->init(1);

    // 创建全双工管道
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);

    Utils::p_pipefd = m_pipefd;
    utilsPtr->epollfd = m_epollfd;

    utilsPtr->addfd(m_pipefd[0], m_trigMode);
    utilsPtr->setNonBlocking(m_pipefd[1]);

    utilsPtr->addsig(SIGPIPE, SIG_IGN); // 忽略SIGPIPE信号

    //设置信号处理函数
    utilsPtr->addsig(SIGALRM, Utils::sig_handler, false);
    utilsPtr->addsig(SIGTERM, Utils::sig_handler, false);
    alarm(1);
}

void WebServer::addfd(int fd)
{
    epoll_event event;
    event.data.fd = fd;
    
    if(m_trigMode == 0) // ET模式
    {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP ;
    }
    else // LT模式
    {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if(fd != m_listenfd)
        event.events |= EPOLLONESHOT;

    setNonBlocking(fd);
    int res = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &event);
    if(res == 0)
    {
        LOG_INFO("add fd successfully !");
    }
    else
    {
        LOG_ERROR("add fd failed ! ");
    }
       
}

void WebServer::deleteFd(int fd) 
{
    int res = epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, 0);
    if(res == 0)
    {
        //LOG_INFO("delete fd successfully ");
    }
    else
    {
        //LOG_ERROR("delete fd failed !");
    }
}

void WebServer::eventLoop()
{
    while(true)
    {
        int number = epoll_wait(m_epollfd, m_events,MAX_EVENT_NUMBER, -1);
        
        if(number < 0)
        {
            // LOG_ERROR("epoll failure");
            if(number < 0 && errno != EINTR)
                break;
        }
        for(int i = 0; i < number; i ++)
        {
            if(m_events[i].data.fd == m_listenfd)
            {
                newConnection(); // 处理新连接
            }
            else if(m_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                utilsPtr->timeWheel.delTimer(users[m_events[i].data.fd].timer);
                users[m_events[i].data.fd].timer = nullptr;
                deleteFd(m_events[i].data.fd);
                close(m_events[i].data.fd);
                httpConnList[m_events[i].data.fd]->reInitial();
            }
            else if((m_events[i].data.fd == m_pipefd[0]) && (m_events[i].events & EPOLLIN))
            {
                dealWithSignal();
            }
            else if(m_events[i].events & EPOLLIN) // 读事件到来
            {
                // 更新定时器
                utilsPtr->timeWheel.adjustTimer(users[m_events[i].data.fd].timer);
                httpConnList[m_events[i].data.fd]->actionModel_ = 0; // 表示该文件描述符号有数据可读。
                threadpoolPtr->append(&(*httpConnList[m_events[i].data.fd]));
            }
            else if(m_events[i].events & EPOLLOUT)
            {
                utilsPtr->timeWheel.adjustTimer(users[m_events[i].data.fd].timer);
                httpConnList[m_events[i].data.fd]->actionModel_ = 1; // 表示该文件描述符号有数据可写。
                threadpoolPtr->append(&(*httpConnList[m_events[i].data.fd]));
            }
        }
        if(m_timeOut)
        {
            utilsPtr->timer_handler();
            m_timeOut = false;
        }
    }
}

void WebServer::setNonBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}

void WebServer::newConnection()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int newConnFd = accept4(m_listenfd, (struct sockaddr *)&client_addr,
                    &client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if(newConnFd < 0)
    {
        LOG_ERROR(" Server Accept failed");
        exit(1);
    }
    else
    {
        LOG_INFO("new connection fd = %d", newConnFd);
        tw_timer * t =  utilsPtr->timeWheel.addTimer(30); // 设置心跳为30秒
        
        if(t)
        {
            t->user_data = &users[newConnFd];
            t->callback = std::bind(&Utils::cbFunc, &(*utilsPtr), std::placeholders::_1);
            users[newConnFd].timer = t;
            users[newConnFd].sockfd = newConnFd;                                                                                      
        }
    }

    // 使用智能指针维护一个TCP连接
    if(!httpConnList[newConnFd])
    {
        shared_ptr<httpConn> conn(new httpConn);
        httpConnList[newConnFd] = conn;
    }
    else
    {
        httpConnList[newConnFd]->reInitial();
    }
    
    httpConnList[newConnFd]->init(newConnFd, m_epollfd, client_addr, m_trigMode);
}

void WebServer::dealWithSignal()
{
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if(ret == -1 || ret == 0)
    {
        return ;
    }
    else
    {
        for(int i = 0; i < ret; i++)
        {
            switch(signals[i])
            {
                case SIGALRM:
                {
                    m_timeOut = true;
                    break;
                } 
                case SIGTERM:
                {
                    m_stopServer = true;
                    break;
                }
            }
        }   
    }
}
