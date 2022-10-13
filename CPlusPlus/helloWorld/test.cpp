#include <iostream>
#include <string>
#include <memory>

class testGlobalInit
{
    public:
        testGlobalInit(int input):a(input){}
        int getA(){return a;}
    private:
        int a;
};

class foo1
{

};

class foo2
{
    foo2(){}
    virtual ~foo2(){}
};


class foo3
{
    foo3(){}
    ~foo3(){}
};

int GetSize(int data[])
{   // 会退化成指针
    return sizeof(data);
}

class base
{
    public:
        base(int a):m_base(a){}

        virtual void p()
        {
            std::cout << "m_base: " << m_base << std::endl;
        }
    private:
        int m_base;
};

class derive: public base
{
    public:
        derive(int b, int d): base(b), m_derive(d){}

        virtual void p()
        {
            std::cout << "m_derive: " << m_derive << std::endl;
        }

    private:
        int m_derive;

};

void valueSharedPtr(std::shared_ptr<std::string> a)
{
    std::cout << "valueSharedPtr: " << a.use_count() << std::endl;
    // auto b = std::move(a);
    // std::cout << "valueSharedPtr after std::move: " << a.use_count() << std::endl;
}

void refSharedPtr(std::shared_ptr<std::string>& a)
{    // 使用引用 智能指针的计数没有加1
    std::cout << "refSharedPtr: " << a.use_count() << std::endl;
    // auto b = std::move(a);
    // std::cout << "refSharedPtr after std::move: " << a.use_count() << std::endl;
}

//testGlobalInit g_a;  // 编译不通过
testGlobalInit g_a(1);

int main()
{
    std::cout << "hello world" << std::endl;
    std::cout << "C++ 11" << std::endl;
    std::cout << "git" << std::endl;
    std::cout << "STL" << std::endl;
    std::cout << "sizeof foo1 " << sizeof(foo1) << std::endl;  // 输出1，空类型也会占用空间
    std::cout << "sizeof foo2 " << sizeof(foo2) << std::endl;   // 输出8，有虚函数会生成虚函数表指针，所以占用一个指针长度
    std::cout << "sizeof foo3 " << sizeof(foo3) << std::endl;  // 还是1，构造函数和析构函数在obj之外

    int data1[] = {1,2,3,4,5};
    int size1 = sizeof(data1);

    int* data2 = data1;
    int size2 = sizeof(data2);
    int size3 = GetSize(data1);
    std::cout << size1 << " " << size2 << " " << size3 << std::endl;

    base* pBase = new derive(1,2);
    pBase->p();

    std::shared_ptr<std::string> a(new std::string("helloworld"));
    std::cout << "main use count " << a.use_count() << std::endl;

    valueSharedPtr(a);
    //refSharedPtr(a);

    std::cout << "main use count " << a.use_count() << std::endl;

    std::cout << g_a.getA() << std::endl;

    return 0;
}