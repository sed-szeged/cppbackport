// RUN: backport auto_for_template_template_default_parameter_with_namespace.cpp -no-db -final-syntax-check

// This demonstrates a clang bug where namespace is missing when printing a type
// class B shows an example
// class C shows what should occur

namespace foo {
    template<typename T>
    class A {};
    
    template<template <class> class T = A>
    class B {};
    
    template<typename T = A<int> >
    class C {};
}

int main() {
    foo::B<> b;
    auto bb = b;
    auto bbb = new auto(&b);
    auto bbbb = new foo::B<foo::A>();
    
    foo::C<> c;
    auto cc = c;
    return 0;
}