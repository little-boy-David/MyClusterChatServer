#include "friendmodel.hpp"
#include "connectionpool.hpp"
#include <stdlib.h>

// 添加好友
void FriendModel::insert(int userid, int friendid)
{
    // 1.组装sql
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    // 2.获取mysql连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();

    // 3.执行sql语句
    mysql->update(sql);
}

// 返回用户好友列表, 采用联表查询，得到User,减少mysql连接的负担；
vector<User> FriendModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select u.id, u.name, u.state from user u inner join friend f on f.friendid = u.id where f.userid = %d", userid);

    vector<User> vec;
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    MYSQL_RES *res = mysql->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            vec.emplace_back(user);
        }
    }
    mysql_free_result(res);
    return vec;
}