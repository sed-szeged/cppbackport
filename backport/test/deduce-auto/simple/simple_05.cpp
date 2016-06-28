// RUN: backport simple_05.cpp -no-db -final-syntax-check

int main() { return 0; }

namespace bar {
    struct A {};
    auto foo(A& a) -> decltype(a);
}

auto bar::foo(A& a) -> decltype(a) { return a; }
