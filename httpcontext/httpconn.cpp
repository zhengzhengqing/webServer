#include "httpconn.h"

const char CRLF[] = "\r\n";
const char *AND = "&";

const char *error_404_title = "Not Found";
const char *error_404_form = "The requested URL was not found on this server";

httpConn::HttpCode httpConn::parseRequest(char *buf)
{
    bool ok = true;
    bool hasMore = true;
    char * start = buf;
    while(hasMore)
    {
        if(state_ == kExpectRequestLine)  // 处理请求行
        {
            char *crlf = findCRLF(buf);
            if(crlf)
            {
                ok = processRequestLine(start, crlf);
                if(ok)
                {
                    start = crlf + 2; 
                    state_ = kExpectHeaders;  
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if(state_ == kExpectHeaders) // 处理请求头
        {
            char * crlf = findCRLF(start);
            if(crlf)
            {
                const char * colon = std::find(start, crlf, ':');
                if(colon != crlf)
                {
                    addHeader(start, colon, crlf);
                }
                else
                {
                    state_ = kExpectBody;
                }
                
                start = crlf + 2;
            }
            else
            {
                hasMore = false;
            }
        }
        else if(state_ == kExpectBody) // 处理请求体
        {
            bool continue_ = true;
            if(*start == '\0')
                continue_ = false;
            
            while(continue_)
            {
                char *and_ = findAnd(start);
                if(0 == memcmp(and_, AND, 1))
                {
                    const char * equal = std::find(start, and_, '=');
                    if(0 != memcmp(equal, AND, 1))
                    {
                        addBody(start, equal, and_);
                        start = and_ + 1;
                    }
                    else
                    {
                        continue_ = false;
                    }
                }
                else
                {
                    const char * equal = std::find(start, and_, '=');
                    if(0 != memcmp(equal, AND, 1))
                    {
                        addBody(start, equal, and_);
                    }
                    continue_ = false;
                }
            }
            hasMore = false;
        }
    }
    return httpcode;
}

char * httpConn::findAnd(char *buf) const
{
    return search(buf, buf + strlen(buf), AND, AND + 1);
}

 char * httpConn::findCRLF(char * buf) const 
{
    return  search(buf, buf + strlen(buf), CRLF, CRLF + 2);
}

bool httpConn::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char * start = begin;

    // 返回空格的位置
    const char * space = std::find(start, end ,' ');
    // 設置請求方法
    if(space != end && setMethod(start, space))
    {
    
        start = space + 1;
        space = std::find(start, end, ' '); 
        if(space != end)
        {
           
            const char* question = std::find(start, space, '?');
            if (question != space)
            {
                setPath(start, question);
                setQuery(question, space);
            }
            else
            {
                setPath(start, space);
            }

            if(httpcode == BAD_REQUEST)
                return false;
            start = space + 1;
            succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");

            if (succeed)
            {
                if (*(end-1) == '1')
                {
                    setVersion(kHttp11);
                }
                else if (*(end-1) == '0')
                {
                    setVersion(kHttp10);
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}


bool httpConn::setMethod(const char *start, const char *end)
{
    string m(start, end);
    if(m == "GET")
        method_ = kGet;
    else if(m == "POST")
        method_ = kPost;
    else if(m == "HEAD")
        method_ = kHead;
    else if(m == "PUT")
        method_ = kPut;
    else if(m == "DELETE")
        m = kDelete;
    else
        method_ = kInvalid;
            
    return method_ != kInvalid;
}

void httpConn::setPath(const char *start, const char *end)
{
    path_.assign(start, end);
    if(urlSet.count(path_) == 0)
    {
        LOG_INFO("there is no URL in methond");
        httpcode = BAD_REQUEST; //表示请求资源不存在
    }
}

void httpConn::setQuery(const char* start, const char* end)
{
    query_.assign(start, end);
}

void httpConn::setVersion(Version v)
{
    version_ = v;
}

void httpConn::addHeader(const char *start, const char *colon, const char *end)
{
    string field(start, colon);
    ++colon;
    while(colon < end && isspace(*colon)) // 去除空格
        ++colon;
    
    string value(colon, end);
    while (!value.empty() && isspace(value[value.size()-1]))
    {
      value.resize(value.size()-1);
    }
    headers_[field] = value;
}

void httpConn::addBody(const char * start, const char *equal, const char *end)
{
    string filed(start, equal);
    ++equal;
    string value(equal, end);
    users_[filed] = value;
}

void httpConn::init(int fd, int epollfd,  struct sockaddr_in & addr, int trigmodel)
{
    
    trigerMode_ = trigmodel;
    connfd_ = fd;
    epollfd_ = epollfd;
    addr_.sin_port = addr.sin_port;
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = addr.sin_addr.s_addr;
    const char * filename = "/home/zzq/unix/webServer/html";
    int len = strlen(filename);
    strncpy(fileName, filename, len);
    addfd(fd);
}

void httpConn::reInitial()
{
    mivCount_ = 0;
    bytesHaveSend_ = 0;
    bytesToSend_ = 0;
    state_ = kExpectRequestLine;
    method_ = kInvalid;
    version_ = kUnknow;
    epollfd_ = -1;
    connfd_ = -1;
    actionModel_ = 0;
    trigerMode_ = 0;
    readableIndex_ = 0;
    leftIndex_ = 0;
    reply = NULL;
    memset(readBuffer, 0, READ_BUFFER_SIZE);
    memset(writeBuffer, 0, WRITE_BUFFER_SIZE);
}

void httpConn::readData()
{
    if(trigerMode_ == 0)  
    {
        readDataByET();
    }
    else                
    {
        readDataByLT();
    }
    
    analyseRequest(readBuffer);
}


void httpConn::deleteFd()
{
    int res = epoll_ctl(epollfd_, EPOLL_CTL_DEL, connfd_, 0);
    if(res == 0)
    {
        ////LOG_INFO("deletre file descriptor = %d!", connfd_);
    }  
    else
    {
        //LOG_ERROR("delete file descriptor failed = %d!", connfd_);
    }
}


void httpConn::modfd(int ev, int fd)
{
    epoll_event event;
    event.data.fd = fd;

    if(trigerMode_ == 0) // ET
    {
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    }
    else
    {
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    }

    int res = epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event);
    if(res == -1)
    {
        //LOG_INFO("modfd descriptor failed!");
    }
    else
    {
        //LOG_INFO("modfd descriptor successfully！");
    }
    
}

void httpConn::addfd(int fd)
{

    epoll_event event;
    event.data.fd = fd;
    
    if(trigerMode_ == 0) 
    {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP ;
    }
    else 
    {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    event.events |= EPOLLONESHOT;

    setNonBlocking(fd);
    int res = epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event);
    if(res == 0)
    {
        //LOG_INFO("add a file descriptor successfully!");
    }
    else
    {
        //LOG_INFO("add a file descriptor failed");
    }
        
}

void httpConn::setNonBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    //LOG_INFO("set nonblocking successfully!");
}


void httpConn::readDataByET()
{
    while(true)
    {
        int ret = recv(connfd_, readBuffer + readableIndex_,
                         READ_BUFFER_SIZE - readableIndex_, 0);
       
        if(ret == 0)
        {
            cout <<"close close close close close http"<<endl;
            deleteFd();
            close(connfd_);
            break;
        }
        else if(ret > 0)
        {
            readableIndex_ += ret;
        }
        else if(ret == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
        }
    }
    
    // string msg(readBuffer);
    // cout <<"msg = " << msg <<endl;
}

void httpConn::initialBuffer()
{
    reply = NULL;
    mivCount_ = 0;
    bytesHaveSend_ = 0;
    bytesToSend_ = 0;
    state_ = kExpectRequestLine;
    method_ = kInvalid;
    version_ = kUnknow;
    httpcode=FILE_REQUEST;
    readableIndex_ = 0;
    leftIndex_ = 0;
    memset(readBuffer, 0, READ_BUFFER_SIZE);
    memset(writeBuffer, 0, WRITE_BUFFER_SIZE);
}

void httpConn::readDataByLT()
{
    int ret = recv(connfd_, readBuffer + readableIndex_, 
                        READ_BUFFER_SIZE - readableIndex_, 0);
    if(ret == 0)
    {
        deleteFd();
        close(connfd_);
    }
    else if(ret > 0)
    {
        readableIndex_ += ret;
    }
    else if(ret == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            //LOG_INFO("msg has readed !");
        }
    }
}

void httpConn::writeData()
{
    if(bytesToSend_ == 0) // 没有数据将要发送
    {
        modfd(EPOLLIN, connfd_);
        initialBuffer();
    }
    else
    {
        int res = writev(connfd_, miv, mivCount_);
        
        if(res < 0)
        {
            if(errno == EAGAIN)
            {
                modfd(EPOLLOUT, connfd_);
            }
            unmap();
        }

        bytesHaveSend_  += res;
        bytesToSend_    -= res;

        if(bytesHaveSend_ >= miv[0].iov_len)
        {
            miv[0].iov_len  = 0;
            miv[1].iov_base = file_address + (bytesHaveSend_ - leftIndex_);
            miv[1].iov_len  = bytesToSend_;
        }
        else
        {
            miv[0].iov_base = writeBuffer + bytesHaveSend_;
            miv[0].iov_len  = miv[0].iov_len - bytesHaveSend_;
        }

        if(bytesToSend_ <= 0)
        {
            modfd(EPOLLIN, connfd_);
            initialBuffer();
        }
        else
        {
            modfd(EPOLLOUT, connfd_);
        }
    }
}

void httpConn::unmap()
{
    if (file_address)
    {
        munmap(file_address, file_stat.st_size);
        file_address = 0;
    }
}

void httpConn::analyseRequest(char * buf)
{
    char filename[200];
    memset(filename, 0, sizeof(filename));
    char path[50];
    memset(path, 0, sizeof(path));

    int len = strlen(fileName);
    strncpy(filename, fileName, len);
    
    if(parseRequest(buf) == BAD_REQUEST) // 请求不存在
    {
        addResponseHeader(404, error_404_title);
        addLinkStatus();
        addContentLength(strlen(error_404_form));
        addContent(error_404_form);

        //缓冲区0保存响应行和响应头
        miv[0].iov_base = writeBuffer;
        miv[0].iov_len = leftIndex_;
        bytesToSend_ = leftIndex_;
        mivCount_ = 1;
    }
    else
    {
        addResponseHeader(200, "ok");
        addLinkStatus();
        addContentType();
        
        if(path_ == "/") // home页面，主页面
            strcpy(path, "/home.html");
        else if(path_ == "/3CGISQL.cgi")
        {
            MYSQL *mysql = NULL;
            connectionRAII connRaii(&mysql, ConnectionPool::getInstance());

            string query_order("select id from usertb where name = ");
            query_order.append("'");
            query_order.append(users_["user"]);
            query_order.append("'");

            if(mysql_query(mysql, query_order.c_str()))
            {
                LOG_ERROR( "SELECT error:%s", mysql_error(mysql));
            }

            MYSQL_RES * result = mysql_store_result(mysql);
            if(result == nullptr)
            {
                LOG_ERROR( "mysql_store_result error:%s", mysql_error(mysql));
            }

            // 返回结果集中的列数目 此时只是检测password一个字段，所以num_fileds = 1
            //int num_fields = mysql_num_fields(result);
            int num_rows = mysql_num_rows(result);

            if(num_rows != 0)
                strcpy(path, "/registerError.html");
            else
            {
                
                query_order = "insert into usertb(name, password) values(";
                query_order.append("'");
                query_order.append(users_["user"]);
                query_order.append("'");
                query_order.append(",");
                query_order.append("'");
                query_order.append(users_["password"]);
                query_order.append("'");
                query_order.append(")");

                if(mysql_query(mysql, query_order.c_str()))
                {
                    LOG_ERROR( "SELECT error:%s", mysql_error(mysql));
                }
                else
                {
                    strcpy(path, "/registerSuccessfully.html");
                }
            }
        }
        else if(path_ == "/logon") //body为登录数据
        {
            // RAII 封装
            MYSQL *mysql = NULL;
            connectionRAII connRaii(&mysql, ConnectionPool::getInstance());

            string query_order("select password from usertb where name = ");
            query_order.append("'");
            query_order.append(users_["user"]);
            query_order.append("'");

            if(mysql_query(mysql, query_order.c_str()))
            {
                LOG_ERROR( "SELECT error:%s", mysql_error(mysql));
            }

            MYSQL_RES * result = mysql_store_result(mysql);
            if(result == nullptr)
            {
                LOG_ERROR( "mysql_store_result error:%s", mysql_error(mysql));
            }

            // 返回结果集中的列数目 此时只是检测password一个字段，所以num_fileds = 1
            //int num_fields = mysql_num_fields(result);
            int num_rows = mysql_num_rows(result);
            
            if(num_rows == 0)
                strcpy(path, "/notExisted.html");
            else
            {
                //返回所有字段结构的数组
                //MYSQL_FIELD * fields = mysql_fetch_fields(result);

                //从结果集中获取下一行，将对应的用户名和密码，存入map中
                // while (MYSQL_ROW row = mysql_fetch_row(result))
                // {
                //     password(row[0]);
                // }

                MYSQL_ROW row = mysql_fetch_row(result);
                string password(row[0]);
                if(password == users_["password"])
                    strcpy(path, "/picture.html");
                else
                    strcpy(path, "/loginError.html");
            }
        }
        else
        {
            if(path_ != "/xxx.jpg")
                path_.append(".html");
            strcpy(path, path_.c_str());
        }
            

        strncpy(filename + len, path, strlen(path));
        if(stat(filename, &file_stat) < 0)
        {
            LOG_ERROR("stat error errno = %d", errno);
        }
            
        int fileFd;
        if((fileFd = open(filename,O_RDONLY)) != -1)
        {
            file_address = (char*)mmap(0, file_stat.st_size,
                                PROT_READ, MAP_PRIVATE, fileFd, 0);

            close(fileFd);
            addContentLength(file_stat.st_size); // 添加相应体的字节大小
                
            //缓冲区0保存响应行和响应头
            miv[0].iov_base = writeBuffer;
            miv[0].iov_len = leftIndex_;

            //缓冲区1保存html文件
            miv[1].iov_base = file_address;
            miv[1].iov_len  = file_stat.st_size;
            bytesToSend_ = leftIndex_ + file_stat.st_size;
            mivCount_ = 2;
        }
    }
    writeData();
}

httpConn::REDIS_REPLY httpConn::getValue(const string &key, string &value)
{
    string cmd = "get " + key;
    redisContext * redis = NULL;
    RedisRaii raii(&redis, RedisPool::getInstance());
    redisCom(key, redis, value, raii); 

    

    
}

httpConn::REDIS_REPLY httpConn::setValue(const string &key, const string &value)
{

}

httpConn::REDIS_REPLY httpConn::redisCom(const string &key, redisContext * re, 
                                        string &value, RedisRaii &ra)
{
    reply = (redisReply *)redisCommand(re, key.c_str(), value, ra);
    if(reply == NULL)
    {
        return  R_REPLY_NULL;
    }

    if(re->err)
    {
        return R_REDIS_CONTEXT_ERROR;
    }

    if(reply->type == REDIS_REPLY_ERROR)
    {
        return R_REDIS_REPLY_ERROR;
    }
    else if(reply->type == REDIS_REPLY_STATUS)
    {
        if(!strcmp(reply->str, "ok"))
        {
            return R_REDIS_REPLY_OK;
        }
    }
}

void httpConn::addResponse(const char * format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(writeBuffer + leftIndex_, 
        WRITE_BUFFER_SIZE -(leftIndex_ + 1),format, arg_list);

    leftIndex_ += len;
    va_end(arg_list);
}

void httpConn::addResponseHeader(int status, const char * title)
{
    addResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

void httpConn::addContentLength(int length)
{
    addResponse("Content-Length:%d\r\n\r\n", length);
}

void httpConn::addContentType()
{
    addResponse("Content-Type:%s\r\n", "text/html");
}

void httpConn::addContent(const char * context)
{
    addResponse("%s", context);
}

void httpConn::addResponseBody(char * body)
{
    addResponse("%s", body);
}

void httpConn::addLinkStatus()
{
    addResponse("Connection:%s\r\n", "keep-alive");
}