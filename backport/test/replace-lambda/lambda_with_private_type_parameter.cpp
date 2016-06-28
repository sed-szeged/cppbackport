// RUN: backport lambda_with_private_type_parameter.cpp -no-db -final-syntax-check

class A {
private:
	class B {
	private:
		class C {};
	public:
		void bar();
		void ged() {
			C c;
			[](C c){}(c);
		}
	};
public:
	void foo();
	void sna() {
		B b;
		[](B b){}(b);
	}
};

class D {};

void A::foo() {
	B b;
	D d;
	[](B b, D d){}(b, d);
}

void A::B::bar() {
	C c;
	D d;
	[](C c, D d){}(c, d);
}

int main() {
	A a;
	a.foo();
	return 0;
}