#include <stdio.h>
#include <thread>

#include "EasyTcpClient.hpp"
bool g_bRun = true;
void cmdThread( /*EasyTcpClient* client*/)
{
	while (true)
	{
		// 3 ��������
		char cmdBuf[256] = {};
		scanf_s("%s", cmdBuf, 256);
		// 4 ��������
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			//client->CloseSocket();
			printf("�˳�cmdThread�̡߳�\n");
			break;
		}
		//else if (0 == strcmp(cmdBuf, "login"))
		//{
		//	Login login;
		//	strcpy_s(login.userName, "lyd");
		//	strcpy_s(login.passWord, "lydmm");
		//	// 5 ���������������
		//	client->SendData(&login);
		//}
		//else if (0 == strcmp(cmdBuf, "logout"))
		//{
		//	Logout logout;
		//	strcpy_s(logout.userName, "lyd");
		//	// 5 ���������������
		//	client->SendData(&logout);
		//}
		else
		{
			printf("��֧�ֵ�������������롣 \n");
		}
	}	
}

int main()
{
	const int cCount = 1000;//windowsĬ�Ͽͻ�������������ȥһ������ˣ��������򲻻ᴫ������
	//��Ҫ��ָ�룬��Ȼջ�ڴ�ᱬ��
	EasyTcpClient* client[cCount];//�����������Ϳ������Ӷ��������
	for (int n = 0; n < cCount; n++)
	{
		if (!g_bRun)//��;�˳���ֹͣ����
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
	
	//�����߳�
	std::thread t1(cmdThread);
	t1.detach();//�����̷߳���

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
	printf("���˳������������\n");
	getchar();
	return 0;
}