// RUN: backport simple_06.cpp -no-db -final-syntax-check

template <typename T, int N> 
struct Foo {
    const static auto bar = (T)N;
};

int main() { return 0; }