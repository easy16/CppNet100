#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_
//跨平台头文件
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN	//避免引用早期的windows库
	#include <windows.h>
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>//uni std unix系统下的标准库
	#include<arpa/inet.h>
	#include <string.h>

	#define  SOCKET int
	#define  INVALID_SOCKET		(SOCKET)(~0)
	#define  SOCKET_ERRROR						(-1)
#endif //_WIN32

#include <stdio.h>
#include "MessageHeader.hpp"

class EasyTcpClient
{
	SOCKET _sock;
public:
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
	}
	//虚析构函数
	virtual ~EasyTcpClient()
	{
		CloseSocket();
	}
	//初始化socket
	int InitSocket()
	{
#ifdef _WIN32
		//启动windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(ver, &data);
#endif
		//-- 用socket api 建立简易TCP客户端
		// 1 建立一个socket;Ipv4，面向数据流，TCP协议
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>关闭旧连接。。。\n", _sock);
			CloseSocket();
		}
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (INVALID_SOCKET == _sock)
		{
			printf("错误，建立socket失败。。。\n");
			return -1;
		}
		return 0;
	}
	//连接服务器
	void ConnectServer(char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			if (-1 == InitSocket())
			{
				return;
			}			
		}
		// 2 connect 连接服务器
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;//网络类型
		_sin.sin_port = htons(port);//防止主机中的short类型与网络字节序中的不同
#ifdef _WIN32
		//_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		inet_pton(AF_INET, ip, (void*)&_sin.sin_addr.S_un.S_addr);
#else
		//_sin.sin_addr.s_addr = inet_addr("127.0.0.1");//可以定义宏修正错误
		inet_pton(AF_INET, "127.0.0.1", (void*)&_sin.sin_addr.S_addr);
#endif

		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("错误，连接服务器失败。。。\n");
		}
		else {
			printf("连接服务器成功。。。\n");
		}
	}

	//关闭socket
	void CloseSocket()
	{
		if (INVALID_SOCKET != _sock)
		{		
			// 7 closesocket 关闭套接字
#ifdef _WIN32
		closesocket(_sock);
		//清除windows socket环境
		WSACleanup();
#else
		close(_sock);
#endif
		
		}
		_sock = INVALID_SOCKET;
	}

	//查询网络消息
	bool OnRun()
	{
		if (isRun())
		{
			//伯克利socket
			fd_set fdRead;
			//清空数组
			FD_ZERO(&fdRead);
			//赋值
			FD_SET(_sock, &fdRead);
			timeval t = { 1, 0 };
			//其他平台需要sock+1，否则收不到服务端消息
			int ret = select(_sock + 1, &fdRead, 0, 0, &t);
			if (ret < 0)
			{
				printf("<socket=%d>select任务结束1。\n", _sock);
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);

				if (-1 == RecvData(_sock))
				{
					printf("<socket=%d>select任务结束2。\n", _sock);
					return false;
				}
			}
			return true;
		}
		return false;
	}

	//判断是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET _cSock)
	{
		//缓冲区
		char szRecv[4096] = {};
		// 5 接收客户端数据
		int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			printf("与服务器断开连接，任务结束。\n");
			return -1;
		}
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(header);
		return 0;
	}

	//响应网络消息
	void OnNetMsg(DataHeader* header)
	{
		// 6 处理请求		
		switch (header->cmd)
		{
			case CMD_LOGIN_RESULT:
				{
					LoginResult* login = (LoginResult*)header;
					printf("收到服务器数据消息：CMD_LOGIN_RESULT 内容：%d \n", login->result);
				}
				break;
			case CMD_LOGOUT_RESULT:
				{
					LogoutResult* logout = (LogoutResult*)header;
					printf("收到服务器数据消息：CMD_LOGOUT_RESULT 内容：%d \n", logout->result);
				}
				break;
			case CMD_NEW_USER_JOIN:
				{
					NewUserJoin* userJoin = (NewUserJoin*)header;
					printf("收到服务器数据消息：CMD_NEW_USER_JOIN 内容：%d \n", userJoin->socketID);
				}
				break;
		}
	}

	//发送数据
	int SendData(DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
};

#endif//_EasyTcpClient_hpp_