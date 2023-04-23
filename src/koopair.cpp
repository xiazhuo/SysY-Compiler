#include <string>
#include <cassert>
#include <iostream>
#include <fstream>
#include "../include/koopair.hpp"
#include "koopa.h"

using namespace std;

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