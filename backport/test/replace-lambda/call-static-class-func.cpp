// RUN: backport call-static-class-func.cpp -no-db -final-syntax-check

#include <iostream>

struct B {
    void g() {
        [&]() { B::f(); }();
    }

private:
    static void f() { std::cout << "Hello World" << std::endl; };
};

int main() {
    B b;
    b.g();
}
