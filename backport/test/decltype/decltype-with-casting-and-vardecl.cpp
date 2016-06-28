// RUN: backport decltype-with-casting-and-vardecl.cpp -no-db -final-syntax-check

int main() {
	double x = 3.4;
	long l = 4;
	
	decltype(x*l) k = x*l;
	
	int c = (decltype(c))((decltype(l))(x) + 0.8) ;
	
	if(c != 3)
		return -1;
	
	unsigned long  size = sizeof(decltype(x));
	
	
	return 0;
}