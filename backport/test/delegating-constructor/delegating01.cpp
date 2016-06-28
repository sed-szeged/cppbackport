// RUN: backport delegating01.cpp -no-db -final-syntax-check

#include <string>

// complex to simple
class class_c {
public:
    int max;
    int min;
    int middle;

    class_c(int my_max) : middle(42) {
        max = my_max > 0 ? my_max : 10;
    }
    class_c(int my_max, int my_min) : class_c(my_max) {
        min = my_min > 0 && my_min < max ? my_min : 1;
    }
    class_c(int my_max, int my_min, int my_middle) : class_c (my_max, my_min){
        middle = my_middle < max && my_middle > min ? my_middle : 5;
    }
};

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

class Base {
    
};

// simple to more complex
class A : public Base{
public:
    A() : A(2){}
    A(int i) : A(i, 0){}
    A(int j, int i) : Base(), num1(j), num2(i) {
        num1=j;
        num2=i;
        average=(num1+num2)/2;
    }

    int num1;
    int num2;
    int average;
};

class HighlyDifferentParams : public Base{
public:
    HighlyDifferentParams() : HighlyDifferentParams(2){}
    HighlyDifferentParams(int asddfafasdfasd) : HighlyDifferentParams(asddfafasdfasd, 621){}
    HighlyDifferentParams(int llokpoi, int wqeqerqwetqew) : Base(), num1(llokpoi), num2(wqeqerqwetqew)




    {
        num1=llokpoi;
        num2=wqeqerqwetqew;
        average=(num1+num2)/2;
    }

    int num1;
    int num2;
    int average;
};

int main() {
    {
        A a;
        if (a.num1 != 2 && a.num2 != 0 && a.average != 1)
            return -1;
        A a2(32);
        if (a2.num1 != 32 && a2.num2 != 0 && a2.average != 16)
            return -2;
    }
    
    {
        HighlyDifferentParams a;
        if (a.num1 != 2 && a.num2 != 0 && a.average != 1)
            return -3;
        HighlyDifferentParams a2(32);
        if (a2.num1 != 32 && a2.num2 != 0 && a2.average != 16)
            return -4;
    }
    
    {
        class_a a;
        if (a.m_string != "" && a.m_int != 1)
            return -5;
        
        class_a a2("test");
        if (a2.m_string != "test" && a2.m_int != 1)
            return -6;
        class_a a3("test", 3);
        if (a2.m_string != "test" && a2.m_int != 3)
            return -7;
    }
    
    {
        class_c c(30);
        if (c.max != 30 && c.middle != 42)
            return -8;
        class_c c1(30, 5);
        if (c.max != 30 && c.min != 5 && c.middle != 42) 
            return -9;
        class_c c2(30, 5, 10);
        if (c.max != 30 && c.min != 5 && c.middle != 10)
            return -10;
    }
    
    return 0;
}
