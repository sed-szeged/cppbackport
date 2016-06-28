// RUN: backport lambda_05.cpp -no-db -final-syntax-check

int main() {
    bool b = true;
    if ([&](bool a) -> decltype(a) { return (a != b); }(false)) {
        return 0;
    } else {
        return 1;
    }
}
