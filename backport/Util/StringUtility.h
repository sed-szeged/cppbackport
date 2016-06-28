#ifndef STRING_UTILITY_H
#define STRING_UTILITY_H

#include <string>
#include <vector>
#include <utility> // for std::pair

namespace backport { namespace helper {

    class SplittableString : public std::string, public std::pair<std::string, std::string> {
    public:
        SplittableString(std::string text) : std::string(text), std::pair<std::string, std::string>(make_pair(text, "")) {}
        SplittableString(std::string f, std::string s) : std::string(f + s), std::pair<std::string, std::string>(make_pair(f, s)) {}
    };

    // trim from start
    std::string &ltrim(std::string &s);

    // trim from end
    std::string &rtrim(std::string &s);

    // trim from both ends
    std::string &trim(std::string &s);

    // trim from both ends without modifying the argument
    std::string trim(std::string const &s);

    // Replaces the given text with the given replacement at all matches
    void RegexReplaceAll(const std::string& regexString, std::string& text, const std::string& replacement, bool warning = false);

    // Replaces the provided substrings in the given string
    bool ReplaceAll(std::string &str, const std::string& from, const std::string& to);

    // Split up the given string at the given delimeter
    std::vector<std::string>& split(const std::string &s, char delim, std::vector<std::string> &elems);

    // Split up the given string at the given delimeter
    std::vector<std::string> split(const std::string &s, char delim);

} /*namespace helper*/ }/*namespace backport*/

#endif // STRING_UTILITY_H
