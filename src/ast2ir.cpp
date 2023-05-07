#include "../include/ast.hpp"
#include "../include/symbol.hpp"
#include "../include/util.hpp"
#include <iostream>

using namespace std;

SymbolTableStack st;
KoopaString ks;
BlockController bc;

void CompUnitAST::Dump() const {
    st.alloc();     // 全局作用域栈
    func_def->Dump();
    st.quit();
    return;
}

void FuncDefAST::Dump() const {
    st.resetNameManager();
    ks.append("fun @" + ident + " (): ");
    func_type->Dump();
    ks.append(" {\n");
    bc.set();           // 函数是一个基本块
    ks.label("%entry");
    block->Dump();
    // 特判空块
    if (bc.alive())
    {
        ks.ret("0");
        bc.finish();
    }
    ks.append("}");
    return;
}

void FuncTypeAST::Dump() const {
    ks.append("i32");
    return;
}

void BlockAST::Dump() const {
    st.alloc();
    int len = block_items.size();

    // 注意识别的时候从后往前
    for (int i = len - 1; i >= 0; i--)
    {
        block_items[i]->Dump();
    }
    st.quit();
    return;
}

void BlockItemAST::Dump() const
{
    // 若已存在跳转指令则不执行后面的语句
    if(!bc.alive())
        return;
    if (decl)
        decl->Dump();
    else
        stmt->Dump();
}

void DeclAST::Dump() const
{
    if (var_decl)
        var_decl->Dump();
    else
        const_decl->Dump();
}

void ConstDeclAST::Dump() const
{
    int len = const_defs.size();
    for (int i = len - 1; i >= 0; i--)
    {
        const_defs[i]->Dump();
    }
}

void VarDeclAST::Dump() const
{
    int len = var_defs.size();
    for (int i = len - 1; i >= 0; i--)
    {
        var_defs[i]->Dump();
    }
}

void BTypeAST::Dump() const
{
    ks.append("i32");
}

void ConstDefAST::Dump() const
{
    int v = const_init_val->getValue();
    st.insertINTCONST(ident, v);
}

void VarDefAST::Dump() const
{
    st.insertINT(ident);
    string name = st.getName(ident);
    ks.alloc(name);
    if (init_val)
    {
        string s = init_val->Dump();
        ks.store(s, name);
    }
    return;
}

void StmtAST::Dump() const {
    // 若已存在跳转指令则不执行后面的语句
    if (!bc.alive())
        return;
    if (tag == RETURN)
    {
        if (exp)
        {
            string name = exp->Dump();
            ks.ret(name);
        }
        else
        {
            ks.ret("");
        }
        bc.finish();        // return语句之后的语句不再执行
    }
    else if (tag == ASSIGN)
    {
        string val = exp->Dump();
        string to = lval->Dump(true);
        ks.store(val, to);
    }
    else if (tag == BLOCK)
    {
        block->Dump();
    }
    else if (tag == EXP)
    {
        if (exp)
        {
            exp->Dump();
        }
    }
    else if (tag == IF)
    {
        string s = exp->Dump();
        string t = st.getLabelName("then");
        string e = st.getLabelName("else");
        string j = st.getLabelName("end");
        ks.br(s, t, else_stmt == nullptr ? j : e);

        // IF Stmt
        bc.set();       // 进入新的基本块
        ks.label(t);
        if_stmt->Dump();
        if (bc.alive()){
            ks.jump(j);
            bc.finish();
        }

        // else stmt
        if (else_stmt != nullptr)
        {
            bc.set();
            ks.label(e);
            else_stmt->Dump();
            if (bc.alive()){
                ks.jump(j);
                bc.finish();
            }
        }
        // end
        bc.set();
        ks.label(j);
    }
    return;
}

string PrimaryExpAST::Dump() const {
    if (exp)
        return exp->Dump();
    else if(lval)
        return lval->Dump();
    else
        return to_string(number);
}

int PrimaryExpAST::getValue() const {
    if(exp)
        return exp->getValue();
    else if(lval)
        return lval->getValue();
    else
        return number;
}

string UnaryExpAST::Dump() const {
    if (primary_exp)
        return primary_exp->Dump();
    string exp = unary_exp->Dump();
    string ans;
    if (unary_op == "+")
    {
        return exp;
    }
    else if(unary_op == "-")
    {
        ans = st.getTmpName();
        ks.binary("sub", ans, "0", exp);
    }
    else if (unary_op == "!")
    {
        ans = st.getTmpName();
        ks.binary("eq", ans, exp, "0");
    }
    return ans;
}

int UnaryExpAST::getValue() const {
    if (primary_exp)
        return primary_exp->getValue();
    int v = unary_exp->getValue();
    return unary_op == "+" ? v : (unary_op == "-" ? -v : !v);
}

string MulExpAST::Dump() const
{
    if (unary_exp)
        return unary_exp->Dump();
    string exp1, exp2, ans;

    exp1 = mul_exp_1->Dump();
    exp2 = unary_exp_2->Dump();

    string op = mul_op == "*" ? "mul" : (mul_op == "/" ? "div" : "mod");

    ans = st.getTmpName();
    ks.binary(op, ans, exp1, exp2);
    return ans;
}

int MulExpAST::getValue() const {
    if (unary_exp)
        return unary_exp->getValue();
    int v1 = mul_exp_1->getValue(), v2 = unary_exp_2->getValue();
    return mul_op == "*" ? v1 * v2 : (mul_op == "/" ? v1 / v2 : v1 % v2);
}

