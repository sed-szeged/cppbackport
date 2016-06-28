// RUN: backport decltype_template_static_var_initilize_function_ptr_temp_par.cpp.cpp -no-db -final-syntax-check

int car;
void f(decltype(car) * param, long &&) {}
void g(int * param, long &&) {}

template <decltype(f) *func_ptr> 
class A {
	public:
	static decltype(func_ptr) m_ptr;	
	
	static long ghf(decltype(func_ptr) my_god) {
		auto k = new auto(5);
		
		my_god(k, 6);
		return 5;
	}
};

template<decltype(f) *func_ptr>
decltype(func_ptr) A<func_ptr>::m_ptr = func_ptr;

int main() {
	A<f> m;
	
	unsigned long const i = reinterpret_cast<decltype(i)>(A<f>::m_ptr);
	A<f>::ghf(f);
	A<f>::ghf(g);
	
	A<g> gm;
	
	unsigned long const ig = reinterpret_cast<decltype(i)>(A<g>::m_ptr);
	A<g>::ghf(g);
	A<g>::ghf(f);
	
	
	return 0;
}
