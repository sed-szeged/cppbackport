// RUN: backport variadic_04.cpp -no-db -final-syntax-check

#include <iostream>
#include <string>

using namespace std;

template<typename T>
T adder(T v) {
    return v;
}

template<typename T, typename... Args>
T adder(T first, Args... args) {
    return first + adder(args...);
}

int main() {
    long sum = adder(1, 2, 3, 8, 7);

    cout << sum << endl;

    string s1 = "x", s2 = "aa", s3 = "bb", s4 = "yy";
    string ssum = adder(s1, s2, s3, s4);
    
    cout << ssum << endl;
    
    if((sum == 21) && (ssum.compare("xaabbyy") == 0)) {
        return 0;
    } else {
        return 1;
    }
}