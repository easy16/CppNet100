#include <iostream>
#include <thread>
#include <mutex>//锁
#include "CELLTimestamp.hpp"
using namespace std;

mutex m;
const int tCount = 4;
int sum = 0;
//说明：主线程和子线程都调用了cout即使有换行，因为没有锁定调用还是有时会并做一行
//锁：不应该频繁使用；合理锁定区域
void workFun(int index)
{	
	for (int n = 0; n < 20000; n++)
	{
		m.lock();//只锁定公共资源，一定要合理
		//临街区域-开始
		sum++;
		//临街区域-结束
		m.unlock();		
	}	
	//cout << index << "Hello,workFun thread." << n << endl;
}//抢占式

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
		//t[n].detach();//启动线程
		t[n].join();//启动线程
	}
	cout << tTime.getElapsedTimeInMilliSec()<<",sum=" << sum << endl;
	cout << "Hello,main thread." << endl;
	return 0;
}