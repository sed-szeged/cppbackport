// RUN: backport template_01.cpp -no-db -final-syntax-check

template <typename A>
class Test_01 {};

template <typename A>
class Test_02 {
public:
    auto foo(A a) -> decltype(a) { return a; }
};

template <typename A>
class Test_03 {
public:
    void foo(A a) { auto b = a; }
};

template <typename A>
auto nothing(A a) -> decltype(a) { return a; }

int main() {
    Test_01<int> t01;

    Test_02<int> t02;
    t02.foo(2);

    Test_03<int> t03;
    t03.foo(3);

    nothing(3);

    return 0;
}
