#ifndef CONFIG_H_
#define CONFIG_H_

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include "../noncopyable/noncopyable.h"
using namespace std;

class Config : public noncopyable
{
    public:
        Config();
        ~Config();
        void parseArg(int argc, char **argv);
    
    public:
        int port;           // 默认9696
        int trigMode;       // 0 表示ET， 1表示LT，默认ET模式
        int threadPoolNum;  // 默认4个
        int openLogFlag;    // 是否打开日志 0表示打开，1表示关闭，默认打开
        int sqlPoolNum;     // 数据库连接池数量
};

#endif
