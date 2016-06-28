// RUN: backport lambda_10.cpp -no-db -final-syntax-check

template<typename T>
int lambdaFunc(T a) {
    return ([]() -> bool { return true; })();
}

int main()
{
    lambdaFunc(1);
        
    return 0;
}
