// RUN: backport simple_02.cpp -no-db -final-syntax-check

namespace bar {
    struct A {};
    template<typename T>
    auto foo(A& a) -> decltype(a) { return a; }
}

int main() {
    bar::A a;
    return 0;
}
