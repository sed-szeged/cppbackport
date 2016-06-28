// RUN: backport decltype_mul_scope.cpp -no-db -final-syntax-check

#include <string>

namespace A {
    namespace B {
        namespace C {
            struct ReturnType {};
            auto foo(ReturnType& d) -> decltype(d);
        }
    }
}

namespace A {
    namespace B {
        namespace C {
            class SampleClass {
                auto foo(ReturnType& d) -> decltype(d);
            };
        }
    }
}

namespace A {
    namespace B {
        auto C::foo(C::ReturnType& d) -> decltype(d);
    }
}

namespace A {
    auto B::C::foo(B::C::ReturnType& d) -> decltype(d);
}

auto A::B::C::foo(A::B::C::ReturnType& d) -> decltype(d) { return d; }

auto A::B::C::SampleClass::foo(A::B::C::ReturnType& d) -> decltype(d) { return d; }

auto test() -> std::string { return "asd"; }


int main() {
    A::B::C::ReturnType d;

    A::B::C::foo(d);

    using namespace A;

    B::C::foo(d);

    using namespace B;

    C::foo(d);

    using namespace C;

    foo(d);

    return 0;
}
