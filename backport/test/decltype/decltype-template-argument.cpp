// RUN: backport decltype-template-argument.cpp -no-db -final-syntax-check

template <class A>
class C {
public:
	A value;
}; 

int main() {
	double x = 3.4;
	
	C<decltype(x)> b;
	
	C<decltype(b)> d;
	
	C<decltype(C<decltype(d)>())> f;
	
	return 0;
}