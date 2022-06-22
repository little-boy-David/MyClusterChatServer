#include "groupmodel.hpp"
#include "connectionpool.hpp"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values ('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());
    
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();

    if (mysql->update(sql))
    {
        group.setId(mysql_insert_id(mysql->getConnection()));
        return true;
    }
    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values (%d, %d, '%s')",
            groupid, userid, role.c_str());

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();

    mysql->update(sql);
    
}

// 查询用户的所有群详细信息（包括群成员） =》提供给客户端保存
vector<Group> GroupModel::query(int userid)
{
    /*
     任务一：在groupuser表中查到groupid再联表查询allgroup拿Group的详细信息
     任务二：在groupuser表中查到groupid，需要查询该群的userid，再联表查询user表信息保存到groupuser
    */
    
    // 任务一
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from \
            allgroup a inner join groupuser b on a.id = b.groupid where b.userid = %d",
            userid);
    
    vector<Group> groupVec;
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    
    MYSQL_RES *res = mysql->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            Group group;
            group.setId(atoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);
            groupVec.emplace_back(group);
        }
        mysql_free_result(res);
    }
    

    // 任务二
    for (Group &group: groupVec)
    {
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a inner join \
                groupuser b on b.userid = a.id where b.groupid = %d", group.getId());

        MYSQL_RES *res = mysql->query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser groupUser;
                groupUser.setId(atoi(row[0]));
                groupUser.setName(row[1]);
                groupUser.setState(row[2]);
                groupUser.setRole(row[3]);
                group.getUsers().emplace_back(groupUser);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 查询当前群所有用户（不包括userid自己） =》 满足群发消息业务
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    vector<int> idVec;
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> mysql = cp->getConnection();
    
    MYSQL_RES *res = mysql->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            idVec.emplace_back(atoi(row[0]));
        }
        mysql_free_result(res);
    }
    
    return idVec;
}