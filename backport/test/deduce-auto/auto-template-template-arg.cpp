// RUN: backport auto-template-template-arg.cpp -no-db -final-syntax-check

#include <vector>

template<class T>
class tet {
    T mem;
    static auto const memsize = sizeof(typename T::value_type);
};

int main() {
    tet<std::vector<bool> > x;
    return 0;
}
