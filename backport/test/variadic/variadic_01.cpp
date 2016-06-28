// RUN: backport variadic_01.cpp -no-db -final-syntax-check

#include <sstream>
#include <iostream>
#include <string>

bool VarPrint(std::ostringstream& out, const std::string& s) {
    std::string::size_type offset = 0;
    if ((offset = s.find("%")) != std::string::npos) {
        if (!(offset != s.size() - 1 && s[offset + 1] == '%')) {
            return false;
        }
    }
    out << s;
    return true;
}

template<typename T, typename... Args>
bool VarPrint(std::ostringstream& out, const std::string& s, const T& value, const Args&... args) {
    std::string::size_type prev_offset = 0;
    std::string::size_type curr_offset = 0;
    while ((curr_offset = s.find("%", prev_offset)) != std::string::npos) {
        out << s.substr(prev_offset, curr_offset);
        if (!(curr_offset != s.size() - 1 && s[curr_offset + 1] == '%')) {
            out << value;
            if (curr_offset + 2 < s.length())
                return VarPrint(out, s.substr(curr_offset + 2), args...);
        }

        prev_offset = curr_offset + 2;
        if (prev_offset >= s.length()) {
            break;
        }
    }
    return false;
}

int main() {
    std::ostringstream out;

    bool test1 = VarPrint(out, "integer %i\n", 1);
    bool test2 = VarPrint(out, "mix of %i and %s\n", 2, "foo");
    bool test3 = VarPrint(out, "mix of %i and %s and %i again\n", 2, "foo", 3);

    std::cout << out.str() << std::endl;

    if (test1 && test2 && test3) {
        return 0;
    } else {
        return 1;
    }
}