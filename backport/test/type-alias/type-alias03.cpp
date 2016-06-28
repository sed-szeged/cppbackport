// RUN: backport type-alias03.cpp -no-db -final-syntax-check

#include <iostream>
#include <vector>
#include <map>

template<class T, class U>
using mymap = std::map<T, U>;

template<class T>
using myvector = std::vector<T>;

// Not possible.
/*template<class T, class U>
using anonType = struct { myvector<T> asd; mymap<T, U> a; };*/

class Base
{
public:
	virtual ~Base() { }
};

template<typename T>
struct NestedTypeDef
{
    using type = T;
};

template<typename T>
using baseAlias = NestedTypeDef<T>;

myvector<baseAlias<float>::type> teszt;

namespace a {
    namespace b {
        template <typename T>
        class Derived : public Base
        {
        public:
            Derived();
            ~Derived();
            Derived(T param);
            Derived(T param, T param2) : member(param) { }
            
            int func();
            T member;
        };
        
        template<class T>
        using myderived = Derived<T>;

        template<>
        myderived<myvector<mymap<int, double> > >::Derived() {
        }
        
        // specialization
        template<>
        int myderived<myvector<mymap<int, double> > >::func() {
            return 52;
        }
    }
}

// If we transforms constructors in the general way than clang
// and visual studio 2005 throws an error:
// error: out-of-line definition of 'typename Derived<type-parameter-0-0>::Derived'
// does not match any declaration in 'Derived<T>'
template<class T>
::a::b::myderived<T>::Derived() {
}

template<class T>
a::b::myderived<T>::~Derived() {
}

template<class T>
a::b::myderived<T>::Derived(T param) : member(param) {
}

template<class T>
int a::b::myderived<T>::func() {
	return 42;
}

template<class T>
using simpleType = T;

// sizeof, if stmt
int main()
{
    baseAlias<float>::type floatVar = 3.14f;
    using namespace a::b;
	auto size = sizeof(myderived<int>);
	
	const simpleType<int> j = 3; // j is declared const
    simpleType<int>* pj = const_cast<simpleType<int>*>(&j);
	
	Base* basebase = new myderived<myderived<int>* >(new myderived<int>(64));
	Base* base = new myderived<int>();
	Base* base2 = new myderived<int>(64);
	Base* base3 = new myderived<int>(64, 64);
	Base basetemp = myderived<int>();
	const Base basetemp2 = myderived<int>(64);
	Base basetemp3 = myderived<int>(64, 64);
	myderived<int>* der = static_cast<myderived<int>* >(base);
	auto noconst = const_cast<myderived<int>&>(*der);
	der->member = 32;
	std::cout << der->member << std::endl;
	std::cout << ((myderived<int>*)base)->member << std::endl;
	if (dynamic_cast<myderived<int>* >(base)->member != 32)
		return -1;
	return 0;
}
