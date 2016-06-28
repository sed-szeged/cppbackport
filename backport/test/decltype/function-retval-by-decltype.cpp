// RUN: backport function-retval-by-decltype.cpp -no-db -final-syntax-check

#include <iostream>
#include <vector>

auto multiply(int x, int y) -> decltype(*(new int()));

template<typename T>
auto operator + (T l, T r) -> decltype(*(new T()));

template<class T>
struct Simple {
    T val;
};

template<class T>
class S {
private:
    T val;

public:
    typedef T *pointer_to_data;
    typedef S<T> my_type;
    typedef T data_type;
    typedef S<T> *pointer_to_me_type;

    auto getVal() -> decltype(T()) {
        return val;
    }

    void setVal(T nv)  {
        val = nv;
    }

    auto get_T_vector_end(std::vector<T> v) -> decltype(v.end()) {
        return v.end();
    }

    auto get_T_vector_end2(std::vector<T> v) -> decltype(v.end());
};

int main() {
    S<Simple<double> > c;

    return 0;
}
