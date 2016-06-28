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

    static void doTheMagic(int n){
        int first = 0, second = 1, next, c;
        for ( c = 0 ; c < n ; c++ ) {
          if ( c <= 1 )
             next = c;
          else {
             next = first + second;
             first = second;
             second = next;
          }
        }
        std::cout << next << std::endl;
    }

    static int returnFive() {
        return FIVE;
    }
};

#endif
