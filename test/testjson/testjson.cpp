#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

// json序列化示例1：基础json，底层是采用无序哈希表,即输出结果无序
string func1() {
    json js;
    js["msg_type"] = 2;
    js["from"] = "hjy";
    js["to"] = "lulu";
    js["msg"] = "i love u";
    cout << js << endl;

    string sendBuf = js.dump(); //json数据对象 =》 序列化
    cout << sendBuf << endl;
    return sendBuf;
}

// json序列化示例2：可以放复杂类型
string func2() {
    json js;
    js["id"] = {1, 2, 3};
    js["name"] = "hjy";
    //相当于msg里面还是一个json对象，json里面得key "hjy" 对应 value ”hello lulu“
    js["msg"]["hjy"] = "hello lulu";
    js["msg"]["msg_type"] = "555";
    cout << js << endl;
    // 上面等于下面这句话，一次性添加
    js["msg"] = {{"hjy", "hello lulu"}, {"msg_type", "555"}};
    cout << js << endl;

    return js.dump(); // 返回 json数据对象 =》 序列化
}

// json序列化示例代码3：直接序列化容器
string func3() {
    json js;

    // 添加vetor容器;
    vector<int> vec{1, 2, 3};
    js["list"] = vec;

    // 添加map容器;
    map<int, string> mp;
    mp.insert({1, "系统分析"});
    mp.insert({2, "计算机网络"});
    mp.insert({3, "算法应用"});
    js["考试科目"] = mp;

    string sendBuf = js.dump(); // json数据对象 =》 序列化
    cout << sendBuf << endl;
    return sendBuf;
}

int main() {
    // 反序列化func()1 中的string；
    string recvBuf1 = func1();
    json jsBuf1 = json::parse(recvBuf1); // 接收的stirng =》 反序列化为json对象；
    cout << jsBuf1["msg"] << endl;

    //反序列化 func()2 中的string;
    string recvBuf2 = func2();
    json jsBuf2 = json::parse(recvBuf2); // 接收的stirng =》 反序列化为json对象；
    auto arr = jsBuf2["id"]; // 接收其中的数组类型
    auto msgJson = jsBuf2["msg"]; // 接收其中的msg
    cout << jsBuf2["msg"]["msg_type"] << " " << msgJson["hjy"] << " " <<arr[0] << endl;

    //反序列化 func()3 中的string;
    string recvBuf3 = func3();
    json jsBuf3 = json::parse(recvBuf3); // 接收的stirng =》 反序列化为json对象；
    vector<int> vec = jsBuf3["list"]; // js对象里的数组类型 =》 放到vector容器;
    map<int, string> mp = jsBuf3["考试科目"]; // js对象里的map类型 =》 放到vector容器;
    for (auto &tmp : mp) {
        cout << tmp.first << " " << tmp.second << " ";
    }
    cout << endl;

    return 0;
}