#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <map>
#include <set>

#include "C.h"

using namespace std;

int main(int argc, char** argv){
    vector<string> myVec;

    C someVar;
    someVar.printOut();
    myVec.push_back("Item #1");
    myVec.push_back("Item #2");
    myVec.push_back("Item #3");
    myVec.push_back("Item #4");
    myVec.push_back("Item #5");

    for (auto i = 0; i < 10; i++){
        cout << (((i % 3) == 2) ? i : 0) << endl;
    }

    for( auto s: myVec) {
        cout << s << endl;
    }

    return 0;
}
