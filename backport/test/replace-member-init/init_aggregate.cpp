// RUN: backport init_aggregate.cpp -no-db -final-syntax-check

#include <string>

class CC {
    int a;
    std::string b;
};

struct X {
    X(int x, int y, std::string z) : x(x), y(y), z(z) {}
    int x;
    int y;
    std::string z;
};

struct Aggregate {
    int x;
    int y;
    std::string z;
    int* dd;
    char c;
};

struct S {
    int a=5;
    int zs{32};
    X c = X(1,2,"b");
    X cc{1, 2, "b"};
    std::string zss{ "somestring" };
    Aggregate b{1,2,"b"}; // aggregate initialization
    Aggregate bb{1,2};
    Aggregate bbb{1};
    
    Aggregate y = Aggregate{1, 2, "b"}; // move constructor
    Aggregate z = {1, 2}; // aggregate initialization
    Aggregate zz = { 1, 2, "bb", new int(3) };
    Aggregate zzz = {1};

};

int main() {
    S s;
    if (s.y.x != 1 ||
        s.y.y != 2 ||
        s.y.z != "b" ||
        s.y.dd != NULL ||
        s.y.c != 0)
        return -1;

    if (s.z.x != 1 ||
        s.z.y != 2 ||
        s.z.z != "" ||
        s.z.dd != NULL ||
        s.z.c != 0)
        return -2;
        
    if (s.zz.x != 1 ||
        s.zz.y != 2 ||
        s.zz.z != "bb" ||
        *s.zz.dd != 3 ||
        s.zz.c != 0)
        return -3;
    return 0;
}