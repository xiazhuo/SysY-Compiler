#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <stack>
#include <vector>

using namespace std;

// 封装了一个生成KoopaIR的类,避免反复传参
class KoopaString
{
private:
    string koopa_str;

public:
    void append(const string &s)
    {
        koopa_str += s;
    }

    void binary(const string &op, const string &rd, const string &s1, const string &s2)
    {
        koopa_str += "  " + rd + " = " + op + " " + s1 + ", " + s2 + "\n";
    }

    void ret(const string &name)
    {
        koopa_str += "  ret " + name + "\n";
    }

    void alloc(const string &name)
    {
        koopa_str += "  " + name + " = alloc i32\n";
    }

    void globalAllocINT(const string &name, const string &init = "zeroinit")
    {
        koopa_str += "global " + name + " = alloc i32, " + init + "\n";
    }

    void load(const string &to, const string &from)
    {
        koopa_str += "  " + to + " = load " + from + '\n';
    }

    void store(const string &from, const string &to)
    {
        koopa_str += "  store " + from + ", " + to + '\n';
    }

    void label(const string &s)
    {
        koopa_str += s + ":\n";
    }

    void br(const string &v, const string &then_s, const string &else_s)
    {
        koopa_str += "  br " + v + ", " + then_s + ", " + else_s + '\n';
    }

    void jump(const string &label)
    {
        koopa_str += "  jump " + label + '\n';
    }

    void call(const string &to, const string &func, const vector<string> &params)
    {
        if (to.length())
        {
            koopa_str += "  " + to + " = ";
        }
        else
        {
            koopa_str += "  ";
        }
        koopa_str += "call " + func + "(";
        if (params.size())
        {
            int n = params.size();
            koopa_str += params[0];
            for (int i = 1; i < n; i++)
            {
                koopa_str += ", " + params[i];
            }
        }
        koopa_str += ")\n";
    }

    void declLibFunc()
    {
        koopa_str.append("decl @getint(): i32\n");
        koopa_str.append("decl @getch(): i32\n");
        koopa_str.append("decl @getarray(*i32): i32\n");
        koopa_str.append("decl @putint(i32)\n");
        koopa_str.append("decl @putch(i32)\n");
        koopa_str.append("decl @putarray(i32, *i32)\n");
        koopa_str.append("decl @starttime()\n");
        koopa_str.append("decl @stoptime()\n");
        koopa_str.append("\n");
    }

    string getKoopaIR(){
        return koopa_str;
    }
};

// 封装了一个生成Riscvstr的类,避免反复传参
class RiscvString
{
private:
    string riscv_str;
public:
    void append(const string &s)
    {
        riscv_str += s;
    }

    void binary(const string &op, const string &rd, const string &rs1, const string &rs2)
    {
        riscv_str += "  " + op + "\t" + rd + ", " + rs1 + ", " + rs2 + "\n";
    }

    void ret()
    {
        riscv_str += "  ret\n";
    }

    void two(const string &op, const string &a, const string &b)
    {
        riscv_str += "  " + op + "\t" + a + ", " + b + "\n";
    }

    void li(const string &to, int im)
    {
        riscv_str += "  li\t" + to + ", " + to_string(im) + "\n";
    }

    string getRiscvStr()
    {
        return riscv_str;
    }
};

// 每个基本块的结尾必须有且仅有 br, jump 或 ret中的一个
class BlockController
{
private:
    bool f = true;

public:
    bool alive()
    {
        return f;
    }

    void finish()
    {
        f = false;
    }

    void set()
    {
        f = true;
    }
};

class WhileName
{
public:
    string entry_name, body_name, end_name;
    WhileName(const string &_entry, const string &_body, const string &_end) : entry_name(_entry), body_name(_body), end_name(_end) {}
};

// while栈，记录 while的名字，入口，结束位置
class WhileStack
{
private:
    stack<WhileName> whiles;

public:
    void append(const string &_entry, const string &_body, const string &_end)
    {
        whiles.emplace(_entry, _body, _end);
    }

    void quit()
    {
        whiles.pop();
    }

    string getBodyName()
    {
        return whiles.top().body_name;
    }

    // 从 continue 跳转时，用 getEntryName 函数获得 while 的入口地址
    string getEntryName()
    {
        return whiles.top().entry_name;
    }

    // 从 break 跳出时，用 getEndName 函数获得 while 结束地址
    string getEndName()
    {
        return whiles.top().end_name;
    }
};