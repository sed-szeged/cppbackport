// RUN: backport noreturn.cpp -no-db -final-syntax-check

auto i = 0;

[[noreturn]] void f() {

    for (auto j = 0; j < 345; ++j) {
        i = (i + j * 988) * i;

        for (auto k = i; k < i + 1000; ++k)
            i += k * i * j;
    }
}

auto globlambd = []() { return 89987; };

int main() {
    f();
    globlambd();

    int array[30];

    array[([]{ return 0; }())] = 45;

    return 0;
}
