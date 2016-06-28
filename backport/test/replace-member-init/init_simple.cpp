// RUN: backport init_simple.cpp -no-db -final-syntax-check

#include <iostream>
#include <string>
#include <cstring>

struct SampleStruct {
    SampleStruct() {}
    SampleStruct(int n) : n(n) {}
    int n = 3;
    std::string s = "text";
};

union SampleUnion {
    double a = 3.2;
    int c;
};

class SeparatedDefinition {
public:
    SeparatedDefinition();

    int sampleInt = 32;
    double sampleDouble = 3.2023;
    const char * cchar = "text";
};

SeparatedDefinition::SeparatedDefinition() {}

class CtorsUndefined {
public:
    int a = 24;
    const char* str = "text";
    double *d = new double(3.02);
    SampleStruct *sStruct = new SampleStruct(32);
};

class Base {
public:
    int tmp = 23;
    SampleStruct sStruct = SampleStruct(64);
};

class Derived : public Base {
public:
    Derived() : Base() {}
    Derived(int a, std::string b) : Base(), aa(a) { str2 = b; }

    int a = 32;
    int aa = 2132;
    const char* str = "text";
    std::string str2 = "text";
};

class DerivedWithCopyCtor : public Base {
public:
    DerivedWithCopyCtor() : Base() {}
    DerivedWithCopyCtor(int a, std::string b) : Base(), aa(a) { str2 = b; }
    DerivedWithCopyCtor(const DerivedWithCopyCtor&) {} // uses in-class init in this case, so we have to move the in-class initializations to copy ctor's init list

    int a = 32;
    int aa = 2132;
    const char* str = "text";
    std::string str2 = "text";
};

int main() {
    SampleStruct ss;
    if (ss.n != 3 ||
        ss.s != "text")
        return -1;

    SampleUnion su;
    if (su.a != 3.2)
        return -2;

    SeparatedDefinition sd;
    if (sd.sampleInt != 32 ||
        sd.sampleDouble != 3.2023 ||
        strcmp(sd.cchar, "text") != 0)
        return -3;

    CtorsUndefined undef;
    if (undef.a != 24 ||
        *undef.d != 3.02 ||
        strcmp(undef.str, "text") != 0 ||
        undef.sStruct->n != 32)
        return -4;

    CtorsUndefined undefCopy = undef;
    if (undefCopy.a != 24 ||
        strcmp(undefCopy.str, "text") != 0 ||
        undefCopy.sStruct->n != 32)
        return -4;

    Derived deriv;
    if (deriv.aa != 2132 ||
        deriv.a != 32 ||
        strcmp(deriv.str, "text") != 0 ||
        deriv.str2 != "text" ||
        deriv.sStruct.n != 64)
        return -5;

    Derived deriv2(25, "changed");
    if (deriv2.aa != 25 ||
        deriv2.a != 32 ||
        strcmp(deriv2.str, "text") != 0 ||
        deriv2.str2 != "changed" ||
        deriv2.sStruct.n != 64)
        return -6;

    Derived derivCopy = deriv2;
    if (derivCopy.aa != 25 ||
        derivCopy.a != 32 ||
        strcmp(derivCopy.str, "text") != 0 ||
        derivCopy.str2 != "changed" ||
        derivCopy.sStruct.n != 64)
        return -7;

    DerivedWithCopyCtor derivWithCopy(25, "changed");
    DerivedWithCopyCtor derivCopy2 = derivWithCopy;
    if (derivCopy2.aa != 2132 ||
        derivCopy2.a != 32 ||
        strcmp(derivCopy2.str, "text") != 0 ||
        derivCopy2.str2 != "text" ||
        derivCopy2.sStruct.n != 64)
        return -8;

    return 0;
}
