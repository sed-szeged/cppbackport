// RUN: backport lambda_access_static_glob_var.cpp -no-db -final-syntax-check

int foo() {
    static int bar;
    return []() { return bar++; } (); // lambda capturing by reference
}

int main(int argc, char* argv[]) {
    return 0;
}
