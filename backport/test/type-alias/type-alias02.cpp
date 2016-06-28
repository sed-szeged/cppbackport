// RUN: backport type-alias02.cpp -no-db -final-syntax-check
#include <string>
#include <vector>
#include <map>

using myint = int;


template<class T = float, int U = 32> using Vec = std::vector<T>; // type-id is vector<T, Alloc<T>>

// type alias in type alias
template<class T> using mapVec = std::map<T, Vec<T> >;
template<class T> using mapVecVec = std::map<T, Vec<Vec<T> > >; // at this nesting level with same type names regex might fail.

// type alias in typedef
typedef mapVec<int> mapVecTypedef;

// type alias in type alias
using mapVecUsing = mapVecVec<int>;

mapVecVec<int> asd;

//  Unsupported
/*template<typename T>
class TT {
    static const T a;
};

template<typename T>
using asdasd = decltype(TT<T>::a);*/

// Template type alias does not accept unnamed types.
//using S = struct { mapVec<int> a; std::vector<mapVecVec<double> > b; mapVecVec<int> (****functionP)(mapVec<double>, Vec<float>); };

// function signatures with type aliases
using functionNoProto = mapVecVec<int>();
template<typename T>
using functionPointerNoProto = mapVec<T> (*)();


typedef mapVecVec<int> function_type(mapVec<int>, Vec<int>);
using function_type_using = mapVecVec<int>(mapVec<int>, Vec<int>);
typedef mapVecVec<int> (*function_pointer)(mapVec<int>, Vec<int>);
using function_pointer_using =  mapVecVec<int> (*)(mapVec<int>, Vec<int>);

template<typename T>
using template_function_type = mapVecVec<T>(mapVec<T>, Vec<T>);

template<typename T>
using template_function_pointer =  mapVecVec<T> (****&&)(mapVec<T>, Vec<T>);

template<typename T>
using template_function_pointer_ =  mapVecVec<T> (****&)(mapVec<T>, Vec<T>);


using function_pointer_nested_using = template_function_pointer_<double> (*)(mapVecVec<int> (*)(mapVec<int>, mapVecVec<int> (***)(mapVec<int>, Vec<int>)), std::vector<Vec<int> >);

template <
Vec<mapVecVec<double> >* tempmyVec,
template_function_pointer_<double>
(*nonTypeFunctionPointerArgument)
(mapVecVec<int> (*)(mapVec<int>, // ParmVarDecl
mapVecVec<int> (***)(mapVec<int>, // ParmVarDecl
Vec<int>)),// ParmVarDecl
std::vector<Vec<int> >) // ParmVarDecl
>
void nonTypeFunctionTemplate() {
}

// array
template<typename T>
using t_array = T*[10];

template<typename T>
using t_array2 = T[][5][2];

template<typename T, int U>
using t_array3 = T[U];


// Template class specialization with template type alias.
template<class T>
class A {T k;};

template<class K>
using vk = std::vector<K>;

template<>
class A<vk<long> > { long k; };

template<typename T, typename U>
void retFuncSpec() { T tmp; }

template<>
void retFuncSpec<vk<long>, mapVecVec<mapVec<int> > >() { long tmp; }

Vec<int, 32> v; // Vec<int> is the same as vector<int>
Vec<> v2;

// template type alias
template<class T> using ptr = T*;
// the name 'ptr<T>' is now an alias for pointer to T
ptr<mapVec<ptr<int> > > x;

static Vec<> v3;

// type alias used to hide a template parameter
template <class CharT> using mystring =
    std::basic_string<CharT,std::char_traits<CharT> >;
mystring<char> str;

template <class T, class U> using mymap = std::map<T, U>;

mymap<std::string, myint> aMap;
std::map<std::string, int> aMap2; // shouldn't match

mymap<std::string, ptr<double> >* aMap3; // nested template type alias
::std::map<std::string, ptr<int> > aMap4; // should replace because of prt<int>
std::map<std::string, ::ptr<int> > aMap5;

mymap<std::string, ptr<double>* > aMap6 = mymap<std::string, ptr<double>* >();
std::map<std::string, std::map<int, ptr<double> > > aMap7;

