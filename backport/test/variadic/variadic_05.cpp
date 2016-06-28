// RUN: backport variadic_05.cpp -no-db -final-syntax-check

#include <iostream>
#include <string>

using namespace std;

template<typename T>
bool pair_comparer(T a, T b) {
    return a == b;
}

template<typename T, typename... Args>
bool pair_comparer(T a, T b, Args... args) {
    return a == b && pair_comparer(args...);
}

int main() {
    bool b1 = pair_comparer(1.5, 1.5, 2, 2, 'a', 'a');
    bool b2 = pair_comparer(1.5, 1.5, 2, 3, 'a', 'a');
    
    cout << b1 << ", " << b2 << endl;
    
    if(b1 && !b2) {
        return 0;
    } else {
        return 1;
    }
}