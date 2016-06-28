// Test Disabled
// RUN: backport variadic_06.cpp -no-db -final-syntax-check

#include <iostream>

using namespace std;

template<typename... Args>
int bit1(int bit, Args... args);

template<typename... Args>
int bit0(Args... args);

template<typename... Args>
int bitcheck(int bit, Args... args);

int binary() {
    return 0;
}

template<typename... Args>
int binary(Args... args) {
    return (bitcheck(args...));
}

template<typename... Args>
int bitcheck(int bit, Args... args) {
    if (bit) {
        return bit1(bit, args...);
    } else {
        return bit0(args...);
    }
}

template<typename... Args>
int bit1(int bit, Args... args) {
    return (bit + 2 * binary(args...));
}

template<typename... Args>
int bit0(Args... args) {
    return (2 * binary(args...));
}

int main() {
    int k = binary(1, 0, 0, 1, 0, 1);

    cout << k << endl;

    if(k == 41) {
        return 0;
    } else {
        return 1;
    }
}
