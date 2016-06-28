// RUN: backport lambda_02.cpp -no-db -final-syntax-check

#include <vector>
#include <algorithm>
#include <iostream>

int main() {
    std::vector<int> nums(6);

    int inc = 7;

    std::for_each(nums.begin(), nums.end(), [&inc](int &n) { n += inc; });
    std::for_each(nums.begin(), nums.end(), [](int n) { std::cout << n << " "; });

    return 0;
}
