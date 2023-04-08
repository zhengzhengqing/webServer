#include "Config.h"

Config::Config():port(9696),trigMode(0), threadPoolNum(4),openLogFlag(0),
                 sqlPoolNum(4)
{}

Config::~Config()
{}

void Config::parseArg(int argc, char **argv)
{
    int opt;
    const char *str = "p:m:t:l:q";
    while((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p': 
        {
            port = atoi(optarg);
            break;
        }
        case 'l': //是否移动日志系统
        {
            openLogFlag = atoi(optarg);
            break;
        }
        case 'm':
        {
            trigMode = atoi(optarg);
            break;
        }
        case 't':
        {
            threadPoolNum = atoi(optarg);
            break;
        }
        case 'q':
        {
            sqlPoolNum = atoi(optarg);
        }
        default:
            break;
        }
    }
}