#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <queue>
#include <memory>

using namespace std;

// 命名管理器
class NameManager
{
private:
    int cnt;
    unordered_map<string, int> no;

public:
    NameManager() : cnt(0) {}
    void reset() {
        cnt = 0;
    };
    // 返回临时变量名，如 %0,%1
    string getTmpName() {
        return "%" + to_string(cnt++);
    }
};