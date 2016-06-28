// RUN: backport random-attr.cpp -no-db -final-syntax-check

auto i = 1;

[[an_attribute]] void f() {

    for (auto j = 0; j < 345; ++j) {
        i = (i + j * 988) * i;

        for (auto k = i; k < i + 1000; ++k)
            i += k * i * j;
    }
}

[[an_attribute]] auto globlambd = []() { return 89987; };

[[an_attribute2("This is an Attribute")]] int  main() {
    f();
    globlambd();

    [[an_attribute]] int   array[30];

    array[([]{return 0; }())] = 45;

    return 0;
}
