// RUN: backport same-auto-diff-type.cpp -no-db -final-syntax-check

template<int i> struct b;

template<>
struct b<0> {
    static const auto res = (short)9897;
};

template<>
struct b<1> {
    static const auto res = 987UL;
};

template<int n, int b = 1>
struct a {
    static const auto res = a<n - 2, (n>10) > ::res;
};

template<int n>
struct a<n, 0> {
    static const auto res = b<n % 2>::res;
};

int main() {
    short s;
    unsigned long l;

    if (sizeof(a<34>::res) != sizeof(s))
        return -1;

    if (sizeof(a<33>::res) != sizeof(l))
        return -1;

    return 0;
}
