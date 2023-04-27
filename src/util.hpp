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

    string getKoopaIR(){
        return koopa_str;
    }
};
