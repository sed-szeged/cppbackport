// RUN: backport lambda_06.cpp -no-db -final-syntax-check

#include <vector>
#include <algorithm>
#include <iostream>

auto lendl = []() { std::cout << std::endl; };

namespace name {
    auto forward = [](const int& a) -> const int& { return a; };
}

struct A {
    int k = []() -> int { return 6; }();
    
    void v() {
        []() { std::cout << "Hello\n"; }();
    }
};

int main() {
    A a;
    
    a.v();
    
    std::vector<int> nums(name::forward(a.k));
    
    int k = []() -> int { return 3; }();
    
    [&k](int j) { k +=j; }( [&k](int j) -> int { return k += j; }( 2 ) );

    std::for_each(nums.begin(), nums.end(), [=](int n) { std::cout << n + k << " "; });
    
    lendl();

    return 0;
}
