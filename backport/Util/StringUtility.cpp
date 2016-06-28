#include <algorithm>
#include <functional>
#include <cctype>
#include <sstream>

#include "llvm/Support/Regex.h"
#include "StringUtility.h"
#include "Log.h"

namespace backport { namespace helper {

    // trim from start
    std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
    }

    // trim from end
    std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
    }

    // trim from both ends
    std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
    }

    // trim from both ends without modifying the argument
    std::string trim(std::string const &s) {
        std::string copy = std::string(s);
        ltrim(rtrim(copy));
        return copy;
    }

    // Replaces the given text with the given replacement at all matches
    void RegexReplaceAll(const std::string& regexString, std::string& text, const std::string& replacement, bool warning) {
        llvm::Regex regex(regexString);
        std::string orig = text;
        bool replaced = false;
        while (regex.match(text)) {
            replaced = true;
            text = regex.sub(replacement, text);
        }
        if (warning && replaced) {
            LOG(logWARNING) << "The text\n\n" << orig << "\n\n\tBecame the following after regex replacement\n\n" << text << "\n";
        }
    }

    // Replaces the provided substrings in the given string
    // TODO : The regex one is from, text, to. This is text, from, to. Consider change if time allows
    bool ReplaceAll(std::string &str, const std::string& from, const std::string& to) {
        bool replaced = false;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            // Handle case where 'to' is a substring of 'from'
            start_pos += to.length();
            replaced = true;
        }
        return replaced;
    }

    // Split up the given string at the given delimeter
    std::vector<std::string>& split(const std::string &s, char delim, std::vector<std::string> &elems) {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }

    // Split up the given string at the given delimeter
    std::vector<std::string> split(const std::string &s, char delim) {
        std::vector<std::string> elems;
        split(s, delim, elems);
        return elems;
    }

} /* namespace helper */ } /* namespace backport*/
