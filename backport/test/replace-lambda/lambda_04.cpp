// RUN: backport lambda_04.cpp -no-db -final-syntax-check

template<class A, class B>
class AutoLambda {
public:
    auto lambda(A a, B b) -> decltype(a * b) {
        auto lambdafunc = [&](A a) -> decltype(a * b) { return a * b; };

        return lambdafunc(a);
    }
};

int main() {
    AutoLambda<int, double> autoLambda;
    auto tmp = autoLambda.lambda(1, 2.0);

    return 0;
}
