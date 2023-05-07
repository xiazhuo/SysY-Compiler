#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;

// 所有类的声明
class BaseAST;
class CompUnitAST;
class DeclAST;
class ConstDeclAST;
class BTypeAST;
class ConstDefAST;
class ConstInitValAST;
class VarDeclAST;
class VarDefAST;
class InitValAST;

class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class BlockItemAST;
class StmtAST;

// Expression
class ExpAST;
class LValAST;
class PrimaryExpAST;
class UnaryExpAST;
class AddExpAST;
class MulExpAST;
class RelExpAST;
class EqExpAST;
class LAndExpAST;
class LOrExpAST;
class ConstExpAST;

// 程序框架的基类
class BaseAST {
  public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
};

class CompUnitAST : public BaseAST {
  public:
  // 用智能指针管理对象
    unique_ptr<BaseAST> func_def;

    void Dump() const override;
};

class FuncDefAST : public BaseAST {
  public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;

    void Dump() const override;
};

class FuncTypeAST : public BaseAST {
    void Dump() const override;
};

class BlockAST : public BaseAST {
  public:
    vector<unique_ptr<BaseAST>> block_items;

    void Dump() const override;
};

class BlockItemAST : public BaseAST
{
  public:
    unique_ptr<BaseAST> decl;
    unique_ptr<BaseAST> stmt;

    void Dump() const override;
};

class DeclAST : public BaseAST
{
  public:
    unique_ptr<BaseAST> const_decl;
    unique_ptr<BaseAST> var_decl;
    void Dump() const override;
};

class ConstDeclAST : public BaseAST
{
  public:
    unique_ptr<BaseAST> btype;
    vector<unique_ptr<BaseAST>> const_defs;
    void Dump() const override;
};

class BTypeAST : public BaseAST
{
    void Dump() const override;
};

class ConstDefAST : public BaseAST
{
  public:
    string ident;
    unique_ptr<ExpAST> const_init_val;
    void Dump() const override;
};

class VarDeclAST : public BaseAST
{
  public:
    unique_ptr<BaseAST> btype;
    vector<unique_ptr<BaseAST>> var_defs;
    void Dump() const override;
};

class VarDefAST : public BaseAST
{
  public:
    string ident;
    unique_ptr<ExpAST> init_val;
    void Dump() const override;
};

class StmtAST : public BaseAST
{
  public:
    enum TAG
    {
      RETURN,
      ASSIGN,
      BLOCK,
      EXP,
      IF,
      WHILE,
      BREAK,
      CONTINUE
    };
    TAG tag;
    unique_ptr<ExpAST> exp;
    unique_ptr<LValAST> lval;
    unique_ptr<BaseAST> block;
    unique_ptr<BaseAST> stmt;
    unique_ptr<BaseAST> if_stmt;
    unique_ptr<BaseAST> else_stmt;

    void Dump() const override;
};


// 所有表达式的基类
class ExpAST {
  public:
    virtual ~ExpAST() = default;

    virtual string Dump() const = 0;    // 返回结果对应的符号
    virtual int getValue() const = 0;   // 返回结果，用于条件判断等
};

class PrimaryExpAST : public ExpAST {
  public:
    int number;
    unique_ptr<ExpAST> exp;
    unique_ptr<LValAST> lval;

    string Dump() const override;
    int getValue() const override;
};

class UnaryExpAST : public ExpAST {
  public:
    string unary_op;
    unique_ptr<ExpAST> primary_exp;
    unique_ptr<ExpAST> unary_exp;

    string Dump() const override;
    int getValue() const override;
};

class MulExpAST : public ExpAST
{
  public:
    string mul_op;
    unique_ptr<ExpAST> unary_exp;
    unique_ptr<ExpAST> mul_exp_1;
    unique_ptr<ExpAST> unary_exp_2;

    string Dump() const override;
    int getValue() const override;
};

class AddExpAST : public ExpAST
{
  public:
    string add_op;
    unique_ptr<ExpAST> mul_exp;
    unique_ptr<ExpAST> add_exp_1;
    unique_ptr<ExpAST> mul_exp_2;

    string Dump() const override;
    int getValue() const override;
};

class RelExpAST : public ExpAST
{
  public:
    string rel_op;
    unique_ptr<ExpAST> add_exp;
    unique_ptr<ExpAST> rel_exp_1;
    unique_ptr<ExpAST> add_exp_2;

    string Dump() const override;
    int getValue() const override;
};

class EqExpAST : public ExpAST
{
  public:
    string eq_op;
    unique_ptr<ExpAST> rel_exp;
    unique_ptr<ExpAST> eq_exp_1;
    unique_ptr<ExpAST> rel_exp_2;

    string Dump() const override;
    int getValue() const override;
};

class LAndExpAST : public ExpAST
{
  public:
    unique_ptr<ExpAST> eq_exp;
    unique_ptr<ExpAST> l_and_exp_1;
    unique_ptr<ExpAST> eq_exp_2;

    string Dump() const override;
    int getValue() const override;
};

class LOrExpAST : public ExpAST
{
  public:
    unique_ptr<ExpAST> l_and_exp;
    unique_ptr<ExpAST> l_or_exp_1;
    unique_ptr<ExpAST> l_and_exp_2;

    string Dump() const override;
    int getValue() const override;
};

class InitValAST : public ExpAST
{
  public:
    unique_ptr<ExpAST> exp;
    string Dump() const override;
    int getValue() const override;
};

class ConstInitValAST : public ExpAST
{
  public:
    unique_ptr<ExpAST> const_exp;

    string Dump() const override { return ""; }
    int getValue() const override;
};

class LValAST
{
  public:
    string ident;
    string Dump(bool dump_ptr = false) const;   // 赋值时store到 @x，计算时load到 %n
    int getValue() const;
};

class ConstExpAST : public ExpAST
{
  public:
    unique_ptr<ExpAST> exp;

    string Dump() const override { return ""; }
    int getValue() const override;
};
