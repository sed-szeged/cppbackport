// RUN: backport function_overload_class_type.cpp -no-db -final-syntax-check

#include <utility>
#include <string>

namespace my {
	template <class T>
	T && move(T &x) {
		return static_cast<T&&>(x);
	}
	
}

template<typename t>
class my_foo {

    t k[10];
public:
	my_foo() {}
	
	my_foo(my_foo const &other) {for(unsigned i = 0; i< sizeof(k) / sizeof(k[0]); ++i) this->k[i] = other.k[i];}
	
	my_foo(my_foo &&other) {for(unsigned i = 0; i< sizeof(k) / sizeof(k[0]); ++i) this->k[i] = other.k[i];}

	my_foo operator = (my_foo const &other) {
		if(&other == this)
			return *this;
			
		for(int i = 0; i<10; ++i)
			k[i] = other.k[i];
		
		return *this;
	}
	
	my_foo operator = (my_foo &&other) {
		if(&other == this)
			return *this;
			
		for(int i = 0; i<10; ++i)
			k[i] = other.k[i];
		
		return *this;
	}
	
};


template<typename t>
class my_nontrivial_foo : my_foo<t>{

    t *_asdf;
public:
    my_nontrivial_foo() {
        _asdf = new t[10];
    }

    my_nontrivial_foo(const my_nontrivial_foo &other) {

        if (&other == this)
            return;

        _asdf = new t[10];

        for (int i = 0; i< 10; ++i) {
            _asdf[i] = other._asdf[i];
        }
    }

    my_nontrivial_foo(my_nontrivial_foo &&other) {

        if (&other == this)
            return;

        this->_asdf = other._asdf;

        other._asdf = 0;
    }

    virtual ~my_nontrivial_foo() {
        delete[] _asdf;
    }



    my_nontrivial_foo & operator =(my_nontrivial_foo const &other) {
        if (&other == this)
            return *this;

        delete[] _asdf;

        this->_asdf = new t[10];

        for (int i = 0; i < 10; ++i) {
            _asdf[i] = other._asdf[i];
        }

        return *this;
    }

    my_nontrivial_foo & operator =(my_nontrivial_foo  &&other) {
        if (&other == this)
            return *this;

        delete[] _asdf;

        this->_asdf = other._asdf;

        other._asdf = 0;

        return *this;
    }

};



int move = 0;
int copy = 0;

template<class T>
int f(const my_foo<T> &other) {
    ++copy;

    return 0;
}

template<class T>
int f(const my_nontrivial_foo<T> &other) {
    ++copy;

    return 0;
}


template <class T>
int f(my_nontrivial_foo<T> && other) {
    decltype(other) _ = my::move(other);


    ++move;
    return 0;
}

template <class T>
int f(my_foo<T> && other) {
    decltype(other) _ = my::move(other);


    ++move;
    return 0;
}

template<class T>
int g(my_foo<T> && asdf) {
    my_foo<T> _ = my::move(asdf);

    ++move;

    return 0;
}

template<class T>
int g(my_nontrivial_foo<T> && asdf) {
    my_nontrivial_foo<T> _ = my::move(asdf);

    ++move;

    return 0;
}

long long int multiplier = 1;

template <class K>
long long int test() {
    multiplier *= 10;
    copy = 0;
    move = 0;

    my_foo<K> a1;

    decltype(a1) a2 = my::move(a1);

    f(my_foo<K>());

    if (move != 1 && copy != 0)
        return -1 * multiplier;

    a2 = my_foo<K>();

    f(a2);

    if (move != 1 && copy != 1)
        return -2 * multiplier;

    g(my_foo<K>());

    if (move != 2 && copy != 1)
        return -3 * multiplier;


    return 0;
}

template <class K>
long long int testd() {
    multiplier *= 10;
    copy = 0;
    move = 0;

    my_nontrivial_foo<K> a1;

    decltype(a1) a2 = my::move(a1);

    f(my_nontrivial_foo<K>());

    if (move != 1 && copy != 0)
        return -1 * multiplier;

    a2 = my_nontrivial_foo<K>();

    f(a2);

    if (move != 1 && copy != 1)
        return -2 * multiplier;

    g(my_nontrivial_foo<K>());

    if (move != 2 && copy != 1)
        return -3 * multiplier;


    return 0;
}

int main() {

    long long int res = 0;

    if ((res = test<std::string>()))
        return res;



    if ((res = test<int>()))
        return res;


    if ((res = test<long double>()))
        return res;


    if ((res = test<double>()))
        return res;


    if ((res = test<short>()))
        return res;


    if ((res = test<char>()))
        return res;


    if ((res = test<bool>()))
        return res;

 /*   if ((res = test<my_foo<long double> >()))
        return res;

    if ((res = test<my_foo<std::string> >()))
        return res;


    if ((res = test<my_nontrivial_foo<long double> >()))
        return res;

    if ((res = test<my_nontrivial_foo<std::string> >()))
        return res;


    if ((res = test<my_foo<my_nontrivial_foo<long double> > >()))
        return res;

    if ((res = test<my_foo<my_nontrivial_foo <std::string>  > >()))
        return res;


    if ((res = test<my_nontrivial_foo<my_nontrivial_foo< long double> > >()))
        return res;

    if ((res = test<my_nontrivial_foo<my_nontrivial_foo < std::string> > >()))
        return res;

*/
    /*================================*/
    if ((res = testd<std::string>()))
        return res;



    if ((res = testd<int>()))
        return res;


    if ((res = testd<long double>()))
        return res;


    if ((res = testd<double>()))
        return res;


    if ((res = testd<short>()))
        return res;


    if ((res = testd<char>()))
        return res;


    if ((res = testd<bool>()))
        return res;

/*    if ((res = testd<my_foo<long double> >()))
        return res;

    if ((res = testd<my_foo<std::string> >()))
        return res;


    if ((res = testd<my_nontrivial_foo<long double> >()))
        return res;

    if ((res = testd<my_nontrivial_foo<std::string> >()))
        return res;


    if ((res = testd<my_foo<my_nontrivial_foo<long double> > >()))
        return res;

    if ((res = testd<my_foo<my_nontrivial_foo <std::string>  > >()))
        return res;


    if ((res = testd<my_nontrivial_foo<my_nontrivial_foo< long double> > >()))
        return res;

    if ((res = testd<my_nontrivial_foo<my_nontrivial_foo < std::string > > >()))
        return res;

*/


    return 0;
}
