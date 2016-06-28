// RUN: backport simple_04.cpp -no-db -final-syntax-check

namespace A {
    namespace B {
        namespace C {
            struct D {};
            template<typename T>
            auto foo(D& d) -> decltype(d);
        }
    }
}

namespace A {
    namespace B {
        namespace C {
            template<typename T>
            auto foo(D& d) -> decltype(d);
        }
    }
}

namespace A {
    namespace B {
        template<typename T>
        auto C::foo(C::D& d) -> decltype(d);
    }
}

namespace A {
    template<typename T>
    auto B::C::foo(B::C::D& d) -> decltype(d);
}

template<typename T>
auto A::B::C::foo(A::B::C::D& d) -> decltype(d) { return d; }

int main() {
    A::B::C::D d;

    A::B::C::foo<A::B::C::D>(d);

    using namespace A;

    B::C::foo<B::C::D>(d);

    using namespace B;

    C::foo<C::D>(d);

    using namespace C;

    foo<D>(d);

    return 0;
}
