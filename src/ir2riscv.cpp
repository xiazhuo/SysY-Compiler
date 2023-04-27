#include <string>
#include <cassert>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "koopa.h"
#include "../include/util.hpp"
#include "../include/symbol.hpp"

using namespace std;

// 重载 Visit，遍历访问每一种 Koopa IR结构
void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit_binary(const koopa_raw_value_t &value);
void get_left_right_reg(koopa_raw_value_t l, koopa_raw_value_t r, string &lreg, string &rreg);

// 查询 value对应的指令
const char *op2inst[] = {
    "", "", "sgt", "slt", "", "",
    "add", "sub", "mul", "div",
    "rem", "and", "or", "xor",
    "sll", "srl", "sra"};

RiscvString rvs;
RiscvDateManager dm;

void koopa_ir_from_str(string irstr)
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
    Visit(raw);

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}

// 函数声明略
// ...

// 访问 raw program
void Visit(const koopa_raw_program_t &program)
{
    // 访问所有全局变量
    Visit(program.values);
    // 访问所有函数
    Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice)
{
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

// 访问函数
void Visit(const koopa_raw_function_t &func)
{
    dm.reset();
    string func_name = string(func->name).substr(1);
    rvs.append("  .text\n");
    rvs.append("  .globl " + func_name + "\n");
    rvs.append(func_name + ":\n");
    // 访问所有基本块
    Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb)
{
    // inputstr.append()
    // 访问所有指令
    Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value)
{
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        Visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        Visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        Visit_binary(value);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret)
{
    if(ret.value){
        koopa_raw_value_t ret_value = ret.value;
        // 特判return为一个整数情况
        if (ret_value->kind.tag == KOOPA_RVT_INTEGER)
        {
            rvs.append("  li\ta0, ");
            Visit(ret_value->kind.data.integer);
            rvs.append("\n");
        }
        else
        { // value指向了上一条指令的结果寄存器
            string reg = dm.get_reg(ret_value);
            rvs.two("mv", "a0", reg);
        }
    }
    rvs.ret();
}

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &integer)
{
    rvs.append(to_string(integer.value));
}

// 访问binary指令
void Visit_binary(const koopa_raw_value_t &value)
{
    koopa_raw_binary_t binary = value->kind.data.binary;
    string lreg, rreg;
    get_left_right_reg(binary.lhs, binary.rhs, lreg, rreg);
    string ans = dm.getNewReg();
    dm.regmap[value] = dm.register_num - 1;
    // 根据运算符类型判断后续如何翻译
    switch (binary.op)
    {
    case KOOPA_RBO_NOT_EQ:
        rvs.binary("xor", ans, lreg, rreg);
        rvs.two("snez", ans, ans);
        break;
    case KOOPA_RBO_EQ:
        rvs.binary("xor", ans, lreg, rreg);
        rvs.two("seqz", ans, ans);
        break;
    case KOOPA_RBO_GE:
        rvs.binary("slt", ans, lreg, rreg);
        rvs.two("seqz", ans, ans);
        break;
    case KOOPA_RBO_LE:
        rvs.binary("sgt", ans, lreg, rreg);
        rvs.two("seqz", ans, ans);
        break;
    default:
        string op = op2inst[(int)binary.op];
        rvs.binary(op, ans, lreg, rreg);
        break;
    }
    return;
}

// 找到操作数对应的寄存器
void get_left_right_reg(koopa_raw_value_t l, koopa_raw_value_t r, string &lreg, string &rreg)
{
    int cnt = 0;
    if (l->kind.tag == KOOPA_RVT_INTEGER)
    {
        if (l->kind.data.integer.value == 0)
            lreg = "x0";
        else
        {
            lreg = dm.getNewReg();
            cnt++;
            rvs.li(lreg, l->kind.data.integer.value);
        }
    }
    else
        lreg = dm.get_reg(l);
    if (r->kind.tag == KOOPA_RVT_INTEGER)
    {
        if (r->kind.data.integer.value == 0)
            rreg = "x0";
        else
        {
            rreg = dm.getNewReg();
            cnt++;
            rvs.li(rreg, r->kind.data.integer.value);
        }
    }
    else
        rreg = dm.get_reg(r);
    dm.register_num -= cnt;
    return;
}