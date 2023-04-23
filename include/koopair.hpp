#pragma once
#include "koopa.h"
#include <string>

void koopa_ir_from_str(std::string irstr, std::string &inputstr);

// 重载 Visit，遍历访问每一种 Koopa IR结构
void Visit(const koopa_raw_program_t &program, std::string &inputstr);
void Visit(const koopa_raw_slice_t &slice, std::string &inputstr);
void Visit(const koopa_raw_function_t &func, std::string &inputstr);
void Visit(const koopa_raw_basic_block_t &bb, std::string &inputstr);
void Visit(const koopa_raw_value_t &value, std::string &inputstr);
void Visit(const koopa_raw_return_t &ret, std::string &inputstr);
void Visit(const koopa_raw_integer_t &integer, std::string &inputstr);