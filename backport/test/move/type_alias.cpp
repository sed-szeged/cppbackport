// RUN: backport type_alias.cpp -no-db -final-syntax-check

#include <vector>
#include <iostream>

class A {
public:
    bool x;
    A() : x(false) {}
    ~A() {}
    A(const A &other) : x(other.x) { std::cout << "A copied" << std::endl; }
    A(A &&other) : x(other.x){ std::cout << "A moved" << std::endl; }

    void shout() { std::cout << "shouted!!" << std::endl; x = true; }
};

template <class T> 
using rvref = T&&;

int main() {
    rvref<int> asdf = 5;
    rvref<A> asdf_ = A();

    asdf_.shout();

    rvref<A> _ = static_cast<rvref<A> >(asdf_);
    
    if (_.x == false)
        return -1;

    return 0;
}

 
 