#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

//跨平台头文件
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN	//避免引用早期的windows库
#define _WINSOCK_DEPRECATED_NO_WARNINGS	//避免不能使用socket的旧函数出现错误
#include <WS2tcpip.h>	//socket的新函数头文件
#include <windows.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>//uni std unix系统下的标准库
#include<arpa/inet.h>
#include <string.h>

#define  SOCKET int
#define  INVALID_SOCKET		(SOCKET)(~0)
#define  SOCKET_ERRROR						(-1)
#endif 
#include <stdio.h>
#include <vector>
#include "MessageHeader.hpp"

class EasyTcpServer
{
private:
	SOCKET _sock;
	std::vector<SOCKET> _clients;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
	}
	virtual ~EasyTcpServer()
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
			printf("<socket=%d>关闭旧连接。。。\n", (int)_sock);
			CloseSocket();
		}
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (INVALID_SOCKET == _sock)
		{
			printf("错误，建立socket失败。。。\n");
			return -1;
		}
		else
		{
			printf("建立socket=<%d>成功。。。\n", (int)_sock);
		}
		return 0;
	}
	//绑定ip和端口号
	int BindPort(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			if (-1 == InitSocket())
			{
				return -1;
			}
		}
		// 2 bind 绑定用于接收客户端连接的网络端口
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;//网络类型
		_sin.sin_port = htons(port);//防止主机中的short类型与网络字节序中的不同

#ifdef _WIN32
		if (ip) 
		{
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}		
#else
		if (ip)
		{
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("错误，绑定网络端口<%d>失败。。。\n", port);
		}
		else {
			printf("绑定网络端口<%d>成功。。。\n", port);
		}
		return ret;
	}
	//监听端口号
	int ListenPort(int n)//等待连接数n
	{
		// 3 listen 监听网络端口
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("socket=<%d>错误，监听网络端口失败。。。\n", (int)_sock);
		}
		else {
			printf("socket=<%d>监听网络端口成功。。。\n", (int)_sock);
		}
		return ret;
	}
	//接收客户端连接
	SOCKET AcceptClient()
	{
		// 4 accept 等待接收客户端连接
		sockaddr_in clientAddr = {};//客户端地址
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		_cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif // _WIN32			
		if (INVALID_SOCKET == _cSock)
		{
			printf("socket=<%d>错误，接收到无效的客户端SOCKET。。。\n", (int)_sock);
		}
		else
		{			
			NewUserJoin userJoin;
			SendData2All(&userJoin);
			char sendBuf[20] = { '\0' };
			printf("socket=<%d>新客户端加入：socket = %d, IP = %s \n", (int)_sock, (int)_cSock, inet_ntop(AF_INET, (void*)&clientAddr.sin_addr, sendBuf, 16));
			_clients.push_back(_cSock);
		}
		return _cSock;
	}
	//关闭socket
	void CloseSocket()
	{
		if (INVALID_SOCKET != _sock)
		{
#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				closesocket(_clients[n]);
			}
			// 8 closesocket 关闭套接字
			closesocket(_sock);
			//--------------
			//清除windows socket环境
			WSACleanup();
#else
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				close(_clients[n]);
			}
			// 8 closesocket 关闭套接字
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
			//伯克利套接字 BSDsocket
			fd_set fdRead;//描述符（socket）集合
			fd_set fdWrite;
			fd_set fdExp;
			//集合计数清零
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);
			//将服务端socket加入集合
			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExp);

			SOCKET maxSock = _sock;
			//size_t不能做--
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				//将客户端socket加入集合
				FD_SET(_clients[n], &fdRead);
				if (maxSock < _clients[n])
				{
					maxSock = _clients[n];
				}
			}

			//nfds 是一个整数值 是指fd_set集合中所有描述符（socket）的范围，而不是数量，
			//既是所有文件描述符最大值+1 在windows中这个参数可以写0
			//select最后一个参数是null，是阻塞模式（有数据可操作的时候才返回），纯接收数据的服务可以接受
			timeval t = { 0, 0 };//查询时间为1 最大查询时间为1并非等待1s 非阻塞网络模型  综合性网络程序
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			if (ret < 0)
			{
				printf("select任务结束。\n");
				CloseSocket();
				return false;
			}
			//判断描述符是否在集合中
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				AcceptClient();
			}
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				if (FD_ISSET(_clients[n], &fdRead))
				{
					if (-1 == RecvData(_clients[n]))
					{
						//客户端退出在客户端集合中删除客户端
						auto iter = _clients.begin() + n;
						if (iter != _clients.end())
						{
							_clients.erase(iter);
						}
					}
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
			printf("客户端<Socket=%d>已退出，任务结束。\n", _cSock);
			return -1;
		}
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(_cSock, header);
		return 0;
	}

	//响应网络消息
	virtual void OnNetMsg(SOCKET _cSock, DataHeader* header)
	{
		// 6 处理请求
		switch (header->cmd)
		{
			case CMD_LOGIN:
				{
					Login* login = (Login*)header;
					printf("收到客户端<Socket=%d>请求：CMD_LOGIN 数据长度：%d,  userName=%s passWord=%s \n", _cSock, login->dataLength, login->userName, login->passWord);
					//忽略判断用户名密码是否正确的过程
					LoginResult ret;
					//先发消息头再发消息体
					send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
				}
				break;
			case CMD_LOGOUT:
				{
					Logout* logout = (Logout*)header;
					printf("收到客户端<Socket=%d>请求：CMD_LOGOUT 数据长度：%d,  userName=%s\n", _cSock, logout->dataLength, logout->userName);
					//忽略判断用户名密码是否正确的过程
					LogoutResult ret;
					//先发消息头再发消息体
					send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
				}
				break;
			default:
				header->cmd = CMD_ERROR;
				header->dataLength = 0;
				send(_cSock, (char*)&header, sizeof(DataHeader), 0);
				break;
		}
	}

	//发送给指定socket的数据
	int SendData(SOCKET _cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	//向所有socket发送数据
	void SendData2All(DataHeader* header)
	{
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			SendData(_clients[n], header);
		}
	}

};

#endif // _EasyTcpServer_hpp_