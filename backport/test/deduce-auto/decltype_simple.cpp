// RUN: backport decltype_simple.cpp -no-db -final-syntax-check

namespace bar {
    struct A {};
    auto foo(A& a) -> decltype(a);
}

auto bar::foo(A& a) -> decltype(a) { return a; }

auto foo(float a, int b) -> decltype(a*b) { return a * b; }

class SampleClass
{
	auto foo(float a, int b) -> decltype(a*b) { return a * b; }
	virtual auto foo2(float a, int b) -> decltype(a*b) { return a * b; }
	virtual auto foo3(float a, int b) -> decltype(a*b) { return a * b; }
	
	static auto foostatic(float a, int b) -> decltype(a*b) { return a * b; }
};

//auto getTrue() -> bool { return true; }

int main() {
    bar::A a;
    foo(a);
    return 0;
}
