// RUN: backport simple_multiple_type.cpp -no-db -final-syntax-check
// Contains test cases for auto type deduction in global scope, multiple declarations, function pointer, "new auto"

#include <iostream>
#include <vector>

template<class T>
class TmpClass {
public:
    TmpClass() : a(3) {}
    T c;
    double a;
    static const auto sci = 3;
    static const auto scl = 300000000000000LL;
    static const auto scc = 'A';
    static const auto scb = false;
    static const auto scs = (short)3;
};

union Union {
    int a;
    float c;
    //static const auto sci = 3; // invalid 
    //static const auto scl = 300000000;
    //static const auto scc = 'A';
    //static const auto scb = false;
    //static const auto scs = (short)3;
};

struct TmpStruct {
    int attr;
    static const auto sci = 3;
    static const auto scl = 300000000000000LL;
    static const auto scc = 'A';
    static const auto scb = false;
    static const auto scs = (short)3;
};

template <class T>
T foo(T t) { return t; }

template<class T, class U> void multiply(const std::vector<T>& vt, const std::vector<U>& vu) {
    // ...
    auto str = "str";
    // ...
}

template<> void multiply<int, float>(const std::vector<int>& vt, const std::vector<float>& vu) {
    // ...
    auto str = "str";
    auto ttmp = vt[0] * vu[0];
    auto ttmp2 = vt[1];
    auto &ttmp3 = vt[1];
    auto &ttmp4 = vt;
    // ...
}

int f(int x) {
    return x;
}

static auto gsti = 64;
static auto gstd = 3.02;

auto gvi = 32313213;
auto *gpi = &gvi;
auto &gri = gvi;
auto **ppvi = new /*wololo*/ auto(&gvi);
auto pvi2 = new auto (&gvi);
auto pvi3 = new auto  (gvi);
auto pvd = new   auto(3.02);
auto pvf = new                   auto(3.02f);
auto pvb = new
 auto(false);
auto gvf = 3.02f;
auto gvd = 3.02;
auto gfp = f;
int(&gfr)(int x) = f;
auto gfp2 = gfr;
auto gbb = false;

int main() {
    {
        volatile auto voli = 32;
        auto volatile voli2 = 634, voli3 = 683;
        auto * pi = &voli;
        auto lbb = true;
        auto lbb2 = true, lbb3 = false, lbb4 = true;
    }

    {
    double dx = 12.34;
    auto * pa = new     auto(&dx);
    auto * pb = new 
    auto(dx);
    auto * pc = new auto(2.034);
    auto *py = new auto(dx), **z = (new auto(&dx));
}

    {
        auto x = 1;
        auto xp = &x, yp = xp;
        auto *y = &x, **z = &y; // Resolves to int.
        auto a(2.01), *b(&a);         // Resolves to double.
        auto c = 'a', *d(&c);          // Resolves to char.
        auto m = 1, &n = m;            // Resolves to int.
        auto &k = y;
        auto &rk = x;
        auto const &crk = x;
        const auto kk = y;
    }

    {
        auto x = f(0);
        const auto & y = f(1);
        int(*p)(int x);
        p = f;
        auto fp = p;
        
        int(&p2)(int x) = f;
        auto fp2 = p;
    }

    {
        static auto lsmti = 64, l2smti = 32, l3smti(64);
        static auto lsti = 128;

    }

    static const int arri[] = { 3, 2, 5, 6, 10 };
    static const float arrf[] = { 3, 2, 5, 6, 10 };
    static const long arrl[] = { 3, 2, 5, 6, 10 };
    auto tmp = std::vector<int>(arri, arri + sizeof(arri) / sizeof(arri[0])); // std::vector<int>
    auto tmp2 = std::vector<float>(arrf, arrf + sizeof(arrf) / sizeof(arrf[0]));
    auto tmp3 = std::vector<int>(arri, arri + sizeof(arri) / sizeof(arri[0]));
    auto tmp4 = std::vector<long>(arrl, arrl + sizeof(arrl) / sizeof(arrl[0]));

    //good : auto increases readability here
    //v is some [std] container
    for (auto it = tmp.begin(); it != tmp.end(); ++it) {
        //..
    }

    for (auto i = 0; i < 5; i++)
        std::cout << i << std::endl;

    auto test = 13; // int
    auto test2 = 1231231.123123f; // float
    auto test3 = -103123123; // int
    auto test4 = "somestring"; // const char*
    auto test5 = 132341452345245LL;
    auto test6 = 13.321312; // double
    auto test7 = new TmpClass<int>(); // TmpClass<int>
    auto test8 = Union(); // Union
    auto test9 = TmpStruct(); // TmpStruct
    auto test10 = std::string("somestring"); // std::string
    auto var1 = 321;
    auto test11 = foo(test); // int
    auto test12 = foo(test8); // Union
    multiply(tmp, tmp2);
    multiply(tmp, tmp3);
    multiply(tmp, tmp4);
    auto var2 = 321;

    return 0;
}
