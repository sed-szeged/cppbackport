// RUN: backport final-in-name.cpp -no-db -final-syntax-check

#include <iostream>

auto final8var = "123";
auto override_ = "override";
auto a = "final";
auto b = " final ";
auto c = " override ";

struct A  {
    int final1var;
    int afinal1var;
    int vfinal();

    int final_ = 4;
    int _final = 5;

    int override8var = 123;
    int u_override_var = 17453;
    int k_final_var = 923945;
    int alma = k_final_var;

};

int main() {
    A x;

    std::cout << final8var << override_ << a << b << c;
    return 0;
}
