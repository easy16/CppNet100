#define WIN32_LEAN_AND_MEAN	//避免引用早期的windows库
#include <WinSock2.h>
#include <windows.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <thread>

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

struct LoginResult : public DataHeader
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
bool g_bRun = true;
void cmdThread( SOCKET _sock)
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
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strcpy_s(login.userName, "lyd");
			strcpy_s(login.passWord, "lydmm");
			// 5 向服务器发送请求
			send(_sock, (const char*)&login, sizeof(Login), 0);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout logout;
			strcpy_s(logout.userName, "lyd");
			// 5 向服务器发送请求
			send(_sock, (const char*)&logout, sizeof(Logout), 0);
		}
		else
		{
			printf("不支持的命令，请重新输入。 \n");
		}
	}	
}

int processDeal(SOCKET _cSock)
{
	//缓冲区
	char szRecv[4096] = {};
	// 5 接收客户端数据
	int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)szRecv;
	if (nLen <= 0)
	{
		printf("与服务器断开连接，任务结束。\n");
		return -1;
	}
	// 6 处理请求
	switch (header->cmd)
	{
		case CMD_LOGIN_RESULT:
			{
				recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
				LoginResult* login = (LoginResult*)szRecv;
				printf("收到服务器数据消息：CMD_LOGIN_RESULT 内容：%d \n", login->result);
			}
			break;
		case CMD_LOGOUT_RESULT:
			{
				recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
				LogoutResult* logout = (LogoutResult*)szRecv;
				printf("收到服务器数据消息：CMD_LOGOUT_RESULT 内容：%d \n", logout->result);
			}
			break;
		case CMD_NEW_USER_JOIN:
			{
				recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
				NewUserJoin* userJoin = (NewUserJoin*)szRecv;
				printf("收到服务器数据消息：CMD_NEW_USER_JOIN 内容：%d \n", userJoin->socketID);
			}		
			break;
	}
	return 0;
}


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
	//启动线程
	std::thread t1(cmdThread, _sock);
	t1.detach();//与主线程分离
	while (g_bRun)
	{
		//伯克利socket
		fd_set fdRead;
		//清空数组
		FD_ZERO(&fdRead);
		//赋值
		FD_SET(_sock, &fdRead);
		timeval t = { 1, 0 };
		int ret = select(_sock, &fdRead, 0, 0, &t);
		if (ret < 0)
		{
			printf("select任务结束1。\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);

			if (-1 == processDeal(_sock))
			{
				printf("select任务结束2。\n");
				break;
			}			
		}
		//线程thread
		//printf("空闲时间处理其他业务。。。。\n");
		
		//Sleep(3000);
	}	
	// 7 closesocket 关闭套接字
	closesocket(_sock);
	//--------------
	//清除windows socket环境
	WSACleanup();
	printf("已退出，任务结束。\n");
	getchar();
	return 0;
}