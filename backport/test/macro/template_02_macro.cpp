// RUN: backport template_02_macro.cpp -no-db -final-syntax-check

#define typeof(x) decltype(x)

template<typename T>
auto foo(T t) -> typeof(t) {
    return t;
}

template<>
auto foo<int>(int t) -> typeof(t) {
    return t + 1;
}

template<typename T>
auto bar(T t) -> typeof(t) {
    return t;
}

template<>
auto bar<int>(int t) -> typeof(t) {
    return t + 1;
}

int main() {
    bar(3);
    bar(3.);
    return 0;
}
