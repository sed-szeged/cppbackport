// RUN: backport simple_08.cpp -no-db -final-syntax-check

template <typename T, int N> 
struct Foo {
    auto foo(T t) -> decltype(t) { return t; }
};

int main() { return 0; }