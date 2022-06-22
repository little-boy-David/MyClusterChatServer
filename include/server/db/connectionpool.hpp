#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include "connection.hpp"

using namespace std;
/*
* 实现连接池模块
*/
class ConnectionPool
{
public:
	//获取连接池对象实例
	static ConnectionPool* getConnectionPool();
	// 消费者线程函数：给用户连接，归还时放回连接池
	shared_ptr<Connection> getConnection();

private:
	//单例#1 构造函数私有化
	ConnectionPool(); 

	//从配置文件中加载配置
	bool loadConfigFile();

	//生产者线程函数：运行独立的线程中，负责生产新连接,放在类内方便访问成员变量
	void produceConnectionTask();

	//回收线程函数：扫描超过maxIdleTime时间的空闲连接，进行多余的连接回收
	void scannerConnectionTask();

	string _ip;// mysql ip
	unsigned short _port; //mysql 端口 默认3306
	string _username;// mysql 用户名；
	string _password;// mysql登录秘密
	string _dbname; // 数据库名称
	int _initSize;// 连接池的初始连接量
	int _maxSize;// 连接池的最大连接量
	int _maxIdleTime; //连接池最大空闲时间
	int _connectionTimeout;//连接池获取连接的超时时间

	queue<Connection*> _connectionQue; //存储mysql连接的队列，必须是线程安全的；
	mutex _queueMutex;//维护连接队列的线程安全互斥锁
	atomic_int _connectionCnt;// 记录连接所创建的connection连接的总数量，考虑了连接生产消费数量变化的线程安全问题
	condition_variable cv; //设置条件变量，用于连接 生产线程和消费线程的通信
};

#endif 