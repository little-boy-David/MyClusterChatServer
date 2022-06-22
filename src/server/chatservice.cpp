#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <iostream>
using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    // 连接redis服务器
    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回默认的空操作处理器，同时，记录错误日志：msgid 没有对应的事件处理回调
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    // 返回存在的msgid处理器
    return _msgHandlerMap[msgid];
}

// 处理注册业务 =》 读取name password字段 写入数据库
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 获取名字、密码
    string name = js["name"];
    string pwd = js["password"];

    // 创建承接的用户对象，进行数据库插入
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state) // 数据插入成功
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump()); //回调，返回json字符串
    }
    else // 数据插入失败
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["msg"] = "注册失败";
        conn->send(response.dump()); //回调，返回json字符串
    }
}

// 处理登录业务 =》通过id pwd 检测pwd是否正确；
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>(); // 获取id号
    string pwd = js["password"];  // 获取密码

    User user = _userModel.query(id);               // 通过id查询；
    if (user.getId() == id && user.getPwd() == pwd) // 成功匹配
    {
        if (user.getState() == "online") // 用户已经登录，不允许再登录
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        }
        else //用户未登录 =》登录成功，修改登录状态
        {
            // 记录用户连接信息，记录在线用户，考虑线程安全问题
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id); 

            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询是否有离线消息
            vector<string> vec = _offLineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;      //json库可以和容器之间序列化和反序列化
                _offLineMsgModel.remove(id); //删除所有该id的离线消息
            }

            // 查询好友信息
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> strVec;
                for (User &it : userVec)
                {
                    json js;
                    js["id"] = it.getId();
                    js["name"] = it.getName();
                    js["state"] = it.getState();
                    strVec.emplace_back(js.dump());
                }
                response["friends"] = strVec;
            }

            //查询用户的群组消息
            vector<Group> groupVec = _groupModel.query(id);
            if (!groupVec.empty())
            {
                vector<string> groupDetail;
                for (Group &group : groupVec)
                {
                    // 格式：groupjson:[{grouppid:xxx, groupname:xxx,groupdesc:xxx, users:{...}}, {}, {},...]
                    json groupJson;
                    groupJson["id"] = group.getId();
                    groupJson["groupname"] = group.getName();
                    groupJson["groupdesc"] = group.getDesc();
                    vector<string> userDetail;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userDetail.emplace_back(js.dump());
                    }
                    groupJson["users"] = userDetail;
                    groupDetail.emplace_back(groupJson.dump());
                }
                response["groups"] = groupDetail;
            }
            conn->send(response.dump());
        }
    }
    else // 登录失败，用户不存在或者密码错误，这里就不分开写了
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

//处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid); 

    //这里采用更新用户的方法 =》从而保存用户信息
    User user(userid, "", "", "offline");
    _userModel.updateUser(user);
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>(); //获取接受信息的userid；

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid); //查询是否对方在线
        if (it != _userConnMap.end())      //在线即发送消息
        {
            it->second->send(js.dump()); //服务器直接转发
            return;
        }
    }

    //用户不在线，存储离线消息
    _offLineMsgModel.insert(toid, js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 获取当前用户和添加好友的id
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友
    _friendModel.insert(userid, friendid);
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 创建群，存储创建人信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> userIdVec = _groupModel.queryGroupUsers(userid, groupid);

    // 给每个用户发送消息
    lock_guard<mutex> lock(_connMutex);
    for (auto id : userIdVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            _offLineMsgModel.insert(id, js.dump());
        }
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                // 从map表中删除用户在线状态
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId()); 

    // 更新用户状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 处理服务端异常退出(采用reset业务重置)
void ChatService::serverCloseException()
{
    // 把online状态的用户设置为offline
    _userModel.resetState();
}

//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 用户不存在，存储用户的离线消息
    _offLineMsgModel.insert(userid, msg);
}