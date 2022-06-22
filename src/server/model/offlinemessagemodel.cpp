#include "offlinemessagemodel.hpp"
#include "connectionpool.hpp"

// 存储用户离线信息
void OfflineMsgModel::insert(int userid, string msg)
{
    // 1.组装mysql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());

    // 2.获取数据库连接更新
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    mysql->update(sql);
}

// 删除用户离线信息
void OfflineMsgModel::remove(int userid)
{
    // 1.组装mysql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid);

    // 2.连接，删除
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    mysql->update(sql);
}

// 查询用户离线信息
vector<string> OfflineMsgModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    // 2.连接查询获取结果
    vector<string> vec;
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    
    MYSQL_RES *res = mysql->query(sql); //调用mysql查询，MYSQL_RES资源结构体
    if (res != nullptr)
    {
        MYSQL_ROW row; // 获取一行结果的数组；
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            vec.emplace_back(row[0]); //放message进入数组
        }
        mysql_free_result(res); //释放资源
    }
    
    return vec;
}