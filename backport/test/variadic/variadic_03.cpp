// RUN: backport variadic_03.cpp -no-db -final-syntax-check

#include <iostream>

using namespace std;

int parity() {
    return 0;
}

template<typename T>
int parity(T) {
    return 1;
}

template<typename T, typename U, typename... Args>
int parity(T, U, Args... args) {
    return parity(args...);
}

struct Bar {};

int main() {
    int p1 = parity(1, 2., 3., "Foo", 5);
    int p2 = parity(1., "Foo", 3, 'f', 5, 6., Bar(), 8);
    
    cout << p1 << ", " << p2 << endl;

    if((p1 == 1) && (p2 == 0)) {
        return 0;
    } else {
        return 1;
    }
}