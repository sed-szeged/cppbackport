// RUN: backport lambda_07.cpp -no-db -final-syntax-check

class LambdaInClass {
public:
    int b;

    void setB(int newB) { b = newB; }
    void setB(int newB) const {}

    double lambda(double a, int newB) {
        auto lambdafunc = [&, this](double param1, int param2) -> double {
            setB(param2);
            return (param1 * b + 1.0);
        };
        return lambdafunc(a, newB);
    }
    
    double lambda(double a, int newB) const {
        auto lambdafunc = [&, this](double param1, int param2) -> double {
            setB(param2);
            return (param1 * b + 2.0);
        };
        return lambdafunc(a, newB);
    }
    
    double lambdac(double a, int newB) const {
        auto lambdafunc = [&, this](double param1, int param2) -> double {
            setB(param2);
            return (param1 * b + 3.0);
        };
        return lambdafunc(a, newB);
    }
    
    LambdaInClass() : b(1) {}
};

int main() {
    LambdaInClass tmp1;
    double d1 = tmp1.lambda(4.0, 5);
    
    const LambdaInClass tmp2;
    double d2 = tmp2.lambda(4.0, 5);
    
    LambdaInClass tmp3;
    double d3 = tmp3.lambdac(4.0, 5);
    
    if((d1 == 21.0) && (d2 = 6.0) && (d3 = 7.0)) {
        return 0;
    } else {
        return 1;
    }
}
