#include "EasyTcpServer.hpp"

int main()
{
	EasyTcpServer server;
	server.BindPort(nullptr, 4567);
	server.ListenPort(5);
	while (server.isRun())
	{
		server.OnRun();
	}
	printf("已退出，任务结束。\n");
	getchar();
	return 0;
}