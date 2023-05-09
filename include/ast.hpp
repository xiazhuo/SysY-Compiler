#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;

// 框架类的声明
class BaseAST;
class CompUnitAST;
class FuncDefAST;
class FuncFParamAST;
class BTypeAST;
class BlockAST;
class BlockItemAST;
class StmtAST;

// 定义式类
class DefAST;
class DeclAST;
class ConstDeclAST;
class VarDeclAST;
class ConstDefAST;
class VarDefAST;

class LValAST;

// 表达式类
class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;
class AddExpAST;
class MulExpAST;
class RelExpAST;
class EqExpAST;
class LAndExpAST;
class LOrExpAST;
class ConstExpAST;
class ConstInitValAST;
class InitValAST;

// 程序框架的基类
class BaseAST {
  public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
};

class CompUnitAST : public BaseAST {
  public:
  // 用智能指针管理对象
    vector<unique_ptr<BaseAST>> func_defs;
    vector<unique_ptr<DefAST>> decls;

    void Dump() const override;
};

// 函数定义都在全局作用域内
class FuncDefAST : public BaseAST {
  public:
    unique_ptr<BTypeAST> func_type; // 返回值类型
    string ident;
    vector<unique_ptr<FuncFParamAST>> func_f_params;
    unique_ptr<BaseAST> block;

    void Dump() const override;
};

class FuncFParamAST
{
  public:
    unique_ptr<BaseAST> btype;
    string ident;
};

class BTypeAST : public BaseAST
{
  public:
    enum TAG
    {
      VOID,
      INT
    };
    TAG tag;
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
    unique_ptr<DefAST> decl;
    unique_ptr<BaseAST> stmt;

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


// 定义式的基类
class DefAST
{
  public:
    virtual ~DefAST() = default;

    virtual void Dump(bool is_global = false) const = 0;
};

class DeclAST : public DefAST
{
  public:
    unique_ptr<DefAST> const_decl;
    unique_ptr<DefAST> var_decl;
    void Dump(bool is_global = false) const override;
};

class ConstDeclAST : public DefAST
{
  public:
    unique_ptr<BaseAST> btype;
    vector<unique_ptr<DefAST>> const_defs;
    void Dump(bool is_global = false) const override;
};

class VarDeclAST : public DefAST
{
  public:
    unique_ptr<BaseAST> btype;
    vector<unique_ptr<DefAST>> var_defs;
    void Dump(bool is_global = false) const override;
};

class ConstDefAST : public DefAST
{
  public:
    string ident;
    unique_ptr<ExpAST> const_init_val;
    void Dump(bool is_global = false) const override;
};

class VarDefAST : public DefAST
{
  public:
    string ident;
    unique_ptr<ExpAST> init_val;
    void Dump(bool is_global = false) const override;
};

class LValAST
{
  public:
    string ident;
    string Dump(bool dump_ptr = false) const; // 赋值时store到 @x，计算时load到 %n
    int getValue() const;
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
    string ident;
    vector<unique_ptr<ExpAST>> exps;

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

class ConstExpAST : public ExpAST
{
  public:
    unique_ptr<ExpAST> exp;

    string Dump() const override { return ""; }
    int getValue() const override;
};

class ConstInitValAST : public ExpAST
{
  public:
    unique_ptr<ExpAST> const_exp;

    string Dump() const override { return ""; }
    int getValue() const override;
};

class InitValAST : public ExpAST
{
  public:
    unique_ptr<ExpAST> exp;
    string Dump() const override;
    int getValue() const override;
};
