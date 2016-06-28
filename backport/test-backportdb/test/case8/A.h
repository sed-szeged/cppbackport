#ifndef A_H
#define A_H

#include "OnlyA.h"
#include "All.h"
#include <iostream>

class A{
public:
    std::string a;
    std::string b;
    int count;
    A() : a(cString) {}
    void replaceA(std::string a) { this->a = a;  Useful::doTheMagic(); }
};

#endif
