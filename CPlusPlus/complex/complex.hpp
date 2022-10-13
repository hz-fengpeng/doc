#ifndef  __COMPLEX__
#define __COMPLEX__

#include <iostream>


class  complex
{
    // 友元函数，一般还需要在类外进行声明
    friend complex& _doapl(complex*, const complex&);
                                                                                            
    public:
        // 构造函数，采用成员初始化列表
        complex(double r , double i)
            :re(r), im(i)
        {
             
        }

        // 成员函数操作符重载
        complex& operator+=(const complex&);
        // 这里表示复数取反取正有点问题
        // complex operator+(const complex& x)
        // {
        //     return x;
        // }
        // complex operator-(const complex& x)
        // {
        //     return complex(-x.re, -x.im);
        // }

        
        double real() const     // const 表示不改变对象
        {
            return re;
        }

        double imag() const
        {
            return im;
        }

    private:
        double re;
        double im;
};

complex& _doapl(complex* ths, const complex& r);

// 非成员函数操作符重载
complex operator+(const complex& x, const complex& y);
complex operator+(const complex& x, double y);
complex operator+(double x, const complex& y);

std::ostream& operator<<(std::ostream& os, const complex& x);


#endif