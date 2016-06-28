#ifndef B_H
#define B_H

#include <iostream>
#include "OnlyB.h"
#include "All.h"

using namespace std;

class B{
private:
    char a;
    vector<string> myvec;

public:
    int getA() { return a; }
    B(char val) : a(val) {}
    B() : a('c') {}
    void setA(char a) { this->a = a; Useful::doTheMagic(); }
    void print() { std::cout << BSTR << a << std::endl; }
    void addItem(string s){ myvec.push_back(s); }
    string getElemenet() { return string("RetVal"); }
};
#endif
