// RUN: backport lambda_decay_to_function_poitner.cpp -no-db -final-syntax-check

int foo(int (*function)(int k), int l) {
	return function(l);
}

int main() {
	static int x = 7;
	auto lambda = [](int k) -> int { return (k + x); };
	return ((foo(lambda, 20) == 27) ? 0 : 1);
}