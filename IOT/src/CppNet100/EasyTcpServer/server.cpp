#include "EasyTcpServer.hpp"
#include <thread>

bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		// 3 输入请求
		char cmdBuf[256] = {};
		scanf_s("%s", cmdBuf, 256);
		// 4 处理请求
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("退出cmdThread线程。\n");
			break;
		}
		else
		{
			printf("不支持的命令，请重新输入。 \n");
		}
	}
}
int main()
{
	EasyTcpServer server;
	server.BindPort(nullptr, 4567);
	server.ListenPort(5);
	//启动线程
	std::thread t1(cmdThread);
	t1.detach();//与主线程分离
	while (g_bRun)//server.isRun()
	{
		server.OnRun();
	}
	printf("已退出，任务结束。\n");
	getchar();
	return 0;
}