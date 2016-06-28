#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <map>
#include <set>

#include "A.h"

using namespace std;

int main(int argc, char** argv){
    A someVar;
    someVar.replaceA("Hey");
    vector<string> myVec;
    myVec.push_back(someVar.a);
    myVec.push_back("Item #2");
    myVec.push_back("Item #3");
    myVec.push_back("Item #4");
    myVec.push_back("Item #5");

    for( auto s: myVec) {
        cout << s << endl;
    }
    
    for (auto i = 0; i < MAGIC; i++){
        auto valtozo = (((i % 3) == 2) ? i : 0);
        cout << valtozo << endl;
    }

    return 0;
}
