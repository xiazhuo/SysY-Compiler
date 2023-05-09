#pragma once
#include <unordered_map>
#include <iostream>
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
    unordered_map<string, int> no;      // Sys中的变量名 -> Koopa变量名（后缀）

public:
    KoopaNameManager() : cnt(0) {}
    void reset() {
        cnt = 0;
    };
    // 返回临时变量名，如 %0,%1
    string getTmpName() {
        return "%" + to_string(cnt++);
    }
    // 返回Sysy具名变量在Koopa中的变量名，如 @x,@y,重名时后缀加一，@x_1,@y_1
    string getVarName(const string &s){
        // 若是第一次生成就是@s
        if (!no.count(s))
        {
            no[s] = 0;
            return "@" + s;
        }
        // 后续后缀加一
        return "@" + s + "_" + to_string(++no[s]);
    }

    string getLabelName(const string &s){
        if (!no.count(s))
        {
            no[s] = 1;
            return "%" + s + "_1";
        }
        return "%" + s + "_" + to_string(++no[s]);
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
        SYSY_INT,       // 变量
        SYSY_INT_CONST, // 常量
        SYSY_FUNC_VOID,
        SYSY_FUNC_INT
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
    string name;  // KoopaIR中的具名变量，诸如@x_1, @y_1
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
    unordered_map<string, Symbol *> symbol_tb; // ident -> Symbol *
    SymbolTable() = default;
    ~SymbolTable(){
        for (auto &p : symbol_tb)
        {
            delete p.second;
        }
    };

    void insertINTCONST(const string &ident, const string &name, int value)
    {
        SysYType *ty = new SysYType(SysYType::SYSY_INT_CONST, value);
        Symbol *sym = new Symbol(name, ty);
        symbol_tb.insert({ident,sym});
    }

    void insertINT(const string &ident, const string &name)
    {
        SysYType *ty = new SysYType(SysYType::SYSY_INT);
        Symbol *sym = new Symbol(name, ty);
        symbol_tb.insert({ident, sym});
    }

    void insertFUNC(const string &ident, const string &name, SysYType::TYPE _t){
        SysYType *ty = new SysYType(_t);
        Symbol *sym = new Symbol(name, ty);
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

    string getName(const string &ident){
        return symbol_tb[ident]->name;
    }
};

// 符号表栈（不同作用域中定义的符号在不同的栈中）
class SymbolTableStack
{
private:
    deque<unique_ptr<SymbolTable>> sym_tb_st;
    KoopaNameManager nm;

public:
    void alloc()
    {
        sym_tb_st.emplace_back(new SymbolTable());
    }

    void quit()
    {
        sym_tb_st.pop_back();
    }

    // 每次向栈底的符号表中插入
    void insertINT(const string &ident)
    {
        string name = nm.getVarName(ident);
        sym_tb_st.back()->insertINT(ident, name);
    }

    void insertINTCONST(const string &ident, int value)
    {
        string name = nm.getVarName(ident);
        sym_tb_st.back()->insertINTCONST(ident, name, value);
    }

    void insertFUNC(const string &ident, SysYType::TYPE _t)
    {
        string name = "@" + ident;
        sym_tb_st.back()->insertFUNC(ident, name, _t);
    }

    // 从栈底开始往上依次查找
    bool isExists(const string &ident)
    {
        for (int i = (int)sym_tb_st.size() - 1; i >= 0; --i)
        {
            if (sym_tb_st[i]->isExists(ident))
                return true;
        }
        return false;
    }

    int getValue(const string &ident)
    {
        int i = (int)sym_tb_st.size() - 1;
        for (; i >= 0; --i)
        {
            if (sym_tb_st[i]->isExists(ident))
                break;
        }
        return sym_tb_st[i]->getValue(ident);
    }

    SysYType *getType(const string &ident)
    {
        int i = (int)sym_tb_st.size() - 1;
        for (; i >= 0; --i)
        {
            if (sym_tb_st[i]->isExists(ident))
                break;
        }
        return sym_tb_st[i]->getType(ident);
    }

    string getName(const string &ident)
    {
        int i = (int)sym_tb_st.size() - 1;
        for (; i >= 0; --i)
        {
            if (sym_tb_st[i]->isExists(ident))
                break;
        }
        return sym_tb_st[i]->getName(ident);
    }

    // 封装KoopaNameManager
    void resetNameManager()
    {
        nm.reset();
    }

    string getTmpName()
    {
        return nm.getTmpName();
    }

    string getVarName(const string &ident)
    {
        return nm.getVarName(ident);
    }

    string getLabelName(const string &label_ident)
    {
        return nm.getLabelName(label_ident);
    }
};