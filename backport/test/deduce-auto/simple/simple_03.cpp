// RUN: backport simple_03.cpp -no-db -final-syntax-check

namespace bar {
    struct A {};
    template<typename T>
    auto foo(A& a) -> decltype(a);
}

namespace bar {
    template<>
    auto foo<int>(A& a) -> decltype(a);
    
    template<>
    auto foo<double>(A& a) -> decltype(a) { return a; }
}

template<>
auto bar::foo<int>(A& a) -> decltype(a) { return a; }

int main() {
    bar::A a;
    bar::foo<double>(a);
    return 0;
}

