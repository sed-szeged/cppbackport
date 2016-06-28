#ifndef A_H
#define A_H

#include "All.h"
#include <iostream>

#define cString "Constant string literal"
#define LITERAL "This is a string"

class A{
public:
    std::string a;
    std::string b;
    int count;
    A() : a(cString) {}
    void replaceA(std::string a) { this->a = a;  Useful::doTheMagic(); }
};

#endif
