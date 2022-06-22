/*
muduo网络库主要提供了两个主要类
TCPServer: 用于编写服务器程序
TCPClient: 用于编写客户端程序

网络库： 封装 epoll + 线程池
好处： 将网络I/O代码和业务代码分开
业务代码暴露（我们关心）：用户连接和断开； 用户的可读事件  =》 其它交给网络库！
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/* 基于muduo网络库开发服务器程序
1. 组合TcpServer对象 ： 封装了网络底层listen， accept等操作；
2. 创建EventLoop事件循环对象的指针 ：事件发生进行提醒；
3. 明确TcpServer构造函数需要上面参数， 输出ChatServer的构造函数；
4. 在当前服务器类的构造函数中，注册连接处理的回调函数和 读写事件处理的回调函数；
5. 设置何是的服务端线程数量， muduo库会自己划分I/O线程和worker线程； 
*/

class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP + Port
               const string &nameArg)         // 服务器名字
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 服务器注册用户连接的创建和断开 回调 => 即用户创建和断开时会发生的事情
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); //绑定this指针，将原本的那一个参数给个参数占位符

        // 服务器注册用户读写事件 回调 => 即用户做出读写事件时会发生的事情
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务端的线程数量， 这里设置成四个，1个I/O线程，3个worker线程
        _server.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 专门响应处理用户的连接创建和断开(已经封装好了epoll + listend + accept等), 开发业务使用
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state: on" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state: off" << endl;
            conn->shutdown(); // close(fd);释放资源(socket)
            // _loop->quit(); // 服务器结束
        }
    }

    // 专门响应用户读写事件  缓冲区=》提高效率
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,                  // 缓冲区
                   Timestamp time)               // 接收到的数据的时间信息
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << " time:" << time.toString() << endl;
        buf += " give u";
        conn->send(buf);
    }

    TcpServer _server; // #1
    EventLoop *_loop;  // #2
};

int main() {
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listened epoll_ctl => epoll;
    loop.loop(); // epoll_wait 以阻塞方式等待新用户连接，已连接用户的读写事件等；

    return 0;

}