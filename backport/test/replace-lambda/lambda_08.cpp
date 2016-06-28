// RUN: backport lambda_08.cpp -no-db -final-syntax-check

#include <iostream>
#include <vector>
#include <algorithm>

int main()
{
    std::vector<int> row1; row1.push_back(1); row1.push_back(1); row1.push_back(1);
	std::vector<int> row2; row2.push_back(2); row2.push_back(2); row2.push_back(2);
	std::vector<int> row3; row3.push_back(3); row3.push_back(3); row3.push_back(3);
	std::vector< std::vector<int> > matrix; matrix.push_back(row1); matrix.push_back(row2); matrix.push_back(row3);
    
    std::for_each(matrix.begin(), matrix.end(),
        [](std::vector<int>& vec)
        {
            int k = 0;
            std::for_each([](std::vector<int>& v) { return v.begin(); }(vec), vec.end(), [&k](int& n) { n+=k; ++k;});
        } );
        
    auto lambda = [](int& n) { std::cout << n << " "; };
    
    std::for_each(matrix.begin(), matrix.end(),
        [&lambda](std::vector<int>& vec)
        {
            std::for_each(vec.begin(), vec.end(), lambda );
            std::cout << std::endl;
        } );

	return 0;
}
