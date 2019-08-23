#include <stdio.h>
#include <thread>

#include "EasyTcpClient.hpp"
bool g_bRun = true;
void cmdThread( /*EasyTcpClient* client*/)
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
			//client->CloseSocket();
			printf("退出cmdThread线程。\n");
			break;
		}
		//else if (0 == strcmp(cmdBuf, "login"))
		//{
		//	Login login;
		//	strcpy_s(login.userName, "lyd");
		//	strcpy_s(login.passWord, "lydmm");
		//	// 5 向服务器发送请求
		//	client->SendData(&login);
		//}
		//else if (0 == strcmp(cmdBuf, "logout"))
		//{
		//	Logout logout;
		//	strcpy_s(logout.userName, "lyd");
		//	// 5 向服务器发送请求
		//	client->SendData(&logout);
		//}
		else
		{
			printf("不支持的命令，请重新输入。 \n");
		}
	}	
}

int main()
{
	const int cCount = 1000;//windows默认客户端最大个数，减去一个服务端，超出了则不会传输数据
	//需要用指针，不然栈内存会爆掉
	EasyTcpClient* client[cCount];//声明多个对象就可以连接多个服务器
	for (int n = 0; n < cCount; n++)
	{
		if (!g_bRun)//中途退出能停止创建
		{
			return 0;
		}
		client[n] = new EasyTcpClient();
	}
	for (int n = 0; n < cCount; n++)
	{
		if (!g_bRun)
		{
			return 0;
		}
		printf("Connect=%d\n", n);
		client[n]->ConnectServer("127.0.0.1", 4567);
	}
	
	//启动线程
	std::thread t1(cmdThread);
	t1.detach();//与主线程分离

	Login login;
	strcpy_s(login.userName, "lyd");
	strcpy_s(login.passWord, "lydmm");
	while (g_bRun/*client.isRun()*/)
	{
		for (int n = 0; n < cCount; n++)
		{			
			client[n]->SendData(&login);
			//client[n]->OnRun();
		}
		
	}	
	for (int n = 0; n < cCount; n++)
	{
		client[n]->CloseSocket();
	}
	printf("已退出，任务结束。\n");
	getchar();
	return 0;
}