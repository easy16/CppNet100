//跨平台头文件
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN	//避免引用早期的windows库
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
#include <WS2tcpip.h>
#include <vector>
//要求c\s字节序一致、对齐
enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};
struct DataHeader
{
	short dataLength;
	short cmd;
};
//DataPackage继承的方式在构造函数中初始化
struct Login : public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char passWord[32];
};

struct LoginResult	: public DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};

struct Logout : public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};

struct LogoutResult : public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};

struct NewUserJoin : public DataHeader
{
	NewUserJoin()
	{
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		socketID = 0;
	}
	int socketID;
};

std::vector<SOCKET> g_clients;

int processDeal(SOCKET _cSock)
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
	// 6 处理请求
	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		Login login = {};
		recv(_cSock, (char*)&login + sizeof(DataHeader), sizeof(Login) - sizeof(DataHeader), 0);
		printf("收到客户端<Socket=%d>请求：CMD_LOGIN 数据长度：%d,  userName=%s passWord=%s \n", _cSock, login.dataLength, login.userName, login.passWord);
		//忽略判断用户名密码是否正确的过程
		LoginResult ret;
		//先发消息头再发消息体
		send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
	}
	break;
	case CMD_LOGOUT:
	{
		Logout logout = {};
		recv(_cSock, (char*)&logout + sizeof(DataHeader), sizeof(Logout) - sizeof(DataHeader), 0);
		printf("收到客户端<Socket=%d>请求：CMD_LOGOUT 数据长度：%d,  userName=%s\n", _cSock, logout.dataLength, logout.userName);
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
	return 0;
}
int g_port = 4567;
int main()
{
	g_clients.clear();
#ifdef _WIN32
	//启动windows socket 2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
#endif
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
	_sin.sin_port = htons(g_port);//防止主机中的short类型与网络字节序中的不同
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	_sin.sin_addr.s_addr = INADDR_ANY; 
#endif
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))
	{
		printf("错误，绑定网络端口失败。。。\n");
	}
	else {
		printf("绑定网络端口%d成功。。。\n", g_port);
	}
	// 3 listen 监听网络端口
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("错误，监听网络端口%d失败。。。\n", g_port);
	}
	else {
		printf("监听网络端口%d成功。。。\n", g_port);
	}
	
	while (true)//循环重复接收新的客户端（accept在循环里时）/客户端指令
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
		for (int n = (int)g_clients.size()-1; n >= 0; n--)
		{
			//将客户端socket加入集合
			FD_SET(g_clients[n], &fdRead);
			if (maxSock < g_clients[n])
			{
				maxSock = g_clients[n];
			}
		}

		//nfds 是一个整数值 是指fd_set集合中所有描述符（socket）的范围，而不是数量，
		//既是所有文件描述符最大值+1 在windows中这个参数可以写0
		//select最后一个参数是null，是阻塞模式（有数据可操作的时候才返回），纯接收数据的服务可以接受
		timeval t = {1, 0};//查询时间为1 最大查询时间为1并非等待1s 非阻塞网络模型  综合性网络程序
		int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
		if (ret < 0)
		{
			printf("select任务结束。\n");
			break;
		}
		//判断描述符是否在集合中
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
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
				printf("错误，接收到无效的客户端SOCKET。。。\n");
			}
			else
			{
				for (int n = (int)g_clients.size() - 1; n >= 0; n--)
				{
					NewUserJoin userJoin;
					userJoin.socketID = _cSock;
					send(g_clients[n], (const char*)&userJoin, sizeof(NewUserJoin), 0);
				}
				char sendBuf[20] = { '\0' };
				printf("新客户端加入：socket = %d, IP = %s \n", (int)_cSock, inet_ntop(AF_INET, (void*)&clientAddr.sin_addr, sendBuf, 16));
				g_clients.push_back(_cSock);
			}			
		}
		for (int n = (int)g_clients.size() - 1; n >= 0; n--)
		{
			if (FD_ISSET(g_clients[n], &fdRead))
			{
				if (-1 == processDeal(g_clients[n]))
				{ 
					//客户端退出在客户端集合中删除客户端
					auto iter = g_clients.begin() + n;
					if (iter != g_clients.end())
					{
						g_clients.erase(iter);
					}
				}
			}
		}
		//for (size_t n = 0; n < fdRead.fd_count; n++) 
		//{//处理集合中有数据请求的socket
		//	if (-1 == processDeal(fdRead.fd_array[n]))
		//	{
		//		auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[n]);
		//		if (iter != g_clients.end())
		//		{
		//			g_clients.erase(iter);
		//		}
		//	}
		//}
		printf("空闲时间处理其他业务。。。。\n");
	}
#ifdef _WIN32
	for (int n = (int)g_clients.size() - 1; n >= 0; n--)
	{
		closesocket(g_clients[n]);
	}
	// 8 closesocket 关闭套接字
	closesocket(_sock);
	//--------------
	//清除windows socket环境
	WSACleanup();
#else
	for (int n = (int)g_clients.size() - 1; n >= 0; n--)
	{
		close(g_clients[n]);
	}
	// 8 closesocket 关闭套接字
	close(_sock);
#endif
	printf("已退出，任务结束。\n");
	getchar();
	return 0;
}