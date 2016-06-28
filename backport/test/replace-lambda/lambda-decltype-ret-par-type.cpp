// RUN: backport lambda-decltype-ret-par-type.cpp -no-db -final-syntax-check

int m() {
    static auto lint = 34UL;
    static auto lstring = "asdf";
    static auto lstring2 = "aoret";
    return
        [](decltype(lint) l, decltype(lstring) r) -> decltype(lstring[0]) {
            return *(new char(r[0] + (signed char)l));
        } (lint, lstring) +
        [](decltype(lint) l, decltype(lstring) r) -> decltype(lstring[0]) {
            return *(new char(r[0] + (signed char)l));
        } (lint, lstring2);
}

int main() {
    if (m() != -250) {
        return -1;
    }
    return 0;
}
