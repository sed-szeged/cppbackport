// RUN: backport primitive-array.cpp -no-db -final-syntax-check

int main() {
    int array[100] = { 1, 2, 3, 0 };

    int sum = 0;

    for (auto k : array) {
        k = 1;
    }
    
    for (auto k : array) {
        sum += k;
    }

    if (sum == 6) {
        return 0;
    } else {
        return 1;
    }
}
