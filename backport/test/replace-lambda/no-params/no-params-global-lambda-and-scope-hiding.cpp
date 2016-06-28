// RUN: backport no-params-global-lambda-and-scope-hiding.cpp -no-db -final-syntax-check

static auto i = 9876543210ULL;

auto glblambd = [] {
    return --i;
};

static auto glbslambd = []{ return --i; };

int main() {
    auto local = 0;
    static auto glbslambd_loc = [&]{local++; return --i + local; };
    auto res = [&] {glblambd(); glbslambd(); return glbslambd_loc(); }();

    if (res != (9876543210ULL - 2 - 1 + (0 + 1))) {
        return -1;
    }

    {
        static auto local = -9888888;
        static auto glbslambd_loc = [&]{local++; return --i + local; };
        auto res = [&] {glblambd(); glbslambd(); return glbslambd_loc(); }();
        if (res != (9876543210ULL - 3 - 2 - 1 + (-9888888 + 1))) {
            return -1;
        }
    }

    return 0;
}
