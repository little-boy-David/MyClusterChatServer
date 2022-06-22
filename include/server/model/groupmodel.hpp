#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;

//allgroup表、groupuser表的操作类
class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    void addGroup(int userid, int groupid, string role);
    // 查询用户的所有群 =》提供给客户端
    vector<Group> query(int userid);
    // 查询当前群所有用户id（不包括userid自己） =》 满足群发消息业务
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif