// RUN: backport final.cpp -no-db -final-syntax-check

struct A {
    virtual void foo() final {} // A::foo is final
    void bar() /*final*/{} // Error: non-virtual function cannot be final
};

struct B final : A /* struct B is final */ {
    /* void foo(); */ // Error: foo cannot be overridden as it's final in A
    void bar() {}
};

int main() {
    B b;
    return 0;
}
