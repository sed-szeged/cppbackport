// RUN: backport standard-library-functions.cpp -no-db -final-syntax-check

#include <vector>
#include <algorithm>
#include <iterator>
#include <numeric>

template<typename _tasdf>
struct S {
    typedef _tasdf element_type;
    typedef _tasdf *element_pointer_type;
    typedef _tasdf &element_ref_type;

    element_type r;

    S(element_type rv) : r(rv) {}

    auto getP() const -> decltype(&r) { return &r; }
    void inc() { r++; }

    auto operator<(S<element_type> const &l) const -> decltype(this->r < l.r) {
        return this->r < l.r;
    }
    
    auto get32() -> decltype(32) { return 32; }
};

template<class Iter, class T>
Iter binary_find(Iter begin, Iter end, T val) {
    auto i = std::lower_bound(begin, end, val);

    if (i != end && !(val < *i)) {
        return i;
    }
    else {
        return end;
    }
}

auto v = std::vector<int>();

static int counter = 0;

int main() {
    v.push_back(3);
    v.push_back(1);
    v.push_back(4);
    v.push_back(1);
    v.push_back(5);

    auto total = std::accumulate(v.begin(), v.end(), 0, [](int s, const int &item) -> decltype(s) {return s + item; });

    std::sort(v.begin(), v.end(), [](int const &l, int const &r) {return l < r; });

    auto res = binary_find(v.begin(), v.end(), 3);

    auto vt = std::vector<int>();

    vt.resize(v.size());

    std::transform(v.begin(), v.end(), vt.begin(), [](int i) {return ++i; });

    auto res2 = binary_find(vt.begin(), vt.end(), 6);

    auto nv = std::vector<S<double> >();

    nv.push_back(1);
    nv.push_back(2);
    nv.push_back(3);

    auto res3 = binary_find(nv.begin(), nv.end(), S<double>(1));

    std::sort(nv.begin(), nv.end(), [&](S<double> l, S<double> const &r) {
        counter += counter;
        auto nl = l;
        S<double> nr = r;

        nl.inc();
        nr.inc();

        return nl.r > nr.r;
    });

    std::sort(nv.begin(), nv.end());


    std::sort(nv.begin(), nv.end(), [&](S<double> l, S<double> const &r) {
        counter += counter;
        auto nl = l;
        S<double> nr = r;

        nl.inc();
        nr.inc();

        return nl.r + (long)nl.getP() > nr.r + (long)nr.getP();
    });

    auto ret = (total << 16) + (std::distance(v.begin(), res) << 8) + std::distance(vt.begin(), res2) + (std::distance(nv.begin(), res3) << 20) + (counter << 20);

    if (ret != 918020)
        return -1;

    return 0;
}