string AddExpAST::Dump() const {
    if (mul_exp)
        return mul_exp->Dump();
    string exp1, exp2, ans;

    exp1 = add_exp_1->Dump();
    exp2 = mul_exp_2->Dump();

    string op = add_op == "+" ? "add" : "sub";

    ans = st.getTmpName();
    ks.binary(op, ans, exp1, exp2);
    return ans;
}

int AddExpAST::getValue() const {
    if (mul_exp)
        return mul_exp->getValue();
    int v1 = add_exp_1->getValue(), v2 = mul_exp_2->getValue();
    return add_op == "+" ? v1 + v2 : v1 - v2;
}

string RelExpAST::Dump() const {
    if(add_exp)
        return add_exp->Dump();
    string exp1, exp2, ans, op;
    exp1 = rel_exp_1->Dump();
    exp2 = add_exp_2->Dump();
    if(rel_op == "<")
        op = "lt";
    else if(rel_op == "<=")
        op = "le";
    else if(rel_op == ">")
        op = "gt";
    else if (rel_op == ">=")
        op = "ge";
    ans = st.getTmpName();
    ks.binary(op, ans, exp1, exp2);
    return ans;
}

int RelExpAST::getValue() const {
    if (add_exp)
        return add_exp->getValue();

    int v1 = rel_exp_1->getValue(), v2 = add_exp_2->getValue();
    if (rel_op == "<")
        return v1 < v2;
    else if (rel_op == "<=")
        return v1 <= v2;
    else if (rel_op == ">")
        return v1 > v2;
    else
        return v1 >= v2;
}

string EqExpAST::Dump() const
{
    if (rel_exp)
        return rel_exp->Dump();
    string exp1, exp2, ans;

    exp1 = eq_exp_1->Dump();
    exp2 = rel_exp_2->Dump();

    string op = eq_op == "==" ? "eq" : "ne";

    ans = st.getTmpName();
    ks.binary(op, ans, exp1, exp2);
    return ans;
}

int EqExpAST::getValue() const {
    if (rel_exp)
        return rel_exp->getValue();
    int v1 = eq_exp_1->getValue(), v2 = rel_exp_2->getValue();
    return eq_op == "==" ? (v1 == v2) : (v1 != v2);
}

string LAndExpAST::Dump() const
{
    if (eq_exp)
        return eq_exp->Dump();
    // 修改支持短路逻辑
    string result = st.getVarName("SCRES");
    ks.alloc(result);
    ks.store("0", result);

    string lhs = l_and_exp_1->Dump();
    string then_s = st.getLabelName("then_sc");
    string end_s = st.getLabelName("end_sc");

    // 若左条件是true，则继续判断右条件，否则结束
    ks.br(lhs, then_s, end_s);

    bc.set();
    ks.label(then_s);
    string rhs = eq_exp_2->Dump();
    string tmp = st.getTmpName();
    ks.binary("ne", tmp, rhs, "0");
    ks.store(tmp, result);
    ks.jump(end_s);
    bc.finish();

    bc.set();
    ks.label(end_s);
    string ret = st.getTmpName();
    ks.load(ret, result);
    return ret;
}

int LAndExpAST::getValue() const {
    if (eq_exp)
        return eq_exp->getValue();
    int v1 = l_and_exp_1->getValue(), v2 = eq_exp_2->getValue();
    return v1 && v2;
}

string LOrExpAST::Dump() const {
    if (l_and_exp)
        return l_and_exp->Dump();
    // 修改支持短路逻辑
    string result = st.getVarName("SCRES");
    ks.alloc(result);
    ks.store("1", result);

    string lhs = l_or_exp_1->Dump();

    string then_s = st.getLabelName("then_sc");
    string end_s = st.getLabelName("end_sc");

    // 若左条件是false，则继续判断右条件，否则结束
    ks.br(lhs, end_s, then_s);

    bc.set();
    ks.label(then_s);
    string rhs = l_and_exp_2->Dump();
    string tmp = st.getTmpName();
    ks.binary("ne", tmp, rhs, "0");
    ks.store(tmp, result);
    ks.jump(end_s);
    bc.finish();

    bc.set();
    ks.label(end_s);
    string ret = st.getTmpName();
    ks.load(ret, result);
    return ret;
}

int LOrExpAST::getValue() const {
    if (l_and_exp)
        return l_and_exp->getValue();
    int v1 = l_or_exp_1->getValue(), v2 = l_and_exp_2->getValue();
    return v1 || v2;
}

string InitValAST::Dump() const
{
    return exp->Dump();
}

int InitValAST::getValue() const
{
    return exp->getValue();
}

int ConstInitValAST::getValue() const {
    return const_exp->getValue();
}

string LValAST::Dump(bool dump_ptr) const {
    SysYType *ty = st.getType(ident);
    if (ty->ty == SysYType::SYSY_INT_CONST)
        return to_string(getValue());
    else if (ty->ty == SysYType::SYSY_INT)
    {
        if (dump_ptr == false)
        {
            string tmp = st.getTmpName();
            ks.load(tmp, st.getName(ident));
            return tmp;
        }
        return st.getName(ident);
    }
    return "";
}

int LValAST::getValue() const {
    return st.getValue(ident);
}

int ConstExpAST::getValue() const {
    return exp->getValue();
}