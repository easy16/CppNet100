#define WIN32_LEAN_AND_MEAN	//避免引用早期的windows库
#include <WinSock2.h>
#include <windows.h>
#include <stdio.h>
#include <WS2tcpip.h>

//要求c\s字节序一致、对齐
struct DataPackage
{
	int age;
	char name[32];
};

int main()
{
	//启动windows socket 2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
	//-- 用socket api 建立简易TCP客户端
	// 1 建立一个socket;Ipv4，面向数据流，TCP协议
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock)
	{
		printf("错误，建立socket失败。。。\n");
	}
	// 2 connect 连接服务器
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;//网络类型
	_sin.sin_port = htons(4567);//防止主机中的short类型与网络字节序中的不同
	//_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	inet_pton(AF_INET, "127.0.0.1", (void*)&_sin.sin_addr.S_un.S_addr);
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("错误，连接服务器失败。。。\n");
	}
	else {
		printf("连接服务器成功。。。\n");
	}
		
	while (true)
	{
		// 3 输入请求
		char cmdBuf[128] = {};
		scanf_s("%s", cmdBuf, 128);
		// 4 处理请求
		if (0 == strcmp(cmdBuf, "exit"))
		{
			printf("收到退出命令，任务结束。");
			break;
		}
		else
		{
			// 5 向服务器发送请求
			send(_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
		}
		// 6 recv 接收服务器信息
		char recvBuf[256] = {};
		int nLen = recv(_sock, recvBuf, 256, 0);
		if (nLen > 0)
		{
			DataPackage* dp = (DataPackage*)recvBuf;
			printf("接收到数据：年龄=%d，姓名=%s \n", dp->age, dp->name);
		}
	}	
	// 7 closesocket 关闭套接字
	closesocket(_sock);
	//--------------
	//清除windows socket环境
	WSACleanup();
	printf("已退出，任务结束。");
	getchar();
	return 0;
}