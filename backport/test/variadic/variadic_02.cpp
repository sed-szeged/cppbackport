// RUN: backport variadic_02.cpp -no-db -final-syntax-check

#include <iostream>

using namespace std;

int parity() {
    return 0;
}

int parity(int) {
    return 1;
}

template<typename... Args>
int parity(int, int, Args... args) {
    return parity(args...);
}

int main() {
    int p1 = parity(1, 2, 3, 4, 5);
    int p2 = parity(1, 2, 3, 4, 5, 6, 7, 8);
    int p3 = parity(1, 2, 3);
    int p4 = parity(1, 2);
    int p5 = parity(1, 2, 3, 4, 5, 6, 7);
    int p6 = parity(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    
    cout << p1 << ", " << p2 << ", " << p3 << ", " << p4 << ", " << p5 << ", " << p6 << endl;
    
    if((p1 == 1) &&
       (p2 == 0) &&
       (p3 == 1) &&
       (p4 == 0) &&
       (p5 == 1) &&
       (p6 == 0) ) {
        return 0;
    } else {
        return 1;
    }
}