// RUN: backport simple_07.cpp -no-db -final-syntax-check

template <typename T, int N> 
struct Foo {
    const static auto bar = (T)N;
    auto foo(T t) -> decltype(t) { return t; }
};

int main() { return 0; }