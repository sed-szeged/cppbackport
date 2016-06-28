#ifndef ALL_H
#define ALL_H

#include <iostream>

#define SOMESTR "Some string"
#define MAGIC 42
#define TEN 10
#define FIVE 5
#define SIX 6
#include "AllNew.h"

struct Useful {
public:
    static void doTheMagic(){
        auto i = TEN + FIVE;
        std::cout << " MAGIC" << std::endl;
        for (int a = 0; a < FIVE; a++){
            auto n1 = SIX;
            auto n2 = (char) TEN;
            auto n3 = (unsigned int) FIVE;
            std::cout << n1 << ". magic " << a << std::endl;
        }
    }
    
    static int returnTen() {
        return TEN;
    }

    static int returnFive() {
        return FIVE;
    }
};

#endif
