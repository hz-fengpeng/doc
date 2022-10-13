#include "complex.hpp"
#include <iostream>

int main()
{
    complex a(2, 3);
    complex b(a);           // 编译器自动生成拷贝构造函数
    complex c = a+b;


    std::cout << "a" << a << std::endl;
    std::cout << "b" << b << std::endl;
    std::cout << "hello world!" << std::endl;
    std::cout << "a+b" << a+b << std::endl;
    //std::cout << "c" << -c << std::endl;
}