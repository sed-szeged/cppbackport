// RUN: backport lambda_01.cpp -no-db -final-syntax-check

#include <iostream>

int main() {
    int num = 2;
	int& mun = num;

    [&]() { std::cout << ++num << " " << ++mun << std::endl; }();
    [=]() { std::cout << (num + 1) << " " << (mun + 1) << std::endl; }();
    [=]() mutable { std::cout << ++num << " " << ++mun << std::endl; }();

    return 0;
}
