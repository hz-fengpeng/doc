#include "String.hpp"
#include <iostream>
#include "StringArray.hpp"

int main()
{
    // String s1 = "hello world";
    // String s2 = s1;     // 这里调用的是拷贝构造函数，而不是拷贝赋值函数。
                        //  同类型初始化时 = 号调用的拷贝构造函数
    //std::cout << s1 << std::endl;

    StringArray a1;
    String s1("hello");
    String s2("world");
    std::cout << "---------" << std::endl;
    a1.addString(s1);
    a1.addString(s2);
    std::cout << "---------" << std::endl;
    a1.printArray();
    String s3 = a1.getByIndex(0);  // 调用拷贝构造
    std::cout << "s3: " << s3 << std::endl;

    String& s4 = a1.getByIndex(0);  // 不调用拷贝构造
    std::cout << "s4: " << s4 << std::endl;

    s4.upsize();
    std::cout << "s4 upsize: " << s4 << std::endl;
    a1.printArray();

}