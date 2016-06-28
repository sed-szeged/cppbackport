// RUN: backport no-params-lambda_01.cpp -no-db -final-syntax-check

#include <iostream>

int main() {
    int num = 2;

    [&] { std::cout << ++num << std::endl; }();
    [=] { std::cout << (num + 1) << std::endl; }();
    [=]() mutable { std::cout << ++num << std::endl; }();

    return 0;
}
