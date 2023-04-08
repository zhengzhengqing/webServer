#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <map>
#include <set>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <iostream>
#include <algorithm>
#include "../log/log.h"
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include "muduo/base/Mutex.h"
#include "../redis/redisPool.h"
#include "../noncopyable/noncopyable.h"
#include "../mysqlConnectionPool/connectionpool.h"
#include <sys/sendfile.h>
//#include "../mysqlConnectionPool/connectionpool.h"

//struct stat  
//{   
//    dev_t       st_dev;     /* ID of device containing file -文件所在设备的ID*/  
//    ino_t       st_ino;     /* inode number -inode节点号*/    
//    mode_t      st_mode;    /* protection -保护模式?*/    
//    nlink_t     st_nlink;   /* number of hard links -链向此文件的连接数(硬连接)*/    
//    uid_t       st_uid;     /* user ID of owner -user id*/    
//    gid_t       st_gid;     /* group ID of owner - group id*/    
//    dev_t       st_rdev;    /* device ID (if special file) -设备号，针对设备文件*/    
//    off_t       st_size;    /* total size, in bytes -文件大小，字节为单位*/    
//    blksize_t   st_blksize; /* blocksize for filesystem I/O -系统块的大小*/    
//    blkcnt_t    st_blocks;  /* number of blocks allocated -文件所占块数*/    
//    time_t      st_atime;   /* time of last access -最近存取时间*/    
//    time_t      st_mtime;   /* time of last modification -最近修改时间*/    
//    time_t      st_ctime;   /* time of last status change - */  
//}

using namespace std;

const int READ_BUFFER_SIZE  = 2048;
const int WRITE_BUFFER_SIZE = 4080;
const int FILENAME_LEN = 200;

class httpConn  : public noncopyable 
{
    public:
        enum REDIS_REPLY
        {
            R_REPLY_NULL,
            R_REDIS_CONTEXT_ERROR,
            R_REDIS_REPLY_STRING ,
            R_REDIS_REPLY_ARRAY ,
            R_REDIS_REPLY_INTERGER ,
            R_REDIS_REPLY_NIL ,
            R_REDIS_REPLY_OK ,
            R_REDIS_REPLY_ERROR 
        };

        enum HttpRequestParseState
        {
            kExpectRequestLine,
            kExpectHeaders,
            kExpectBody,
            kGotAll,
        };

        enum Method
        {
            kInvalid, kGet, kPost, kHead, kPut, kDelete
        };

        enum Version
        {
            kUnknow, kHttp10, kHttp11
        };

        enum HttpCode
        {
            NO_REQUEST, // 表示请求不完整
            GET_REQUEST,
            BAD_REQUEST,
            NO_RESOURCE,
            FORBIDDEN_REQUEST,
            FILE_REQUEST,
            INTERNAL_ERROR,
            CLOSED_CONNECTION
        };

    public:
        httpConn()
            :state_(kExpectRequestLine),
            method_(kInvalid),
            version_(kUnknow),
            httpcode(FILE_REQUEST),
            epollfd_(-1),
            connfd_(-1),
            actionModel_(0),
            trigerMode_(0),
            readableIndex_(0),
            leftIndex_(0),
            bytesHaveSend_(0),
            bytesToSend_(0),
            mivCount_(0),
            reply(NULL)
        {
            urlSet.insert("/");
            urlSet.insert("/register");
            urlSet.insert("/login");
            urlSet.insert("/logon");
            urlSet.insert("/favicon.ico");
            urlSet.insert("/xxx.jpg");
            urlSet.insert("/3CGISQL.cgi");
            memset(writeBuffer, 0, WRITE_BUFFER_SIZE);
            memset(readBuffer, 0, READ_BUFFER_SIZE);
            memset(fileName, 0, FILENAME_LEN);
        }

        ~httpConn()
        {}

    public:
        void readData();
        void writeData();
        void readDataByLT();
        void readDataByET();
        HttpCode parseRequest(char * buf);
        char * findCRLF(char *buf) const;
        bool processRequestLine(const char *begin, const char *end);
        bool setMethod(const char *start, const char *end);
        void setPath(const char *start, const char *end);
        void setQuery(const char* start, const char* end);
        void setVersion(Version v);
        void addHeader(const char *start, const char *colon, const char *end);
        void addBody(const char * start, const char *equal, const char *end);
        char * findAnd(char *buf) const;
        REDIS_REPLY getValue(const string &key, string &value);
        REDIS_REPLY setValue(const string &key, const string &value);
        REDIS_REPLY redisCom(const string &key, redisContext * re,
                                string &value, RedisRaii &ra);
        void init(int fd, int epollfd, struct sockaddr_in & addr, int trigmodel);
        void modfd(int ev, int fd);
        void addfd(int fd);
        void unmap();
        void setNonBlocking(int fd);
        void reInitial();
        void analyseRequest(char * buf);
        void addResponse(const char * format, ...);
        void deleteFd(); 
        void addResponseHeader(int status, const char * title);
        void addContentLength(int length);
        void addContentType();
        void addContent(const char * context);
        void addResponseBody(char * boddy);
        void addLinkStatus();
        void initialBuffer();

    public:
        int trigerMode_; // 默认LT模式
        int connfd_;
        int epollfd_;
        string path_;
        string query_;
        int mivCount_;
        int readableIndex_;
        int leftIndex_;
        int sendIndex_;
        int bytesHaveSend_; // 已经发送的字节数
        int bytesToSend_ ; //将要发送的字节数
        char readBuffer[READ_BUFFER_SIZE];
        char writeBuffer[WRITE_BUFFER_SIZE];
        int actionModel_; // 0 表示有数据可读， 1 表示有数据要写
        struct sockaddr_in addr_;
        HttpRequestParseState state_;
        Method method_;
        Version version_;
        map<string, string> headers_;
        map<string, string> bodys_;
        map<string, string> users_; // 保存用户帐号和密码
        muduo::MutexLock mutex_;
        char fileName[FILENAME_LEN]; // 保存html路径
        char *file_address;
        struct stat file_stat;
        struct iovec miv[2];
        set<string> urlSet; //URL 集合
        HttpCode httpcode;
        redisReply *reply;
};

#endif