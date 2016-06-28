// RUN: backport unnamed.cpp -no-db -final-syntax-check

struct {
	int a;
	double b;
	union {
		bool b_;
		long long k;
		float kt[2];
	} k;
	
	
	typedef unsigned helper;
} a;



namespace test {
	
	namespace {
		struct {
	int a;
	double b;
	union {
		bool b_;
		long long k;
		float kt[2];
	} k;
	
	
	typedef unsigned helper;
} a;
	}
	void f() {
			auto k = [](){ return a; };
	
	k();
	
	auto k2 = [](decltype(a) k) {return k;};
	
	k2(a);
	
	
	auto k3 = [](decltype(a)::helper k) {return k;};
	
	k3((unsigned)a.a);
	
	auto k4 = [](decltype(a.k) k) {return k;};
	
	auto i = k4(a.k);
	
		
	}
	
	namespace test2 {
		
		struct {
	int a;
	double b;
	union {
		bool b_;
		long long k;
		float kt[2];
	} k;
	
	
	typedef unsigned helper;
} a;
	}
}

int main() {
	
	test::f();
	auto k = [](){ return a; };
	
	k();
	
	auto k2 = [](decltype(a) k) {return k;};
	
	k2(a);
	
	
	auto k3 = [](decltype(a)::helper k) {return k;};
	
	k3((unsigned)a.a);
	
	auto k4 = [](decltype(a.k) k) {return k;};
	
	auto i = k4(a.k);
	
	
	
	
	auto k_ = [](){ return test::test2::a; };
	
	k_();
	
	auto k2_ = [](decltype(test::test2::a) k) {return k;};
	
	k2_(test::test2::a);
	
	
	auto k3_ = [](decltype(test::test2::a)::helper k) {return k;};
	
	k3_((unsigned)test::test2::a.a);
	
	auto k4_ = [](decltype(test::test2::a.k) k) {return k;};
	
	auto i_ = k4_(test::test2::a.k);
	
	
	return 0;
	
}