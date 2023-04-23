#include <string>
#include <iostream>
#include <fstream>

using namespace std;

void write_file(string file_name, string file_content)
{
    ofstream os;                        // 创建一个文件输出流对象
    os.open(file_name, ios::out);       // 将对象与文件关联
    os << file_content; // 将输入的内容放入txt文件中
    os.close();
    return;
}
