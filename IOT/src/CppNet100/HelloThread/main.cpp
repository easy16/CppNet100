#include <iostream>
#include <thread>
#include <mutex>//��
#include "CELLTimestamp.hpp"
using namespace std;

mutex m;
const int tCount = 4;
int sum = 0;
//˵�������̺߳����̶߳�������cout��ʹ�л��У���Ϊû���������û�����ʱ�Ტ��һ��
//������Ӧ��Ƶ��ʹ�ã�������������
void workFun(int index)
{	
	for (int n = 0; n < 20000; n++)
	{
		m.lock();//ֻ����������Դ��һ��Ҫ����
		//�ٽ�����-��ʼ
		sum++;
		//�ٽ�����-����
		m.unlock();		
	}	
	//cout << index << "Hello,workFun thread." << n << endl;
}//��ռʽ

int main()
{
	thread t[tCount];
	for (int n = 0; n < tCount; n++)
	{
		t[n] = thread(workFun, n);
	}
	CELLTimestamp tTime;
	for (int n = 0; n < tCount; n++)
	{
		//t[n].detach();//�����߳�
		t[n].join();//�����߳�
	}
	cout << tTime.getElapsedTimeInMilliSec()<<",sum=" << sum << endl;
	cout << "Hello,main thread." << endl;
	return 0;
}