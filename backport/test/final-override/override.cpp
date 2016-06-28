// RUN: backport override.cpp -no-db -final-syntax-check

struct A {
    virtual void foo() final {} // A::foo is final
    void bar() /*final*/ {} // Error: non-virtual function cannot be final
};

struct B final : A /* struct B is final */ {
    /* void foo(); */ // Error: foo cannot be overridden as it's final in A
    void bar() {}
};

namespace p {
    namespace {
        struct A {
            virtual void foo() {}
            void bar();
        };

        struct B final : A {
            // void foo() const override; // Error: B::foo does not override A::foo
            // (signature mismatch)
            void foo() override {} // OK: B::foo overrides A::foo
            virtual void bar() final {} // Error: A::bar is not virtual
        };
    }
}

int main() {
    B b;

    p::A t;
    p::B k;
    return 0;
}
