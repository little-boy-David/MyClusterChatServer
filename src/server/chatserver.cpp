#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <functional>
#include <iostream>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 服务器注册用户连接的创建和断开 回调 => 即用户创建和断开时会发生的事情
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); //绑定this指针，将原本的那一个参数给个参数占位符

    // 服务器注册用户读写事件 回调 => 即用户做出读写事件时会发生的事情
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置服务端的线程数量， 这里设置成四个，1个I/O线程，3个worker线程
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 连接相关的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端异常断开连接
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 读写事件相关的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 反序列化
    json js = json::parse(buf);
    // 解耦网络和业务模块：做一个回调函数，通过js["msgid"] =》业务handler（在业务模块事先绑定好的） =》传送conn js time等过去
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，执行相应的业务, 这样拆离就可以网络层和业务层分离！
    msgHandler(conn, js, time);
}