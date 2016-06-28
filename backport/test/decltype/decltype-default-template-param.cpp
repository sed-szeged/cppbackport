// RUN: backport decltype-default-template-param.cpp -no-db -final-syntax-check

template <class T = int>
class B {
private:
    T k[10];
};

B<> k;

template<class T = decltype(k)>
class A {
public:
    T asdf;
};

template<class T = decltype(k) *>
class C {
public:
    T asdf;
};

int main() {
    A<> x;
	C<> y;
}