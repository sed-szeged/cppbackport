// RUN: backport list.cpp -no-db -final-syntax-check

#include <list>

int main() {
    std::list<int> a /* {1, 2, 3, 4} */;

    a.push_back(1);
    a.push_back(2);
    a.push_back(3);
    a.push_back(4);

    int sum = 0;

    for (auto k : a) {
        sum += k;
        k = 1;
    }

    if (sum == 10) {
        return 0;
    } else {
        return 1;
    }
}
