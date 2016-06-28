// RUN: backport lambda_in_template_class.cpp -no-db -final-syntax-check

template<typename T> struct A1 {
    void foo() {
        auto bar = [](int Idx) {
            return Idx;
        };
        auto tmp = bar(7);
    }
};

template<typename T> struct A2 {
    void foo() {
        auto bar = [](int Idx) {
            return Idx;
        };
        auto tmp = bar(7);
    }
};

template<typename T> struct A3 {
    void foo() {
        auto bar = [](int Idx) {
            return Idx;
        };
        auto tmp = bar(7);
    }
};

int main() {
    A1<int> a1;
    a1.foo();
	
	A2<int> a2;

    return 0;
}