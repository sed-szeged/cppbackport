// RUN: backport multiple-capture.cpp -no-db -final-syntax-check

#include <vector>
#include <algorithm>

struct B {
    auto some_func() ->decltype(3) {
        return 3;
    }

    int do_() {
        std::vector<int> some_list;

        some_list.push_back(1);
        some_list.push_back(2);
        some_list.push_back(3);
        some_list.push_back(4);
        some_list.push_back(5);

        int total = 0;
        int value = 5;
        std::for_each(some_list.begin(), some_list.end(), [&, value, this](int x) {
            total += x * value * this->some_func();
        });

        if (total != 225)
            return -1;
        return 0;
    }
};

int main() {
    B x;
    return x.do_();
}
