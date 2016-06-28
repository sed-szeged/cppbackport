// RUN: backport lambda_03.cpp -no-db -final-syntax-check

class LambdaInClass {
public:
    int b;

    void setB(int newB) { b = newB; }

    double lambda(double a, int newB) {
        auto lambdafunc = [&, this](double param1, int param2) -> double {
            setB(param2);
            return param1 * b;
        };
        return lambdafunc(a, newB);
    }
};

int main() {
    LambdaInClass tmp;
    double d = tmp.lambda(4.0, 3);
    return 0;
}
