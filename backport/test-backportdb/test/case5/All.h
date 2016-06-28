#ifndef ALL_H
#define ALL_H

#include <iostream>
#include "IncAll.h"

#define SOMESTR "Some string"
#define MAGIC 42
#define TEN 10
#define FIVE 5

struct Useful {
public:
    static void doTheMagic(){
        auto i = TEN;
        std::cout << " MAGIC" << std::endl;
        for (int a = 0; a < FIVE; a++){
            auto n1 = getSix();
            auto n2 = (char) TEN;
            auto n3 = (unsigned int) FIVE;
            std::cout << getSix() << ". magic " << a << std::endl;
        }
    }


    static int returnFive() {
        return FIVE;
    }
};

#endif
