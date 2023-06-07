#pragma once
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <memory>
#include "koopa.h"

using namespace std;

// Riscv的数据管理器
class RiscvDateManager
{ 
public:
    int register_num;     // 寄存器分配号

    unordered_map<koopa_raw_value_t, int> regmap;    // 当前指令对应的寄存器号

    RiscvDateManager() : register_num(0){}
    ~RiscvDateManager() = default;

    void reset(){       //刚进入函数的时候可以将ir信息恢复为初值
        register_num = 0;
    }
    // 生成一个新的的寄存器
    string getNewReg(){
        string reg;
        if (register_num > 6)
            reg = "a" + to_string(register_num-7);
        else
            reg = "t" + to_string(register_num);
        register_num++;
        return reg;
    }
    // 获取当前指令对应的寄存器,暂时不考虑用完的情况
    string get_reg(const koopa_raw_value_t &value)
    {
        // 当t0~t6用完时,用a0~a7
        if (regmap[value] > 6)
            return "a" + to_string(regmap[value] - 7);
        else
            return "t" + to_string(regmap[value]);
    }
};