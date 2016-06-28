// RUN: backport simple_vector_iterate_backward.cpp -no-db -final-syntax-check

#include <vector>

static int global = 0;
template <typename T>
class iterate_backwards {
public:
    static int local;
    explicit iterate_backwards(const T &t) : t(t) {}
    typename T::const_reverse_iterator begin() const { [&](){++global; this->local++; }(); return t.rbegin(); }
    typename T::const_reverse_iterator end()   const { [&](){--global; this->local--; }(); return t.rend(); }
private:
    const T &t;
};

template <typename T>
iterate_backwards<T> backwards(const T &t) {
    return iterate_backwards<T>(t);
}

template<class T>
int iterate_backwards<T>::local = 0;

int main() {
    std::vector<int> a /* {1, 2, 3, 4}*/;

    a.push_back(1);
    a.push_back(2);
    a.push_back(3);
    a.push_back(4);

    int backsum = 0;

    for (auto k : backwards(a)) {
        backsum = backsum * 2 + k;
    }
    
    int sum = 0;

    for (auto k : a) {
        sum = sum * 2 + k;
    }

    if ((backsum == 49) && (sum == 26)) {
        return 0;
    } else {
        return 1;
    }
}
