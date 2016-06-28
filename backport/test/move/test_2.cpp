// RUN: backport test_2.cpp -no-db -final-syntax-check
#include <string>

int globalC = 0;
int moveC = 0;

namespace my {
	template<class T>
	T && move(T &x) {
		return static_cast<T &&>(x);
	}
}

struct A {
    std::string s;
    A() : s("test") {}
    A(const A& o) : s(o.s) { ++globalC; }
    A(A&& o) : s(o.s) { ++moveC;  }
};

struct B : A {
    std::string s2;
    int n;
    // implicit move contructor B::(B&&)
    // calls A's move constructor
    // calls s2's move constructor
    // and makes a bitwise copy of n
	B() : A() {}
	B(B const &o) : A(o), n(o.n), s2(o.s2) {} 
	B(B &&o) : A(my::move(o)), n(o.n), s2(o.s2) {} 
};

struct C : B {
    ~C() {}; // destructor prevents implicit move ctor C::(C&&)
};


A gA;

A f(A a) {return a;}


A g() {
    return gA;

}

A h() {
    static A troll;
    return troll;

}


A k() {

    return A();
}

int main()
{

    A a1 = f(A()); // move-construct from rvalue temporary
    A a2 = my::move(a1); // move-construct from xvalue


    if (globalC != 0)
        return -1;

    B b1;

    B b2 = my::move(b1); // calls implicit move ctor

	/*
	http://stackoverflow.com/questions/26581262/default-move-constructor-in-visual-studio-2013-update-3

    I know that you cannot mark move constructors as default, but that does not imply that the compiler does not support generating default move constructors all-together

    Unfortunately, that's exactly what that means. VS2013 does not support implicit generation of move constructors and move assignment operators. If it did, they wouldn't really have a reason to disallow the = default syntax, especially since you're allowed to do that for the copy constructor and assignment operator.

    Quoting MSDN: Support For C++11 Features (Modern C++)

    "Rvalue references v3.0" adds new rules to automatically generate move constructors and move assignment operators under certain conditions. However, this is not implemented in Visual C++ in Visual Studio 2013, due to time and resource constraints.
	*/
    
    /*if (globalC != 0) // VS would fail here
        return -1;*/   

    globalC = 0;
    C c1;
    C c2 = my::move(c1); // calls the copy constructor

    if (globalC != 1)
        return -1;

    moveC = 0;
    A _ = g(); // 1. copy
    A __ = h(); // 2. copy
    A ___ = k(); // 3. copy or rvo-d out.

    if (moveC != 0)
        return -1;

    globalC = 0;
	
	int a = 4;
	int b = my::move(a);
	
	if(a != b)
		return -3;

    A m_ = static_cast<A &&>(_);
    A m__ = (A &&)__;
    A m___ = ::my::move(___);

    if (globalC != 0)
        return -1;


    return 0;
}











