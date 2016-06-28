// RUN: backport self-made-iterator-vector-global.cpp -no-db -final-syntax-check

// forward-declaration to allow use in Iter
class IntVector;

class Iter {
public:
    Iter(const IntVector* p_vec, int pos) : _pos(pos) , _p_vec(p_vec) {}

    // these three methods form the basis of an iterator for use with
    // a range-based for loop
    bool operator!= (const Iter& other) const {
            return _pos != other._pos;
    }

    // this method must be defined after the definition of IntVector
    // since it needs to use it
    int operator* () const;

    const Iter& operator++ () {
        ++_pos;
        // although not strictly necessary for a range-based for loop
        // following the normal convention of returning a value from
        // operator++ is a good idea.
        return *this;
    }

private:
    int _pos;
    const IntVector *_p_vec;
};

class IntVector
{
public:
    IntVector() {}

    int get(int col) const {
        return _data[col];
    }

    void set(int index, int val) {
        _data[index] = val;
    }

private:
    int _data[100];
};

Iter begin(const IntVector& iv) {
    return Iter(&iv, 0);
}

Iter end(const IntVector& iv) {
    return Iter(&iv, 100);
}

int Iter::operator* () const {
    return _p_vec -> get(_pos);
}

// sample usage of the range-based for loop on IntVector
int main() {
    IntVector v;
    for (int i = 0; i < 100; i++) {
        v.set(i, i);
    }
    
    int sum = 0;
    
    for (int i : v) {
        sum += i;
    }
    
    if (sum == 4950) {
        return 0;
    } else {
        return 1;
    }
}

