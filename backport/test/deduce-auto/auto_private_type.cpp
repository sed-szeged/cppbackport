// RUN: backport auto_private_type.cpp -no-db -final-syntax-check

class A {
	class B {};
	class C {};
public:
	typedef C D;
	B getB() { return B(); }
};

int main() {
	A::D d;
	auto d2 = d;
	
	A a;
	auto b = a.getB();
	
	return 0;
}