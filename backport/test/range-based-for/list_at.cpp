// RUN: backport list_at.cpp -no-db -final-syntax-check

#include <list>

int main() {
    std::list<int> a /* {1, 2, 3, 4} */;

    a.push_back(1);
    a.push_back(2);
    a.push_back(3);
    a.push_back(4);

    int inc = 4;
    int sum = 0;

    for (auto& k : a) {
        k += inc;
    }
    
    for (auto k : a) {
        sum += k;
    }

    if (sum == 26) {
        return 0;
    } else {
        return 1;
    }
}
