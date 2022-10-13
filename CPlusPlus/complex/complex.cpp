#include "complex.hpp"

// 使用引用作为返回值，无需知道以reference接收
complex& _doapl(complex* ths, const complex& r)
{
    ths->re += r.re;
    ths->im += r.im;
    return *ths;
}

// 成员函数操作符重载
complex& complex::operator+= (const complex& r)
{
    return _doapl(this, r);
}

// 非成员函数操作符重载
complex operator+(const complex& x, const complex& y)
{
    return complex(x.real()+y.real(), x.imag()+y.imag());
}
complex operator+(const complex& x, double y)
{
    return complex(x.real()+y, x.imag());
}
complex operator+(double x, const complex& y)
{
    return complex(x+y.real(), y.imag());
}

std::ostream& operator<<(std::ostream& os, const complex& x)
{
    return os << '(' <<x.real() << ','<<x.imag()<<')';
}