// RUN: backport lambda_function_pointer_parameters.cpp -no-db -final-syntax-check

void RunSafely(bool (*Fn)(double*), int *UserData) {
    [&](double* (*Gn)(int*)) { Fn(Gn(UserData)); return Gn; };
	[](double* (*Gn)(int*)) { return Gn; };
}

int main() {
    return 0;
}
