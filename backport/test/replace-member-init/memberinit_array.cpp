// DISABLED: UNIMPLEMENTED FEATURE
// RUN: backport memberinit_array.cpp -no-db -final-syntax-check

#include <iostream>

template<typename T, int K>
class TemplateClass {
    T someint = 32;
public:

#ifdef __GNUG__
    T array[K] = { 12, 11, 10 };
    int array2d[2][3] = { { 10, 20, 60 }, { 30, 80, 90 } };
#endif

};

template<typename T, int K>
class TemplateClass2 {
    T someint = 32;
public:
    TemplateClass<T, K> array[K];
};

TemplateClass2<short, 3> def;

template<typename T, int K>
class TemplateClass3 {
    T someint = 32;
public:
    TemplateClass2<short, 3> ref = def;
    TemplateClass2<T, K> reg;
};

int main() {

#ifdef __GNUG__
    TemplateClass<int, 130> asdf;
    TemplateClass2<double, 50> bsdf;

    for (auto i : asdf.array)
        std::cout << i << " ";

    std::cout << std::endl;

    for (auto i : bsdf.array)
    for (auto j : i.array)
        std::cout << j << " ";

    std::cout << std::endl;
    TemplateClass3<double, 35> a2;

    for (auto i : a2.ref.array)
    for (auto j : i.array)
        std::cout << j << " ";

    std::cout << std::endl;


    for (auto i : a2.reg.array)
    for (auto j : i.array)
        std::cout << j << " ";

    std::cout << std::endl;
#endif

    return 0;
}
