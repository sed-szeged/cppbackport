// RUN: backport type-alias01.cpp -no-db -final-syntax-check
#include <ios>

typedef unsigned long ulong;

using constrefint = const int&;
// simple typedef
using ul = unsigned long;
using ull = unsigned long long;
using uc = unsigned char;
using us = unsigned short;
using l = long;
using ll = long long;
using c = char;
using s = short;

// the following two objects have the same type
unsigned long l1;
ll ll2;
l l3;
ulong l2;

using nauto = decltype(2*3);
using int_t = int;
using intp_t = int*;
// typedef void (&fp)(int, int);
using fp = int (&)(int,ulong);
// typedef int arr_t[10];
using arr_t=int[10];

using ref = int&;
// the following two objects have the same type
int a1[10];
arr_t a2;

using dcltype = decltype(a2);

using S = struct {int a; int b;};
using pS = struct {int a; int b;}*;
using U = union { int a; float b; } ;
using C = class {int a; };

using SS = S;
typedef S SSS;

using AliasC = C;

class SampleClass
{
    using InnerUnnamed = struct { int a; };
    int b;
};

using SampleC = SampleClass;
// the following two objects have the same type
pS ps1;
S* ps2;

// std::add_const, like many other metafunctions, use member typedefs
template< class T>
struct add_const {
    using type = const T;
};

// type alias, identical to
// typedef std::ios_base::fmtflags flags;
using flags = std::ios_base::fmtflags;
// the name 'flags' now denotes a type:
flags fl = std::ios_base::dec;

// type alias, identical to
// typedef void (*func)(int, int);
using func = void (*) (int,int);
// the name 'func' now denotes a pointer to function:
void example(int, int) {}
func fn = example;

// type alias can introduce a member typedef name
template<typename T>
struct Container {
    using value_type = T;
};

// which can be used in generic programming
template<typename Container>
int_t fn2(const Container& c)
{
    typename Container::value_type n1;
    int n2;
    return sizeof(n1) == sizeof(n2);
}

int_t main()
{
    Container<int_t> c;
    if (!fn2(c)) // Container::value_type will be int in this function
        return 1;

    if (sizeof(l1) != sizeof(l2))
        return 2;
    if (sizeof(a1) != sizeof(a2))
        return 3;
    return 0;
}