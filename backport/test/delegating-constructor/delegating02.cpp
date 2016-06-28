// RUN: backport delegating02.cpp -no-db -final-syntax-check

#include <iostream>
#include <string>

// outside constructor definition
class class_c {
public:
    int max;
    int min;
    int middle;

    class_c(int my_max);
    class_c(int my_max, int my_min);
    
    class_c(int my_max, int my_min, int my_middle) : class_c (my_max, my_min){
        middle = my_middle < max && my_middle > min ? my_middle : 5;
    }
};

class_c::class_c(int my_max) : middle(42) {
        max = my_max > 0 ? my_max : 10;
}
    
class_c::class_c(int my_max, int my_min) : class_c(my_max) {
        min = my_min > 0 && my_min < max ? my_min : 1;
}
    
class class_a {
public:
    class_a() {}
    // member initialization here, no delegate
    class_a(std::string str) : m_string(str) {}

    // only member assignment
    class_a(std::string str, int dbl) : class_a(str) { m_int = dbl; }
	
    int m_int = 1;
    std::string m_string;
};

// outside definition
namespace a {
    namespace b {
        class Base {
        public:
            Base() : Base(32, new std::string("test")) { }
            Base(int i, std::string *str);
            
            int i;
            std::string *str;
        };
    }
}

a::b::Base::Base(int i, std::string *str) : i(i), str(str) {}

class Base2 {
public:
    Base2(int *i, a::b::Base* b);
    
    int *ii;
    a::b::Base *b;
};

Base2::Base2(int *i, a::b::Base* b) : ii(i), b(b) {}

// multiple parameter base constructor
class A : public a::b::Base{
public:
    A() : A(2){}
    A(int i) : A(i, 0){}
    A(int j, int i) : a::b::Base(64, new std::string("object")), num1(j), num2(i) {
        num1=j;
        num2=i;
        average=(num1+num2)/2;
    }

    int num1;
    int num2;
    int average;
};

// multiple inheritance
class B : public a::b::Base, public Base2 {
public: 
    B() : B(32, "500") { }
    B(int i, std::string str ) : a::b::Base(i, new std::string(str)), Base2(new int(i), new a::b::Base(i, new std::string("base2"))) { }
};


int main() {
    
    {
        a::b::Base b;
        if (b.i != 32 && (*b.str) != "test")
            return -1;
    }
    
    {
        B b;
        if (b.i != 32 && 
            (*b.ii) != 32 &&
            (*b.str) != "500" &&
            b.b->i != 32 &&
            (*b.b->str) != "500")
            return -2;
    }
    
    {
        class_c c(30);
        if (c.max != 30 && c.middle != 42)
            return -3;
        class_c c1(30, 5);
        if (c.max != 30 && c.min != 5 && c.middle != 42) 
            return -4;
        class_c c2(30, 5, 10);
        if (c.max != 30 && c.min != 5 && c.middle != 10)
            return -5;
    }
    
    {
        A a;
        if (a.num1 != 2 && a.num2 != 0 && a.average != 1)
            return -6;
        A a2(32);
        if (a2.num1 != 32 && a2.num2 != 0 && a2.average != 16)
            return -7;
    }
    
    {
        class_a a;
        if (a.m_string != "" && a.m_int != 1)
            return -8;
        
        class_a a2("test");
        if (a2.m_string != "test" && a2.m_int != 1)
            return -9;
        class_a a3("test", 3);
        if (a2.m_string != "test" && a2.m_int != 3)
            return -10;
    }
    
    return 0;
}