#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <queue>
#include <memory>
#include "koopa.h"

using namespace std;

// KoopaIR的命名管理器
class KoopaNameManager
{
private:
    int cnt;
    unordered_map<string, int> no;      // Koopa中的变量名 -> 值

public:
    KoopaNameManager() : cnt(0) {}
    void reset() {
        cnt = 0;
    };
    // 返回临时变量名，如 %0,%1
    string getTmpName() {
        return "%" + to_string(cnt++);
    }
    // 返回Sysy具名变量在Koopa中的变量名，如 @x,@y,(暂不考虑重名)
    string getName(const string &s){
        return "@" + s;
    }
};

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

// 符号类型
class SysYType
{
public:
    enum TYPE
    {
        SYSY_INT,           // 变量
        SYSY_INT_CONST,     // 常量
    };

    TYPE ty;
    int value;

    SysYType() : ty(SYSY_INT), value(0){}
    SysYType(TYPE _t) : ty(_t), value(0){}
    SysYType(TYPE _t, int _v) : ty(_t), value(_v){}

    ~SysYType() = default;
};

// 符号
class Symbol
{
public:
    string name;  // KoopaIR中的具名变量，诸如@x, @y
    SysYType *ty;
    Symbol(const string &_name, SysYType *_t) : name(_name), ty(_t){}
    ~Symbol()
    {
        if (ty)
            delete ty;
    }
};

// 符号表
class SymbolTable
{
public:
    const int UNKNOWN = -1;
    unordered_map<string, Symbol *> symbol_tb; // name -> Symbol *
    SymbolTable() = default;
    ~SymbolTable(){
        for (auto &p : symbol_tb)
        {
            delete p.second;
        }
    };

    void insertINTCONST(const string &ident, int value){
        SysYType *ty = new SysYType(SysYType::SYSY_INT_CONST, value);
        Symbol *sym = new Symbol(ident, ty);
        symbol_tb.insert({ident,sym});
    }

    void insertINT(const string &ident){
        SysYType *ty = new SysYType(SysYType::SYSY_INT);
        Symbol *sym = new Symbol(ident, ty);
        symbol_tb.insert({ident, sym});
    }

    bool isExists(const string &ident){
        return symbol_tb.find(ident) != symbol_tb.end();
    }

    int getValue(const string &ident){
        return symbol_tb[ident]->ty->value;
    }

    SysYType* getType(const string &ident)
    {
        return symbol_tb[ident]->ty;
    }
};