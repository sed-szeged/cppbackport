#ifndef C_H
#define C_H

#include <iostream>
#include "OnlyC.h"
#include "All.h"

class C {
private:
    int a;
public:
    int getA() { return a; }
    C(int val) : a(val) {}
    C() : a(12) {}
    ~C() {}
    void setA(int a) { this->a = a; }
    void printOut() { std::cerr << STR << a << " Maximum(number, 10): " << max(a,TEN) << std::endl; }
};
#endif