template<class T, class U = double, typename PoinT = mymap<int, double>(mapVec<mapVecVec<double> >, mymap<int, double>(**)(mapVec<int>, mapVecVec<double>)), typename = mymap<int, double>(**)(mapVec<int>, mapVecVec<double>)>
class SampleClassTemplate
{
    template <class AliasArg> using inClassAlias = AliasArg*;
    static ptr<int> sa;
    ptr<int> a;
    inClassAlias<T> b;

public:
    SampleClassTemplate() { }
    SampleClassTemplate(mymap<T, U> par) { }
    SampleClassTemplate(ptr<int> param) { }

    int fooFunc(ptr<int> paramFoo) { return *paramFoo; }

    mymap<int, ptr<T>*> fooFunc2(mapVecVec<T> (****&funcptr)(mapVec<T>, Vec<T>)) { return mymap<int, ptr<T>*>(); }
    mymap<int, ptr<T>*>* fooFunc22(ptr<int> a) { return new mymap<int, ptr<T>*>(5, 2); }
    mymap<int, double> fooFunc3(ptr<int> a) { return mymap<int, double>(); }
    virtual ptr<T> const fooFunc4(ptr<int> a) { return NULL; }
    ptr<T> const* fooFunc5(ptr<int> a) { return NULL; }
    const mymap<int, ptr<double>*> fooFunc6(ptr<int> a) { }
    mymap<int, ptr<double> > fooFunc7(ptr<int> a);

    template <typename TT>
    ptr<TT> fooFuncTemplate(ptr<TT> paramFooTemp) { }

    template <typename TT>
    int fooFuncTemplate2(ptr<TT>* param) { }

    static ptr<int> fooStatic(ptr<long>* param) { }
};

::SampleClassTemplate<int> tc1; // elaborated type :(
SampleClassTemplate<ptr<int> > tc2;
SampleClassTemplate<myint> tc3;
::SampleClassTemplate<ptr<int> > tc4;

class SampleClass
{
    static const volatile ptr<int> sa;
    const ptr<int> a;

public:

    SampleClass(ptr<int> param) : a(NULL) { }

    int fooFunc(ptr<int> paramFoo) { return 0; }

    const mymap<int, ptr<int>*> fooFunc2(ptr<int> a) { return mymap<int, ptr<int>*>(); }
    mymap<int, double> fooFunc3(ptr<int> a) { return mymap<int, double>(); }
    virtual ptr<long> fooFunc4(ptr<int> a) { return new long(300000); }
    virtual const ptr<double> fooFunc5(ptr<int> a) const { return new double(3.02); }
    const mymap<int, ptr<double>*> fooFunc6(ptr<int> a) { return mymap<int, ptr<double>*>(); }
    mymap<int, ptr<double> > fooFunc7(ptr<int> a) { return mymap<int, ptr<double> >(); }

    template <typename TT>
    ptr<TT> fooFuncTemplate(ptr<TT> const paramFooTemp) { }

    template <typename TT>
    int fooFuncTemplate2(ptr<TT>* param) { }

    static ptr<int> fooStatic(ptr<long>* param) { return new int(3); }
};


// type alias used to simplify the syntax of std::enable_if
template <typename T> using Invoke =
    typename T::type;

template <typename T = ptr<int>, typename = mymap<int, double>*, typename = ptr<long>& >
class TemplateMagic
{
    T t1;
};

void func(mymap<int, char>* p1, ptr<long> p2) {
}

template <typename T>
mymap<int, T> retFunc() {

}

template<>
mymap<int, mapVec<double> > retFunc<mapVec<double> >() {
    return mymap<int, mapVec<double> >();
}

template <typename T>
::ptr<int> retFunc2(ptr<int> param) {
    
}

mymap<int, double> typealiasreturnfunct(ptr<int> p1) {
    return mymap<int, double>();
}

static mymap<int, double> staticfunc(ptr<long> p2) {
    return mymap<int, double>();
}

int main()
{
    int asd = 32;
    tc1.fooFunc(&asd);

    {
        mapVec<int> mapVecEx;
        mapVecEx[32].push_back(64);
    }
    return 0;
}
