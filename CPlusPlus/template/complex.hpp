#ifndef __COMPLEX_HPP__
#define __COMPLEX_HPP__

#include <iostream>

template<typename T>
class complex
{
    friend complex<T>& __doapl(complex<T>*, const complex<T>&);
private:
    T re, im;
public:
    complex(T r=0, T i=0)
        : re(r), im(i)
    {
    }

    T real() const {return re;}
    T imag() const {return im;}

    complex& operator+=(const complex&);
};

template<typename T>
complex<T>& __doapl(complex<T>* ths, const complex<T>& r)
{
    ths->re += r.re;
    ths->im += r.im;
    return *ths;
}

template<typename T>
complex<T>& complex<T>::operator+=(const complex<T>& r)
{
    return __doapl(this, r);
}

// 一般模板定义放在头文件中
template<typename T>
std::ostream& operator<<(std::ostream& in, const complex<T>& r)
{
    in << "(" << r.real() << "," << r.imag() << ")";
    return in;
}

#endif