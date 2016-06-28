// RUN: backport simple_09.cpp -no-db -final-syntax-check

template <typename T, int N> 
struct Foo {
    const static auto bar = (T)N;
};

int main() {
    Foo<int, 3> f;
    return 0;
}