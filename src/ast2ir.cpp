#include "../include/ast.hpp"
#include "../include/symbol.hpp"
#include "../include/util.hpp"
#include <iostream>

using namespace std;

SymbolTableStack st;
KoopaString ks;
BlockController bc;
WhileStack wst;

// 部分实用函数（大部分是因为要递归因此单独拎出来）
// 局部变量数组初始化
// 初始化内容在ptr所指的内存区域，数组类型由len描述. ptr[i]为常量，或者是KoopaIR中的名字
void initLocalArray(string name, string *ptr, const vector<int> &len)
{
    int n = len[0];
    if (len.size() == 1)
    {
        for (int i = 0; i < n; ++i)
        {
            if(ptr[i] == "0")
                continue;
            string tmp = st.getTmpName();
            ks.getelemptr(tmp, name, to_string(i));
            ks.store(ptr[i], tmp);
        }
    }
    else
    {
        int width = 1;
        vector<int> sublen(len.begin() + 1, len.end());
        for (auto l : sublen)
            width *= l;
        for (int i = 0; i < n; ++i)
        {
            string tmp = st.getTmpName();
            if (ptr[i*width] != "0"){
                ks.getelemptr(tmp, name, to_string(i));
            }
            initLocalArray(tmp, ptr + i * width, sublen);
        }
    }
}

// 全局变量数组初始化
string initGlobalArray(string *ptr, const vector<int> &len)
{
    int n = len[0];
    string ret = "{";
    if (len.size() == 1)
    {
        ret += ptr[0];
        for (int i = 1; i < n; ++i)
        {
            ret += ", " + ptr[i];
        }
    }
    else
    {
        int width = 1;
        vector<int> sublen(len.begin() + 1, len.end());
        for (auto l : sublen)
            width *= l;
        ret += initGlobalArray(ptr, sublen);
        for (int i = 1; i < n; ++i)
        {
            ret += ", " + initGlobalArray(ptr + width * i, sublen);
        }
    }
    ret += "}";
    return ret;
}

string getArrayType(const vector<int> &len)
{
    string ans = "i32";
    // 从最内层到最外层迭代
    for (int i = len.size() - 1; i >= 0; i--)
    {
        ans = "[" + ans + ", " + to_string(len[i]) + "]";
    }
    return ans;
}

string getElemPtr(const string &name, const vector<string> &index)
{
    if (index.size() == 1)
    {
        string tmp = st.getTmpName();
        ks.getelemptr(tmp, name, index[0]);
        return tmp;
    }
    else
    {
        string tmp = st.getTmpName();
        ks.getelemptr(tmp, name, index[0]);
        return getElemPtr(
            tmp,
            vector<string>(index.begin() + 1, index.end()));
    }
}

void CompUnitAST::Dump() const {
    st.alloc();     // 全局作用域栈

    // 库函数声明
    ks.declLibFunc();
    st.insertFUNC("getint", SysYType::SYSY_FUNC_INT);
    st.insertFUNC("getch", SysYType::SYSY_FUNC_INT);
    st.insertFUNC("getarray", SysYType::SYSY_FUNC_INT);
    st.insertFUNC("putint", SysYType::SYSY_FUNC_VOID);
    st.insertFUNC("putch", SysYType::SYSY_FUNC_VOID);
    st.insertFUNC("putarray", SysYType::SYSY_FUNC_VOID);
    st.insertFUNC("starttime", SysYType::SYSY_FUNC_VOID);
    st.insertFUNC("stoptime", SysYType::SYSY_FUNC_VOID);

    // 全局变量
    int len = decls.size();
    for (int i = 0; i < len; i++)
        decls[i]->Dump(true);
    if (len)
        ks.append("\n");

    // 全局函数
    len = func_defs.size();
    for (int i = 0; i < len; i++)
        func_defs[i]->Dump();

    st.quit();
    return;
}

