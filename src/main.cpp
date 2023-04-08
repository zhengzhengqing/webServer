#include "../log/log.h"
#include "../config/Config.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "server.h"

int main(int argc, char ** argv)
{
	
	Config config;
	config.parseArg(argc, argv);
	
	if(config.openLogFlag == 1)
	{
		cout <<"不启动日志系统" <<endl;
	}
	else
	{
		Log::getInstance()->init("../../serverLogs/ServerLog", 
											0, 1000, 50000, 1000);
	}
	
	WebServer server;

	server.init(config.port,config.trigMode, 
				config.threadPoolNum, config.sqlPoolNum, config.openLogFlag);
	
	server.threadpoolPtr->start();
	server.eventListen();
	server.eventLoop();

	return 0;
}

