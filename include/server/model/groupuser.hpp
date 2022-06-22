#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 群组用户的ORM类
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }
private:
    string role; // 存储当前用户的 群角色
};

#endif