void FuncDefAST::Dump() const {
    st.resetNameManager();

    // 函数名加到符号表 (全局)
    st.insertFUNC(ident, func_type->tag == BTypeAST::INT ? 
            SysYType::SYSY_FUNC_INT : SysYType::SYSY_FUNC_VOID);
    ks.append("fun " + st.getName(ident) + "(");

    st.alloc();
    int i = 0, len = func_f_params.size();
    // 打印函数类型，但不直接使用参数中的变量，因此先不加入符号表中
    vector<string> var_names;
    if(len){
        var_names.push_back(st.getVarName(func_f_params[i]->ident));
        ks.append(var_names.back() + ": ");
        func_f_params[i]->Dump();
        for (i++; i < len; i++)
        {
            ks.append(", ");
            var_names.push_back(st.getVarName(func_f_params[i]->ident));
            ks.append(var_names.back() + ": ");
            func_f_params[i]->Dump();
        }
    }
    ks.append(")");
    if (func_type->tag == BTypeAST::INT)
    {
        ks.append(": i32");
    }
    ks.append(" {\n");

    bc.set();           // 函数是一个基本块
    ks.label("%entry");

    // 将参数中的变量映射为新变量后插入到函数作用域中，即参数中的变量并不在此函数中
    for (i = 0; i < len; i++)
    {
        string var = var_names[i];
        if (func_f_params[i]->tag == FuncFParamAST::VARIABLE){
            st.insertINT(func_f_params[i]->ident);
            string name = st.getName(func_f_params[i]->ident);
            ks.alloc(name);
            ks.store(var, name);
        }else{
            vector<int> len;
            vector<int> padding_len;    // 数组指针维度（第一维设置为-1，表示指针）
            padding_len.push_back(-1);

            func_f_params[i]->getIndex(len);
            for (int l : len)
                padding_len.push_back(l);

            // 实际上插入的是数组指针，这里复用了接口
            st.insertArray(func_f_params[i]->ident, padding_len, SysYType::SYSY_ARRAY);
            string name = st.getName(func_f_params[i]->ident);

            // 注意，这里应该传len，而不是padding_len
            ks.alloc(name, "*" + getArrayType(len));
            ks.store(var, name);
        }
    }

    block->Dump();
    // 特判空块
    if (bc.alive())
    {
        if (func_type->tag == BTypeAST::INT)
            ks.ret("0");
        else
            ks.ret("");
        bc.finish();
    }
    ks.append("}\n\n");
    st.quit();
    return;
}

// 返回参数类型，如i32, *[i32, 4]
void FuncFParamAST::Dump() const
{
    if (tag == VARIABLE)
    {
        ks.append("i32");
        return;
    }
    string ans = "i32";
    for (auto &ce : const_exps)
    {
        ans = "[" + ans + ", " + to_string(ce->getValue()) + "]";
    }
    ks.append("*" + ans);
    return;
}

// 得到数组指针各维度的长度信息
void FuncFParamAST::getIndex(vector<int> &len) const
{
    len.clear();
    for (auto &ce : const_exps)
    {
        len.push_back(ce->getValue());
    }
    return;
}

void BTypeAST::Dump() const
{
    if (tag == BTypeAST::INT)
    {
        ks.append("i32");
    }
}

