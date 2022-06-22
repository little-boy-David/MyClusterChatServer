#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include <vector>
#include <string>
using namespace std;

// offlinemessage表的操作类 => 不需要orm类，因为没有创建取值改值等操作；
class OfflineMsgModel
{
public:
    // 存储用户离线信息
    void insert(int userid, string msg);

    // 删除用户离线信息
    void remove(int userid);

    // 查询用户离线信息
    vector<string> query(int userid);
};

#endif