#include <iostream>
#include <string>
#include "connectionpool.hpp"
#include "dbpublic.hpp"

// 线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool; // 静态变量实现lock和unlock,拿单例的线程池；
	return &pool;
}

// 从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
	// 易错点，可执行文件是在bin目录下，所以文件路径是以bin目录下为基准；
	FILE* pf = fopen("../src/server/db/connmysql.cnf", "r"); 
	if (pf == nullptr)
	{
		LOG("connmysql.cnf file is not exist!");
		return false;
	}

	while (!feof(pf))//末尾查一下
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0);//找出第一个出现=号的下标
		if (idx == -1)//找不到，无效配置项
		{
			continue;
		}

		//实际中是由\n结尾的，password=root\n
		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx); //参数意义：截取的起点以及截取长度
		string value = str.substr(idx + 1, endidx - idx - 1);

		//存值
		if (key == "ip") _ip = value;
		else if (key == "port") _port = atoi(value.c_str());
		else if (key == "username") _username = value;
		else if (key == "password") _password = value;
		else if (key == "dbname") _dbname = value;
		else if (key == "initSize") _initSize = atoi(value.c_str());
		else if (key == "maxSize") _maxSize = atoi(value.c_str());
		else if (key == "maxIdleTime") _maxIdleTime = atoi(value.c_str());
		else if (key == "connectionTimeout") _connectionTimeout = atoi(value.c_str());

	}
    return true;
}

// 连接池的构造
ConnectionPool::ConnectionPool()
{
	//加载配置项
	if (!loadConfigFile())
	{
		return;
	}

	//创建初始数量的连接
	for (int i = 0; i < _initSize; ++i)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime();// 刷新一下开始空闲起始时间；
		_connectionQue.push(p);
		_connectionCnt++;
	}

	//需要启动一个新线程，作为连接的生产者（生产者线程） 
	//c++的线程函数在linux里面底层也是pthread_creat,需要传入c接口，所以传入类方法需要绑定
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	produce.detach();

	//启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行多余的连接回收
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

//生产者线程：运行独立的线程中，负责生产新连接,放在类内方便访问成员变量
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			cv.wait(lock); //队列不为空，此处生产线程进入等待状态,释放锁
		}

		//可以生产新连接，创建新连接
		if (_connectionCnt < _maxSize) { 
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime();// 刷新一下开始空闲起始时间；		
			_connectionQue.push(p);
			_connectionCnt++;
		}

		//通知消费者线程可以消费连接了
		cv.notify_all();
	}
}

// 消费者线程：给用户连接，从连接池中获取一个可用的空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	
	while (_connectionQue.empty())
	{
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			//是真的超时了， 并且连接池为空
			if (_connectionQue.empty())
			{
				LOG("获取空闲连接超时了!!!获取连接失败");
				return nullptr;
			}			
		}
	}

	//有连接在池子里
	/*
	shared_ptr智能指针析构时，默认会调用connection析构函数，connection就会被close
	这里就需要自定义share_ptr的释放资源方式：把connection直接归还到_connectionQue中；
	*/
	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection* pcon) {
			//这里是在服务器(多线程)消费者线程中调用的，涉及了共享数据，所以一定要考虑队列的线程安全操作
			unique_lock<mutex> lock(_queueMutex);
			pcon->refreshAliveTime();// 刷新一下开始空闲起始时间；
			_connectionQue.push(pcon);
		});
	_connectionQue.pop();
	cv.notify_all();//消费完连接后，通知生产者线程检查线程池是否为空；
	return sp;
}

//回收线程函数：扫描超过maxIdleTime时间的空闲连接，进行多余的连接回收
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		//通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));
	
		// 扫描整个队列，释放多余连接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			//这里都释放？应该释放大于initSize以上的？
			if (p->getAliveTime() > _maxIdleTime * 1000) //##队头的空闲时间是最长的，只用看队头就行
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p;//调用connectin析构函数
			}
			else
			{
				break;//队头都小于，后面肯定小；
			}
		}
	}
}