#ifndef __STRING__
#define __STRING__

#include <iostream>

class String
{
public:
    // 构造函数
    String(const char* cstr=0);

    // 类中有指针的成员变量必须有 拷贝构造和拷贝赋值函数
    String(const String& str);      // 拷贝构造函数，一般写法 A(const A& a);
    String& operator=(const String& str);   // 拷贝赋值函数 一般写法 A& operator=(const A& a)

    // 析构函数
    ~String();

    String(String&& str);  // move构造
    String& operator=(String&& str);  // move赋值函数

    void upsize();

    char* get_c_str() const
    {
        return m_data;
    }

private:
    char* m_data;
};

std::ostream& operator<<(std::ostream& os, const String& str);

#endif