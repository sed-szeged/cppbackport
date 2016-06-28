// RUN: backport lambda-macro.cpp -no-db -final-syntax-check

#define ret(x) return x

#define mut mutable

int main() {
    auto s = 2;

    auto res = s; {
        auto res = [&]() { s += 2; ret(s - 3); } ();
    }

    if (res != s - 2)
        return -1;

    res = [&]() { s += 2; ret(s - 3); } ();

    if (res + 3 != s)
        return -1;

    res = [=]() mut{ s += 2; ret(s - 3); } ();

    if (res + 3 - 2 != s)
        return -1;

    return 0;
}
