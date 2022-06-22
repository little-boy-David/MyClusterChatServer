#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 处理消息事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

// 聊天服务器 业务类 （单例模式）
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService *instance();
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 处理服务端异常退出(采用reset业务重置)
    void serverCloseException();
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

private:
    ChatService(); // 单例模式

    // 存储<消息id, 处理的业务方法> , 服务前全部准备好，不存在线程安全问题；
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接， 存储<消息id, TcpConnectionPtr>，服务中会变化，存在线程安全问题!
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    //定义互斥锁
    mutex _connMutex;

    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offLineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    // redis操作类对象
    Redis _redis;
};

#endif