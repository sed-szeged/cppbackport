// RUN: backport init_template.cpp -no-db -final-syntax-check

#include <iostream>
#include <vector>
#include <map>
#include <set>

struct SampleStruct {
    SampleStruct() {}
    SampleStruct(int n) : n(n) {}
    int n = 3;
    std::string s = "text";
};

class SeparatedDefinition {
public:
    SeparatedDefinition();

    int sampleInt = 32;
    double sampleDouble = 3.2023;
    const char * cchar = "text";
};

SeparatedDefinition::SeparatedDefinition() {}

template<typename T>
class TemplateClassWithoutConstructors {
    int someint = 32/*, someint2 = 32*/;
    T member = T(5);
};

template< template<typename> class V> class C {
    V<int> y;
    V<int*> z; // uses the partial specialization
};

template<typename T>
struct MultipleInheritance : public TemplateClassWithoutConstructors<T>, public SeparatedDefinition, public SampleStruct {
    double d = 3.02;
    float f = 3.02f;
};

class Base {
public:
    int tmp = 23;
};

template <typename T>
class DerivedExtDef : public Base {
public:
    DerivedExtDef();
    DerivedExtDef(int c) {}
    DerivedExtDef(int a, std::string b) : Base(), aa(a) { str2 = b; }
    DerivedExtDef(const DerivedExtDef& other) {}
public:
    int a = 32;
    int aa = 2132;
    const char* str = "text";
    std::string str2 = "text";
    T b = T(5);
};

template <typename T>
DerivedExtDef<T>::DerivedExtDef() {
}

template <typename T>
class TemplateNoCall : public Base {
public:
    TemplateNoCall() : Base() {}
    TemplateNoCall(int c);
    TemplateNoCall(int a, std::string b) : Base(), aa(a) { str2 = b; }
    TemplateNoCall(const TemplateNoCall& other) {}
public:
    int a = 32;
    int aa = 2132;
    const char* str = "text";
    std::string str2 = "text";
};

template <typename T>
TemplateNoCall<T>::TemplateNoCall(int c) {
}

template<class T>
struct TemplateClassWithTemplateMember
{
public:
    T tmember = T(5);
    T imember = T(2);
    std::vector<T> *vec = new std::vector<T>(5, 10);
    std::map<T, int> *map = new std::map<T, int>;
    std::set<T*> *set = new std::set<T*>();
    T t2member = T(10);
    
    void magic() {}
    TemplateClassWithTemplateMember() {}
 
    template<class U>
    TemplateClassWithTemplateMember(U uparam) : imember(uparam) {}
};

template<>
struct TemplateClassWithTemplateMember<long>
{
public:
    long tmember = 5;
    int imember = 2;
    std::vector<long> *vec = new std::vector<long>();
 
    TemplateClassWithTemplateMember() {}
 
    template<class U>
    TemplateClassWithTemplateMember(U uparam) : imember(uparam) {}
    
    void magic() {}
};

int main() {
    DerivedExtDef<int> dv; // only the called constructors will be transformed
    DerivedExtDef<float> dv2(12, "test");
    DerivedExtDef<float> dv3 = dv2;
    
    DerivedExtDef<unsigned> dv4;
    DerivedExtDef<unsigned> dv5 = dv4;
    TemplateClassWithTemplateMember<int> tmp;
    TemplateClassWithTemplateMember<float> tmp3(3.02f);
    TemplateClassWithTemplateMember<float> tmp4(32);
    TemplateClassWithTemplateMember<long> tmp2(32);
    return 0;
}
