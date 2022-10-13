#include "String.hpp"
#include <string.h>

// 提供函数的默认参数只能声明一次，一般在头文件中声明，在定义中不用声明。
String::String(const char* cstr)
{
    if(cstr){
        m_data = new char[strlen(cstr)+1];
        strcpy(m_data, cstr);
    }else
    {
        m_data = new char[1];
        *m_data = '\0';
    }
}

String::~String()
{
    delete[] m_data;
}

// 拷贝构造函数  
String::String(const String& str)
{
    std::cout << "String copy construction" << std::endl;
    m_data = new char[strlen(str.m_data) + 1];
    strcpy(m_data, str.m_data);
}

// 拷贝赋值函数
// 1. 运算符函数返回 reference to *this
 String& String::operator=(const String& str)
 {
     // 检测自我赋值
     if(this == &str){
         return *this;
     }

     delete[] m_data;
     m_data = new char[strlen(str.m_data)+1];  // todo判断为NULL
     strcpy(m_data, str.m_data);
     return *this;
 }  

 // 还有一种异常安全的拷贝赋值函数采用 copy and swap技术
 /*
 String& String::operator=(const String& str)
 {
    String temp(str);
    swap(str);
    return *this;
 }

 */


 std::ostream& operator<<(std::ostream& os, const String& str)
 {
     os << str.get_c_str();
     return os;
 }

 void String::upsize()
 {
    int len = strlen(m_data);
    for(int i = 0; i < len; i++)
    {
        m_data[i] = m_data[i] - 32;
    }
 }

 String::String(String&& str)
    :m_data(NULL)
 {
    std::cout << "move construction " << std::endl;
    m_data = str.m_data;
    str.m_data = NULL;
 }

 String& String::operator=(String&& str)
 {
    delete m_data;
    m_data = str.m_data;
    str.m_data = NULL;
    return *this;
 }