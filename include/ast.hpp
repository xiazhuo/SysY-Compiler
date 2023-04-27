#pragma once
#include <iostream>
#include <memory>
#include <string>

using namespace std;

// 所有类的声明
class BaseAST;
class CompUnitAST;
class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class StmtAST;

// Expression
class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;
class AddExpAST;
class MulExpAST;
class RelExpAST;
class EqExpAST;
class LAndExpAST;
class LOrExpAST;

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
    unique_ptr<BaseAST> stmt;

    void Dump() const override;
};

class StmtAST : public BaseAST
{
  public:
    unique_ptr<ExpAST> exp;

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