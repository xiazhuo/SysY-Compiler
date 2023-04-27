#include <string>
#include <cassert>
#include <iostream>
#include <fstream>
#include <map>
#include "koopa.h"

using namespace std;
typedef unsigned long long ull;

// 重载 Visit，遍历访问每一种 Koopa IR结构
void Visit(const koopa_raw_program_t &program, std::string &inputstr);
void Visit(const koopa_raw_slice_t &slice, std::string &inputstr);
void Visit(const koopa_raw_function_t &func, std::string &inputstr);
void Visit(const koopa_raw_basic_block_t &bb, std::string &inputstr);
void Visit(const koopa_raw_value_t &value, std::string &inputstr);
void Visit(const koopa_raw_return_t &ret, std::string &inputstr);
void Visit(const koopa_raw_integer_t &integer, std::string &inputstr);

int register_num = 0;
map<ull, int> M;

// 找到两个操作数的寄存器
void slice_value(koopa_raw_value_t l, koopa_raw_value_t r, int &lreg, int &rreg)
{

    if (l->kind.tag == KOOPA_RVT_INTEGER)
    {
        if (l->kind.data.integer.value == 0)
            lreg = -1;
        else
        {
            cout << "   li t" << register_num++ << "," << l->kind.data.integer.value << endl;
            lreg = register_num - 1;
        }
    }
    else
        lreg = M[(ull)l];
    if (r->kind.tag == KOOPA_RVT_INTEGER)
    {
        if (r->kind.data.integer.value == 0)
            rreg = -1;
        else
        {
            cout << "   li t" << register_num++ << "," << l->kind.data.integer.value << endl;
            rreg = register_num - 1;
        }
    }
    else
        rreg = M[(ull)r];
}

void print(int lreg, int rreg)
{
    if (lreg == -1)
        cout << "x0";
    else
        cout << "t" << lreg;

    if (rreg == -1)
        cout << ", x0" << endl;
    else
        cout << ", t" << rreg << endl;
}

void koopa_ir_from_str(string irstr, std::string &inputstr)
{
    const char *str = irstr.c_str();
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(str, &program);
    assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    Visit(raw, inputstr);

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}

// 函数声明略
// ...

// 访问 raw program
void Visit(const koopa_raw_program_t &program, std::string &inputstr)
{
    inputstr.append("  .text\n");
    // 访问所有全局变量
    Visit(program.values,inputstr);
    // 访问所有函数
    Visit(program.funcs,inputstr);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice, std::string &inputstr)
{
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr), inputstr);
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), inputstr);
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr), inputstr);
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

// 访问函数
void Visit(const koopa_raw_function_t &func, std::string &inputstr)
{
    string func_name = string(func->name).substr(1);
    inputstr.append("  .globl " + func_name + "\n");
    inputstr.append(func_name + ":\n");
    // 访问所有基本块
    Visit(func->bbs,inputstr);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb, std::string &inputstr)
{
    // inputstr.append()
    // 访问所有指令
    Visit(bb->insts,inputstr);
}

// 访问binary指令
void Visit_binary(const koopa_raw_value_t &value, std::string &inputstr)
{
    // lv4中每个二元指令都能将结果存入，因此每次寄存器都是可以从0开始使用
    register_num = 0; // 刷新寄存器值

    // 根据指令类型判断后续需要如何访问
    const auto &binary = value->kind.data.binary;
    int lreg, rreg;
    // 根据运算符类型判断后续如何翻译
    switch (binary.op)
    {
    case KOOPA_RBO_EQ:
        break;
    case KOOPA_RBO_SUB:
        slice_value(value, value, lreg, rreg);
        cout << "   sub t" << register_num++ << ", ";
        print(lreg, rreg);
        M[(ull)value] = register_num - 1;
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
    return;
}

// 访问指令
void Visit(const koopa_raw_value_t &value, std::string &inputstr)
{
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        Visit(kind.data.ret, inputstr);
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        Visit(kind.data.integer, inputstr);
        break;
    case KOOPA_RVT_BINARY:
        // 访问binary指令
        Visit_binary(value, inputstr);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret, std::string &inputstr)
{
    inputstr.append("  li a0, ");
    Visit(ret.value, inputstr);
    inputstr.append("\n  ret");
}

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &integer, std::string &inputstr)
{
    int32_t intnum = integer.value;
    inputstr.append(to_string(intnum));
    return;
}

