// RUN: backport new_decltype.cpp -no-db -final-syntax-check

class A {
	int *data;
	public:
	A() {
		data = new int[10];
	}
	
	A(A const &o) {
		data = new int[10];
		
		for(int i = 0; i< 10; ++i) {
			data[i] = o.data[i];
		}
		
	}
	
	A(A &&o) {
		data = o.data;
		o.data = 0;
	}
	
	A &operator = (A const &other) {
		if(&other == this)
			return *this;
		
		delete[] data;
		data = new int[10];
		for(int i = 0; i<10; ++i) {
			data[i] =  other.data[i];
		}
		
		return *this;
	}
	
	A &operator = (A && other) {
		if(&other == this)
			return *this; 
		data = other.data;
		other.data = 0;
		
		return *this;
	}
};

const A foo2() { return A(); }

A glob;

const A &&foo3(decltype(foo2) *parameter);

template <class K =  decltype(foo3) * *>
class B {};

B<decltype(foo2) *> m;
B<> m2;

int main() {
	
	auto k = new decltype(m)();
	
	delete k;
	
	decltype(&foo2) k2 = &foo2;
	
	auto k3 = new decltype(m2)();
	
	delete k3;
	
	return 0;
}


const A &&foo3(decltype(foo2) *parameter) {
	glob = parameter(); return static_cast<A&&>(glob);
}