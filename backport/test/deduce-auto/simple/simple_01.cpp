// RUN: backport simple_01.cpp -no-db -final-syntax-check

namespace bar {
    struct A {};
    auto foo(A& a) -> decltype(a);
}

auto bar::foo(A& a) -> decltype(a) { return a; }

int main() {
    bar::A a;
    foo(a);
    return 0;
}
