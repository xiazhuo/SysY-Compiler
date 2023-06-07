#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "koopa.h"
#include "front-end/include/ast.hpp"
#include "util.hpp"

using namespace std;

extern FILE *yyin;
extern KoopaString ks;          // 封装了一个生成 KoopaIR的类
extern RiscvString rvs;         // 封装了一个生成 riscvStr的类
extern int yyparse(unique_ptr<BaseAST> &ast);
extern void yyset_lineno(int _line_number);
extern int yylex_destroy();

extern void Visit(const koopa_raw_program_t &program);

// 向文件中写数据
void write_file(string file_name, string file_content)
{
  ofstream os;                  // 创建一个文件输出流对象
  os.open(file_name, ios::out); // 将对象与文件关联
  os << file_content;           // 将输入的内容放入txt文件中
  os.close();
  return;
}

int main(int argc, const char *argv[])
{
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);

  // parse input file
  yyset_lineno(1);
  unique_ptr<BaseAST> ast;
  auto parse_ret = yyparse(ast);
  yylex_destroy();
  assert(!parse_ret);

  // 遍历 AST的同时生成字符串形式的 KoopaIR
  ast->Dump();
  string ir_str = ks.getKoopaIR();

  // 将字符串形式的IR转化为内存中存储的IR,即下面的 raw
  const char *str = ir_str.c_str();
  // 解析字符串 str, 得到 Koopa IR 程序
  koopa_program_t program;
  koopa_error_code_t convert_ret = koopa_parse_from_string(str, &program);
  assert(convert_ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
  // 创建一个 raw program builder, 用来构建 raw program
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // 将 Koopa IR 程序转换为 raw program
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // 释放 Koopa IR 程序占用的内存
  koopa_delete_program(program);

  // TODO: 处理 raw program,开优化

  if (string(mode) == "-koopa")
  {
    // 将 Koopa IR 程序输出到stdout中
    koopa_generate_raw_to_koopa(&raw, &program);
    koopa_dump_to_stdout(program);
    koopa_delete_program(program);
    write_file(output, ir_str);
  }
  else if(string(mode) == "-riscv"){
    Visit(raw);
    string riscvstr = rvs.getRiscvStr();
    cout << riscvstr << endl;
    write_file(output, riscvstr);
  }

  // 处理完成, 释放 raw program builder 占用的内存
  // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
  // 所以不要在 raw program 处理完毕之前释放 builder
  koopa_delete_raw_program_builder(builder);

  return 0;
}
