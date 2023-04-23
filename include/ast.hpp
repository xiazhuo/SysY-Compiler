#pragma once
#include <iostream>

// 所有 AST 的基类
class BaseAST {
  public:
    virtual ~BaseAST() = default;

    virtual void Dump(std::string &inputstr) const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
  public:
  // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void Dump(std::string &inputstr) const override
    {
      func_def->Dump(inputstr);
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump(std::string &inputstr) const override
    {
      inputstr += "fun @" + ident + " (): ";
      func_type->Dump(inputstr);
      inputstr.append("{\n");
      block->Dump(inputstr);
      inputstr.append("}");
    }
};

class FuncTypeAST : public BaseAST {
    void Dump(std::string &inputstr) const override
    {
      inputstr.append("i32");
    }
};

class BlockAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> stmt;

    void Dump(std::string &inputstr) const override
    {
      inputstr.append("%entry: \n");
      stmt->Dump(inputstr);
      inputstr.append("\n");
    }
};

class StmtAST : public BaseAST {
  public:
    std::string number;
    void Dump(std::string &inputstr) const override
    {
      inputstr.append("  ret "+number);
    }
};