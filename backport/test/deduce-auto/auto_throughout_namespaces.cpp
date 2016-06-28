// RUN: backport auto_throughout_namespaces.cpp -no-db -final-syntax-check

#include <utility>

namespace {
    struct unnamed_namespace_struct { int asdf; };
    template<class T>
    struct unnamed_namespace_my_pair {
        std::pair<T, T> p; unnamed_namespace_struct my;
        unnamed_namespace_my_pair() {}
        unnamed_namespace_my_pair(unnamed_namespace_struct t) { my = t; }
    };

    template<class T>
    struct my_array { T a[15]; };

    struct my_super_secret_type { my_array<long long int> a; long long int *p; };
}

auto glob = unnamed_namespace_my_pair<long double>();

namespace my { auto my_glob_my = my_array<char>(); }


namespace my {
    struct unnamed_namespace_struct { long double asdf; long double aer; long double a[30]; };

    namespace {
        struct my_super_secret_type { my_array<short> a; int *p; };
    }

    namespace p {
        template<class T>
        struct my_array { T* a[17]; };

        template<class T>
        struct unnamed_namespace_my_pair {
            std::pair<T, char> p; unnamed_namespace_struct my;
            my_array<long long int> a;
            unnamed_namespace_my_pair() {}
            unnamed_namespace_my_pair(unnamed_namespace_struct t) { my = t; }
        };

        namespace {
            struct my_super_secret_type { my_array<short> a; int *p; };
        }

        auto my_glob_my = my_array<char>();

        short g() {
            auto z = my_array<long double>();
            if (sizeof(my::my_glob_my) == sizeof(my_glob_my))
                return -1;

            if (sizeof(my::my_glob_my) == sizeof(::my_array<long double>))
                return -1;

            auto sec = my_super_secret_type();

            if (sizeof(sec) != sizeof(my_super_secret_type))
                return -1;

            if (sizeof(sec) < sizeof(my_array<char>) + sizeof(int *))
                return  -1;

            if (sizeof(sec) < sizeof(my_array<char>) + sizeof(int *))
                return -1;

            return 0;
        }
    }

    short f() {

        if (sizeof(my_glob_my) != sizeof(my_array<char>))
            return -1;

        if (sizeof(my_glob_my) == sizeof(p::my_glob_my))
            return -1;

        auto z = p::my_array<long double>();
        if (sizeof(z) < sizeof(long double *)* 17)
            return -1;

        auto x = my_array<long long int>();

        if (sizeof(x) != sizeof(::my_array<long long int>))
            return -1;

        auto u = unnamed_namespace_my_pair<long double>();

        if (sizeof(u) != sizeof(glob))
            return -1;

        if (sizeof(my_super_secret_type) == sizeof(p::my_super_secret_type))
            return -1;

        auto secl = my_super_secret_type();

        auto secu = p::my_super_secret_type();

        if (sizeof(secl) == sizeof(secu))
            return -1;

        if (sizeof(::my_super_secret_type) == sizeof(my_super_secret_type))
            return -1;

        return p::g();
    }
}

int main() {
    auto z = unnamed_namespace_struct();
    auto my_glob = glob;
    auto y = unnamed_namespace_my_pair<long double>(my_glob);

    auto x = my_array<float>();
    if (sizeof(x) < sizeof(float)* 15)
        return -1;

    auto ret = my::f();

    if (ret != 0)
        return ret;

    auto my_p_a = my::p::my_array<char *>();

    if (sizeof(x) == sizeof(my_p_a))
        return -1;

    return 0;
}
