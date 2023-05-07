#pragma once
#include <string>
#include <iostream>
#include <fstream>

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

    void load(const string &to, const string &from)
    {
        koopa_str += "  " + to + " = load " + from + '\n';
    }

    void store(const string &from, const string &to)
    {
        koopa_str += "  store " + from + ", " + to + '\n';
    }

    void label(const std::string &s)
    {
        koopa_str += s + ":\n";
    }

    void br(const std::string &v, const std::string &then_s, const std::string &else_s)
    {
        koopa_str += "  br " + v + ", " + then_s + ", " + else_s + '\n';
    }

    void jump(const std::string &label)
    {
        koopa_str += "  jump " + label + '\n';
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
