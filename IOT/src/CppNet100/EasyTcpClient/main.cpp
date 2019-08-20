#include <stdio.h>
#include <thread>

#include "EasyTcpClient.hpp"

void cmdThread( EasyTcpClient* client)
{
	while (true)
	{
		// 3 输入请求
		char cmdBuf[256] = {};
		scanf_s("%s", cmdBuf, 256);
		// 4 处理请求
		if (0 == strcmp(cmdBuf, "exit"))
		{
			client->CloseSocket();
			printf("退出cmdThread线程。\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strcpy_s(login.userName, "lyd");
			strcpy_s(login.passWord, "lydmm");
			// 5 向服务器发送请求
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout logout;
			strcpy_s(logout.userName, "lyd");
			// 5 向服务器发送请求
			client->SendData(&logout);
		}
		else
		{
			printf("不支持的命令，请重新输入。 \n");
		}
	}	
}

int main()
{
	EasyTcpClient client;//声明多个对象就可以连接多个服务器
	client.ConnectServer("127.0.0.1", 4567);
	//启动线程
	std::thread t1(cmdThread, &client);
	t1.detach();//与主线程分离
	while (client.isRun())
	{
		client.OnRun();
	}	
	client.CloseSocket();
	printf("已退出，任务结束。\n");
	getchar();
	return 0;
}