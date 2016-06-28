// RUN: backport init_static_var_and_function_ptr_template_par.cpp -no-db -final-syntax-check

// This test is intentianally already template instantiated.

int car;
void f(decltype(car) * param, long &&) {}

template <decltype(f) *func_ptr> class A ;

template <>
class A<&f> {
public:
    static decltype(f) *m_ptr;
};


decltype(f) *A<&f>::m_ptr = f;

int main() {
    A<f> m;

    unsigned long const i = reinterpret_cast<decltype(i)>(A<f>::m_ptr);

    return 0;
}
