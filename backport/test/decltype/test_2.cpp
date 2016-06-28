// RUN: backport test_2.cpp -no-db -final-syntax-check

const int foo2() {return 30;}

const int &&foo3(decltype(foo2) *parameter);

template <class K =  decltype(foo3) * *>
class A {};


int main() {
	const int&& foo();
	const int bar();
	int i;
	double u;
	struct A { double x; };
	const A* a = new A();
	decltype(foo()) x1 = 5; // type is const int&&
	decltype(bar()) x2; // type is int
	decltype(i) x3; // type is int
	
	decltype(a->x) x4; // type is double
	decltype((a->x)) x5 = u; // type is const double&
	decltype(bar) *x6 = &foo2;
	decltype(bar) x7;
	
	decltype(x6) &x8 = x6;
	decltype(x7) &x9 = x7;
	
	const int &&foo4(decltype(x9) &parameter);
	
	const int &&foo5(void (*parameter)(decltype(x7) *) );
	
	return 0;
}

const int x7() {return 0;}

int lasdf;

const int &&foo3(decltype(foo2) *parameter) {
	parameter();
	return static_cast<int &&>(lasdf);
}