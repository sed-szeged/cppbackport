// RUN: backport lambda_using_class_templ_par.cpp -no-db -final-syntax-check

static int c = 0;

template<class T>
class LambdaInClass {
public:
    T b;

    void setB(T newB) { b = newB; }

    double lambda(double a, T newB) {
        auto lambdafunc = [&, this](double param1, T param2) -> double {
            setB(param2);
            return param1 * b;
        };

        return lambdafunc(a, newB);
    }

};

int main() {
    LambdaInClass<int> tmp;
    double d = tmp.lambda(4.0, 3);
    return 0;
}
