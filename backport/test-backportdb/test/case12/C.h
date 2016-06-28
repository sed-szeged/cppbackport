#ifndef C_H
#define C_H

#include <iostream>

#define STR "Value of a: "
#define max(a, b) ((a >= b) ? a : b) 

#include "All.h"

class C {
private:
    int a;
public:
    int getA() { return a; }
    C(int val) : a(val) {}
    C() : a(50) {}
    ~C() {}
    void setA(int a) { this->a = a; }
    void printOut() { std::cerr << STR << a << " Maximum(number, 5): " << max(a,FIVE) << std::endl; }
};
#endif
