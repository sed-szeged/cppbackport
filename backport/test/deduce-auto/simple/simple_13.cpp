// RUN: backport simple_13.cpp -no-db -final-syntax-check

template <typename T, int N> 
struct Foo {
    auto foo(T t) -> decltype(t) { return t; }
};

int main() {
    Foo<int, 3> f;
    f.foo(4);
    return 0;
}