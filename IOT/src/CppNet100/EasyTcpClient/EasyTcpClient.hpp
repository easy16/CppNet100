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
private:
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
		else
		{
			printf("建立socket=<%d>成功。。。\n", _sock);
		}
		return 0;
	}
	//连接服务器
	void ConnectServer(const char* ip, unsigned short port)
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
		printf("<socket=%d>正在连接服务器<%s:%d>。。。\n", (int)_sock, ip, port);
		if (SOCKET_ERROR == ret)
		{
			printf("<socket=%d>错误，连接服务器<%s:%d>失败。。。\n", (int)_sock, ip, port);
		}
		else {
			printf("<socket=%d>连接服务器<%s:%d>成功。。。\n", (int)_sock, ip, port);
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
			timeval t = { 0, 0 };
			//其他平台需要sock+1，否则收不到服务端消息
			int ret = select(_sock + 1, &fdRead, 0, 0, &t);
			if (ret < 0)
			{
				printf("<socket=%d>select任务结束1。\n", (int)_sock);
				CloseSocket();//不关闭连接 onRun 会重复提醒
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);

				if (-1 == RecvData(_sock))
				{
					printf("<socket=%d>select任务结束2。\n", (int)_sock);
					CloseSocket();
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

	//临时的成员变量
	//缓冲区最小单元大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif

	//接收缓冲区 成员变量最好在构造函数中初始化
	char _szRecv[RECV_BUFF_SIZE] = {};
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE*10] = {};
	//消息缓冲区的数据尾部位置
	int _lastPos = 0;
	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET _cSock)
	{
		// 5 接收数据
		int nLen = (int)recv(_cSock, _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("<socket=%d>与服务器断开连接，任务结束。\n", (int)_sock);
			return -1;
		}
		//将收取的数据拷贝到消息缓冲区
		memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		_lastPos += nLen;
		//判断消息缓冲区的数据长度是否大于消息头DataHeader长度		
		while (_lastPos >= sizeof(DataHeader))//循环解决粘包
		{
			//这是就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)_szMsgBuf;
			//判断消息缓冲区的数据长度大于消息长度		
			if (_lastPos >= header->dataLength)//判断解决少包
			{
				//消息缓冲区剩余未处理数据的长度   需要提前保存下来
				int nSize = _lastPos - header->dataLength;//从接受缓冲区多取过来的一部分
				//处理网络消息
				OnNetMsg(header);//header被处理过后其中header被强制转换已经改变
				//将消息缓冲区剩余未处理数据前移
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
				//消息缓冲区的数据尾部位置前移
				_lastPos = nSize;
			}
			else
			{
				//消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}
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
					//printf("<socket=%d>收到服务器数据消息：CMD_LOGIN_RESULT 内容：%d \n", (int)_sock, login->result);
				}
				break;
			case CMD_LOGOUT_RESULT:
				{
					LogoutResult* logout = (LogoutResult*)header;
					//printf("<socket=%d>收到服务器数据消息：CMD_LOGOUT_RESULT 内容：%d \n", (int)_sock, logout->result);
				}
				break;
			case CMD_NEW_USER_JOIN:
				{
					NewUserJoin* userJoin = (NewUserJoin*)header;
					//printf("<socket=%d>收到服务器数据消息：CMD_NEW_USER_JOIN 内容：%d \n", (int)_sock, userJoin->socketID);
				}
				break;
			case CMD_ERROR:
				{
					printf("<socket=%d>收到服务器数据消息：CMD_ERROR，数据长度：%d \n", (int)_sock, header->dataLength);
				}
				break;
			default:
				{
					printf("<socket=%d>收到未定义消息，数据长度：%d \n", (int)_sock, header->dataLength);
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