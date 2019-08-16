#define WIN32_LEAN_AND_MEAN	//避免引用早期的windows库
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
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
	//-- 用socket api 建立简易TCP服务端
	// 1 建立一个socket;Ipv4，面向数据流，TCP协议
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock)
	{
		printf("错误，建立socket失败。。。\n");
	}
	// 2 bind 绑定用于接收客户端连接的网络端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;//网络类型
	_sin.sin_port = htons(4567);//防止主机中的short类型与网络字节序中的不同
	_sin.sin_addr.S_un.S_addr = INADDR_ANY; //inet_addr("127.0.0.1");
	//上一句可替换为inet_pton(AF_INET, "127.0.0.1", (void*)&_sin.sin_addr.S_un.S_addr);
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))
	{
		printf("错误，绑定网络端口失败。。。\n");
	}
	else {
		printf("绑定网络端口成功。。。\n");
	}
	// 3 listen 监听网络端口
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("错误，监听网络端口失败。。。\n");
	}
	else {
		printf("监听网络端口成功。。。\n");
	}
	// 4 accept 等待接收客户端连接
	sockaddr_in clientAddr = {};//客户端地址
	int nAddrLen = sizeof(sockaddr_in);
	SOCKET _cSock = INVALID_SOCKET;
	
	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock)
	{
		printf("错误，接收到无效的客户端SOCKET。。。\n");
	}
	char sendBuf[20] = { '\0' };
	printf("新客户端加入：socket = %d, IP = %s \n", (int)_cSock, inet_ntop(AF_INET, (void*)&clientAddr.sin_addr, sendBuf, 16));

	char _recvBuf[128] = {};
	while (true)//循环重复接收新的客户端（accept在循环里时）/客户端指令
	{
		// 5 接收客户端数据
		int nLen = recv(_cSock, _recvBuf, 128, 0);
		if (nLen <= 0)
		{
			printf("客户端已退出，任务结束。\n");
			break;
		}
		printf("收到命令：%s \n", _recvBuf);
		// 6 处理请求
		if (0 == strcmp(_recvBuf, "getInfo"))
		{
			// 7 send 向客户端发送一条数据		
			DataPackage dp = { 80, "zhangsan" };
			send(_cSock, (const char*)&dp, sizeof(DataPackage), 0);
		}
		else
		{
			// 7 send 向客户端发送一条数据		
			char msgBuf[] = "???";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);
		}
	}
	// 8 closesocket 关闭套接字
	closesocket(_sock);
	//--------------
	//清除windows socket环境
	WSACleanup();
	printf("已退出，任务结束。");
	getchar();
	return 0;
}