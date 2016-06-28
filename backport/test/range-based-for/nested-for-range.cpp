// RUN: backport nested-for-range.cpp -no-db -final-syntax-check

#include <vector>
#include <list>

auto v = new std::vector<std::list<int>*>();
auto va = new std::vector<std::list<int> >();

int main() {
    v->push_back(new std::list<int>());
    va->push_back(std::list<int>());

    (*v)[0]->push_back(2); (*va)[0].push_back(2);
    (*v)[0]->push_back(3); (*va)[0].push_back(7);
    (*v)[0]->push_back(4); (*va)[0].push_back(5);
    (*v)[0]->push_back(5); (*va)[0].push_back(4);
    (*v)[0]->push_back(6);

    v->push_back(new std::list<int>());
    va->push_back(std::list<int>());

    (*v)[1]->push_back(1); (*va)[1].push_back(4);
    (*v)[1]->push_back(0); (*va)[1].push_back(8);
    (*v)[1]->push_back(9); (*va)[1].push_back(1);
    (*v)[1]->push_back(7);

    v->push_back(new std::list<int>());
    va->push_back(std::list<int>());

    (*v)[2]->push_back(3); (*va)[2].push_back(5);
    (*v)[2]->push_back(4); (*va)[2].push_back(4);
    (*v)[2]->push_back(4);
    (*v)[2]->push_back(5);
    (*v)[2]->push_back(5);

    v->push_back(new std::list<int>());
    va->push_back(std::list<int>());

    (*v)[3]->push_back(1); (*va)[3].push_back(4);
    (*v)[3]->push_back(4); (*va)[3].push_back(8);
    (*v)[3]->push_back(3); (*va)[3].push_back(2);
    (*v)[3]->push_back(0);

    v->push_back(new std::list<int>());
    va->push_back(std::list<int>());

    v->push_back(new std::list<int>());
    va->push_back(std::list<int>());

    v->push_back(new std::list<int>());
    va->push_back(std::list<int>());

    int sum = 0;

    for (auto i : *v) {
        for (auto k : *i) {
            sum += k;
        }
    }
    
    int mul = 1;

    for (auto i : *va) {
        for (auto k : i) {
            mul *= k;
        }
    }

    if ((sum == 66) && (mul == 11468800)) {
        return 0;
    } else {
        return 1;
    }
}
