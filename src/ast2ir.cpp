#include "../include/ast.hpp"
#include "../include/symbol.hpp"
#include "util.hpp"

using namespace std;

NameManager nm;
KoopaString ks;

void CompUnitAST::Dump() const {
    func_def->Dump();
    return;
}

void FuncDefAST::Dump() const {
    nm.reset();
    ks.append("fun @" + ident + " (): ");
    func_type->Dump();
    ks.append(" {\n");
    block->Dump();
    ks.append("}");
    return;
}

void FuncTypeAST::Dump() const {
    ks.append("i32");
    return;
}

void BlockAST::Dump() const {
    ks.append("%entry: \n");
    stmt->Dump();
    return;
}

void StmtAST::Dump() const {
    string name = exp->Dump();
    ks.ret(name);
    return;
}

string PrimaryExpAST::Dump() const {
    if (exp)
        return exp->Dump();
    else
        return to_string(number);
}

int PrimaryExpAST::getValue() const {
    if(exp)
        return exp->getValue();
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
        ans = nm.getTmpName();
        ks.binary("sub", ans, "0", exp);
    }
    else if (unary_op == "!")
    {
        ans = nm.getTmpName();
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

    ans = nm.getTmpName();
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

    ans = nm.getTmpName();
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
    ans = nm.getTmpName();
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

    ans = nm.getTmpName();
    ks.binary(op, ans, exp1, exp2);
    return ans;
}

int EqExpAST::getValue() const {
    if (rel_exp)
        return rel_exp->getValue();
    int v1 = eq_exp_1->getValue(), v2 = rel_exp_2->getValue();
    return eq_op == "==" ? (v1 == v2) : (v1 != v2);
}

// 暂时不需要进行短路求值
// a&&b等价于(a!=0)&(b!=0)
string LAndExpAST::Dump() const
{
    if (eq_exp)
        return eq_exp->Dump();
    string exp1, exp2, ans;

    exp1 = l_and_exp_1->Dump();
    exp2 = eq_exp_2->Dump();

    string ans1 = nm.getTmpName();
    ks.binary("ne", ans1, exp1, "0");
    string ans2 = nm.getTmpName();
    ks.binary("ne", ans2, exp2, "0");
    ans = nm.getTmpName();
    ks.binary("and", ans, ans1, ans2);
    return ans;
}

int LAndExpAST::getValue() const {
    if (eq_exp)
        return eq_exp->getValue();
    int v1 = l_and_exp_1->getValue(), v2 = eq_exp_2->getValue();
    return v1 && v2;
}

// a||b等价于(a!=0)|(b!=0)
string LOrExpAST::Dump() const {
    if (l_and_exp)
        return l_and_exp->Dump();
    string exp1, exp2, ans;

    exp1 = l_or_exp_1->Dump();
    exp2 = l_and_exp_2->Dump();

    string ans1 = nm.getTmpName();
    ks.binary("ne", ans1, exp1, "0");
    string ans2 = nm.getTmpName();
    ks.binary("ne", ans2, exp2, "0");
    ans = nm.getTmpName();
    ks.binary("or", ans, ans1, ans2);
    return ans;
}

int LOrExpAST::getValue() const {
    if (l_and_exp)
        return l_and_exp->getValue();
    int v1 = l_or_exp_1->getValue(), v2 = l_and_exp_2->getValue();
    return v1 || v2;
}