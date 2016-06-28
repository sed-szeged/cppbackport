// RUN: backport lambda_09.cpp -no-db -final-syntax-check

int main() {
    bool d = false;
    bool &b = d;
    if (([&, d](bool a) -> decltype(a) { b = a; return (a != d); }(true)) && d) {
        return 0;
    } else {
        return 1;
    } 
}
