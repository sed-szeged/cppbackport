// RUN: backport template_03.cpp -no-db -final-syntax-check

template<typename T, typename U>
int foo(T t, U u) {
    auto k = t * u;
    return (int)k;
}

template<>
int foo<int, double>(int t, double u) {
    auto k = t * u * 2.0;
    return (int)k;
}

template<>
int foo<double, int>(double t, int u) {
    int k = 3;
    return k;
}

template<typename T, typename U>
int bar(T t, U u) {
    auto k = t * u;
    return (int)k;
}

template<>
int bar<int, double>(int t, double u) {
    auto k = t * u * 2.0;
    return (int)k;
}

template<>
int bar<double, int>(double t, int u) {
    int k = 3;
    return k;
}

int main() {
    bar(3, 2);

    bar(3., 2);

    bar(4, 1.);

    return 0;
}
