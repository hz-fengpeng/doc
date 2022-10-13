#include "complex.hpp"
#include <iostream>

int main()
{
    complex<int> a(3, 1);
    complex<int> b=a;
    std::cout << b << std::endl;
}