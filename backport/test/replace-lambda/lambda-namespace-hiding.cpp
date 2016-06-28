// RUN: backport lambda-namespace-hiding.cpp -no-db -final-syntax-check

auto name = []() { return 10; };

namespace N {
    auto name = []() { return 100; };

    int f() {
        auto name = []() { return 1000; };
        //std::cout<<name()<<std::endl;    /* 1000 */

        if (name() != 1000)
            return -1;

        //std::cout<<N::name()<<std::endl; /* 100 */

        if (N::name() != 100)
            return -1;

        //std::cout<<::name()<<std::endl;  /* 10 */

        if (::name() != 10)
            return -1;

        return 0;
    }
}

int main() {
    return ::N::f();
}
