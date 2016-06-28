// RUN: backport test_1.cpp -no-db -final-syntax-check

#include <string>
#include <iostream>

int globalC = 0;

struct A {
    std::string s;
    A() : s("test") {}
    A& operator

        =  (A const & p) {
        std::cout << "assigned with = copy\n"; this->s = p.s; return *this;
    }
    A& operator
        =

        (A&& p) {
        std::cout << "moved!(with operator=)\n"; this->s = static_cast<std::string&&>(p.s); return *this;
    }
    A(const A& o) : s(o.s) { std::cout << "move failed!\n";}
    A(A&& o) : s(static_cast<std::string&&>(o.s)) { std::cout << "moved!\n"; }
};

int main()
{
    std::cout << "Trying to move A\n";
    A a1 = A();
	
	
    A a2 = static_cast<A&&>(a1); // move-construct from xvalue
	
	if(globalC != 0)
		return -1;


    return 0;
}












