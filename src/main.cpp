#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "koopa.h"
#include "../include/ast.hpp"
#include "../include/util.hpp"

using namespace std;

extern FILE *yyin;
extern KoopaString ks;          // 封装了一个生成 KoopaIR的类
extern RiscvString rvs;         // 封装了一个生成 riscvStr的类
extern int yyparse(unique_ptr<BaseAST> &ast);
extern void yyset_lineno(int _line_number);
extern int yylex_destroy();

extern void koopa_ir_from_str(string);

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
  auto ret = yyparse(ast);
  yylex_destroy();
  assert(!ret);

  // 遍历 AST的同时生成 KoopaIR
  ast->Dump();
  string ir_str = ks.getKoopaIR();

  if (string(mode) == "-koopa")
  {
    cout << ir_str << endl;
    write_file(output, ir_str);
  }
  else if(string(mode) == "-riscv"){
    koopa_ir_from_str(ir_str);
    string riscvstr = rvs.getRiscvStr();
    cout << riscvstr << endl;
    write_file(output, riscvstr);
  }
  return 0;
}
