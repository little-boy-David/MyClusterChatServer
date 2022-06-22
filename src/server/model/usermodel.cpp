#include "usermodel.hpp"
#include "connectionpool.hpp"
#include <iostream>
using namespace std;

// user表的插入(增加)方法
bool UserModel::insert(User &user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')", // sprintf 向字符串中输入
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    // 2.执行sql语句
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();         // 获取数据库连接
    
    if (mysql->update(sql))
    {
        user.setId(mysql_insert_id(mysql->getConnection())); // mysql_insert_id返回最后一次操作的列的id值
        return true;
    }
    
    return false;
}

//根据用户id查询用户信息
User UserModel::query(int id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    // 2.执行sql语句
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    
    MYSQL_RES *res = mysql->query(sql); //调用mysql数据库的查询
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res); //获取行，用主键查一行
        if (row != nullptr)
        {
            User user;
            user.setId(atoi(row[0])); //将字符串 转换成 int
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);
            mysql_free_result(res); // 记得释放资源！
            return user;
        }
    }
    

    return User(); // 查询不到返回user中id = -1的对象
}

//更新用户的状态信息
bool UserModel::updateState(User user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    
    if (mysql->update(sql))
    {
        return true;
    }
    

    return false;
}

//重置用户的状态信息
void UserModel::resetState()
{
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    mysql->update(sql);
}

//更新用户所有信息
bool UserModel::updateUser(User user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set name = '%s', password = '%s' where id = '%d'", user.getName().c_str(), user.getPwd().c_str(), user.getId());

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    
    if (mysql->update(sql))
    {
        return true;
    }
    

    return false;
}