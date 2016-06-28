// RUN: backport template_05.cpp -no-db -final-syntax-check

template<typename T>
auto foo(T t) -> decltype(t);

template<typename T>
auto foo(T t) -> decltype(t) {
    return t;
}

template<>
auto foo<int>(int t) -> decltype(t);

template<>
auto foo<int>(int t) -> decltype(t) {
    return t + 1;
}

template<typename T>
auto bar(T t) -> decltype(t);

template<typename T>
auto bar(T t) -> decltype(t) {
    return t;
}

template<>
auto bar<int>(int t) -> decltype(t);

template<>
auto bar<int>(int t) -> decltype(t) {
    return t + 1;
}

int main() {
    bar(3);

    bar(3.);

    return 0;
}