void BlockAST::Dump() const {
    st.alloc();
    int len = block_items.size();

    for (int i = 0; i < len; i++)
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

void DeclAST::Dump(bool is_global) const
{
    if (var_decl)
        var_decl->Dump(is_global);
    else
        const_decl->Dump(is_global);
}

void ConstDeclAST::Dump(bool is_global) const
{
    int len = const_defs.size();
    for (int i = 0; i < len; i++)
    {
        const_defs[i]->Dump(is_global);
    }
}

void VarDeclAST::Dump(bool is_global) const
{
    int len = var_defs.size();
    for (int i = 0; i < len; i++)
    {
        var_defs[i]->Dump(is_global);
    }
}

void ConstDefAST::Dump(bool is_global) const
{
    if(const_exps.size()){
        DumpArray(is_global);
        return;
    }
    int v = const_init_val->getValue();
    st.insertINTCONST(ident, v);
}

void ConstDefAST::DumpArray(bool is_global) const
{
    vector<int> len;
    for (auto &ce : const_exps)
    {
        len.push_back(ce->getValue());
    }
    st.insertArray(ident, len, SysYType::SYSY_ARRAY_CONST);

    string name = st.getName(ident);
    string array_type = getArrayType(len);

    // 若全局初始化列表为空，则用zeroinit初始化
    if (is_global && const_init_val->inits.size()==0){
        ks.globalAllocArray(name, array_type);
        return;
    }

    // 得到补完 0之后的初始化列表
    int total_len = 1;
    for (auto i : len)
        total_len *= i;
    string *init = new string[total_len];
    for (int i = 0; i < total_len; i++)
        init[i] = "0";
    const_init_val->getInitVal(init, len);

    if (is_global)
    {
        ks.globalAllocArray(name, array_type, initGlobalArray(init, len));
    }
    else
    {
        ks.alloc(name, array_type);
        ks.store("zeroinit", name);
        initLocalArray(name, init, len);
    }
    return;
}

void VarDefAST::Dump(bool is_global) const
{
    if (const_exps.size())
    {
        DumpArray(is_global);
        return;
    }
    st.insertINT(ident);
    string name = st.getName(ident);
    if (is_global)
    {
        if (!init_val)
        {
            ks.globalAllocINT(name);
        }
        else
        {
            int v = init_val->getValue();
            ks.globalAllocINT(name, to_string(v));
        }
    }
    else{
        ks.alloc(name);
        if (init_val)
        {
            string s = init_val->Dump();
            ks.store(s, name);
        }
    }
    return;
}

void VarDefAST::DumpArray(bool is_global) const
{
    vector<int> len;
    for (auto &ce : const_exps)
    {
        len.push_back(ce->getValue());
    }
    st.insertArray(ident, len, SysYType::SYSY_ARRAY);

    string name = st.getName(ident);
    string array_type = getArrayType(len);

    // 若没有初始化列表
    if(init_val == nullptr){
        if(is_global)
            ks.globalAllocArray(name, array_type);
        else
            ks.alloc(name, array_type);
        return;
    }

    // 若全局初始化列表为空，则用zeroinit初始化
    if (is_global && init_val->inits.size()==0){
        ks.globalAllocArray(name, array_type);
        return;
    }

    // 得到补完 0之后的初始化列表
    int total_len = 1;
    for (auto i : len)
        total_len *= i;
    string *init = new string[total_len];
    for (int i = 0; i < total_len; i++)
        init[i] = "0";

    if (is_global)
    {
        // 全局变量初始化要在编译期求得初始值
        init_val->getInitVal(init, len, true);

        ks.globalAllocArray(name, array_type, initGlobalArray(init, len));
    }
    else
    {
        // 局部变量初始化是在运行时求值
        // TO DO: 未初始化的变量不赋值
        init_val->getInitVal(init, len, false);

        ks.alloc(name, array_type);
        ks.store("zeroinit",name);
        initLocalArray(name, init, len);
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
    else if (tag == WHILE)
    {
        string while_entry = st.getLabelName("while_entry");
        string while_body = st.getLabelName("while_body");
        string while_end = st.getLabelName("while_end");

        wst.append(while_entry, while_body, while_end);

        ks.jump(while_entry);

        bc.set();
        ks.label(while_entry);
        string cond = exp->Dump();
        ks.br(cond, while_body, while_end);

        bc.set();
        ks.label(while_body);
        stmt->Dump();
        if (bc.alive()){
            ks.jump(while_entry);
            bc.finish();
        }

        bc.set();
        ks.label(while_end);
        wst.quit(); // 该while处理已结束，退栈
    }
    else if (tag == BREAK)
    {
        ks.jump(wst.getEndName()); // 跳转到while_end
        bc.finish();
    }
    else if (tag == CONTINUE)
    {
        ks.jump(wst.getEntryName()); // 跳转到while_entry
        bc.finish();
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
    else if (unary_exp)
    {
        string exp = unary_exp->Dump();
        string ans;
        if (unary_op == "+")
        {
            return exp;
        }
        else if (unary_op == "-")
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
    else
    {
        // Func_Call
        string tmp = "";
        vector<string> par;
        // 是否有返回值
        if (st.getType(ident)->ty == SysYType::SYSY_FUNC_INT)
        {
            tmp = st.getTmpName();
        }
        int len = exps.size();
        for (int i = 0; i < len; i++)
        {
            par.push_back(exps[i]->Dump());
        }
        ks.call(tmp, st.getName(ident), par);
        return tmp;
    }
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

// 难点：得到填充0后的初始化列表
void InitValAST::getInitVal(string *ptr, const vector<int> &len, bool is_global) const
{
    int n = len.size();
    vector<int> width(n);
    width[n - 1] = len[n - 1];
    for (int i = n - 2; i >= 0; --i)
    {
        width[i] = width[i + 1] * len[i];
    }
    int i = 0; // 指向下一步要填写的内存位置
    for (auto &init_val : inits)
    {
        // 递归终点
        if (init_val->exp)
        {
            // 全局变量要在编译期求得初始值
            if (is_global)
            {
                ptr[i++] = to_string(init_val->exp->getValue());
            }
            // 局部变量在运行时算出
            else
            {
                ptr[i++] = init_val->Dump();
            }
        }
        else
        {
            int j = n - 1;
            if (i == 0)
            {
                j = 1;
            }
            else
            {
                j = n - 1;
                for (; j >= 0; --j)
                {
                    if (i % width[j] != 0)
                        break;
                }
                ++j; // j 指向最大的可除的维度
            }
            init_val->getInitVal(
                ptr + i,
                vector<int>(len.begin() + j, len.end()));
            i += width[j];
        }
        if (i >= width[0])
            break;
    }
}

int ConstInitValAST::getValue() const {
    return const_exp->getValue();
}

// 对ptr指向的区域初始化，所指区域的数组类型由len规定
void ConstInitValAST::getInitVal(string *ptr, const vector<int> &len) const
{
    int n = len.size();
    vector<int> width(n);
    width[n - 1] = len[n - 1];
    for (int i = n - 2; i >= 0; --i)
    {
        width[i] = width[i + 1] * len[i];
    }
    int i = 0; // 指向下一步要填写的内存位置
    for (auto &init_val : inits)
    {
        if (init_val->const_exp)
        {
            ptr[i++] = to_string(init_val->getValue());
        }
        else
        {
            int j = n - 1;
            if (i == 0)
            {
                j = 1;
            }
            else
            {
                j = n - 1;
                for (; j >= 0; --j)
                {
                    if (i % width[j] != 0)
                        break;
                }
                ++j;               // j 指向最大的可除的维度
            }
            init_val->getInitVal(
                ptr + i,
                vector<int>(len.begin() + j, len.end()));
            i += width[j];
        }
        if (i >= width[0])
            break;
    }
}

string LValAST::Dump(bool dump_ptr) const {
    SysYType *ty = st.getType(ident);
    if(!exps.size()){
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
        // 如int a[2][3] 中的 a,或int a[]中的 a
        else
        {
            // 若是数组指针（变量）
            if (ty->value == -1)
            {
                string tmp = st.getTmpName();
                cout << tmp<<' '<<st.getName(ident) << "\n";
                ks.load(tmp, st.getName(ident));
                return tmp;
            }
            // 首值（常量）
            string tmp = st.getTmpName();
            ks.getelemptr(tmp, st.getName(ident), "0");
            return tmp;
        }
    }
    // 多维数组指针或数组值（为int）
    else
    {
        vector<string> index;
        vector<int> len;

        for (auto &e : exps)
        {
            index.push_back(e->Dump());
        }

        ty->getIndex(len);

        // hint: len可以是-1开头的，说明这个数组是函数中使用的参数
        // 如 a[-1][3][2],表明a是参数 a[][3][2], 即 *[3][2].
        // 此时第一步不能用getelemptr，而应该getptr

        string name = st.getName(ident);
        string tmp;
        if (len.size() != 0 && len[0] == -1)
        {
            vector<int> sublen(len.begin() + 1, len.end());
            string tmp_val = st.getTmpName();
            ks.load(tmp_val, name);
            string first_indexed = st.getTmpName();
            ks.getptr(first_indexed, tmp_val, index[0]);
            if (index.size() > 1)
            {
                tmp = getElemPtr(
                    first_indexed,
                    vector<string>(
                        index.begin() + 1, index.end()));
            }
            else
            {
                tmp = first_indexed;
            }
        }
        else
        {
            tmp = getElemPtr(name, index);
        }

        if (index.size() < len.size())
        {
            // 一定是作为函数参数即实参使用，因为下标不完整
            string real_param = st.getTmpName();
            ks.getelemptr(real_param, tmp, "0");
            return real_param;
        }
        if (dump_ptr)
            return tmp;
        string tmp2 = st.getTmpName();
        ks.load(tmp2, tmp);
        return tmp2;
    }
}

int LValAST::getValue() const {
    return st.getValue(ident);
}

int ConstExpAST::getValue() const {
    return exp->getValue();
}