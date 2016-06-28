// RUN: backport Fraction_test.cpp -no-db -final-syntax-check

#define type short

template <type n, type d> struct frac {
    static const auto Num = n;
    static const auto Den = d;
};

template <type N, typename F> struct ScalarMultiplication {
    typedef frac<N*F::Num, F::Den> result;
};

int main() {
    typedef frac<2, 3> a;
    typedef ScalarMultiplication<2, a>::result b;

    auto val = ((b::Num << 15) | b::Den);

    return 0;
}
