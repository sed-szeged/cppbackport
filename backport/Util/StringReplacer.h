#ifndef STRING_REPLACER_H
#define STRING_REPLACER_H

#include <string>
#include <vector>

namespace backport { namespace helper {

    // A replacement for a string
    class StringReplacement {
    public:
        // To create a replacement, we need the replacement text, the starting and the ending offset
        StringReplacement(std::string replacement, unsigned int startOffset, unsigned int endOffset) : replacement(replacement), startOffset(startOffset), endOffset(endOffset) {}

        // Returns the replacement text
        const std::string& getReplacementText() const { return replacement; }

        // Returns the start offset
        unsigned int getStartOffset() const { return startOffset; }

        // Returns the ending offset
        unsigned int getEndOffset() const { return endOffset; }

        // Comparison operators
        bool operator ==(const StringReplacement &other) const { return ((startOffset == other.startOffset) && (endOffset == other.endOffset) && (replacement.compare(other.replacement) == 0)); }
        bool operator  <(const StringReplacement &other) const { return
                                                                    ( 
                                                                        ((startOffset <  other.startOffset)) ||
                                                                        ((startOffset == other.startOffset) && (endOffset <  other.endOffset)) ||
                                                                        ((startOffset == other.startOffset) && (endOffset == other.endOffset) && (replacement.compare(other.replacement) <  0))
                                                                    );
                                                               }
        bool operator !=(const StringReplacement &other) const { return !(*this == other); }
        bool operator <=(const StringReplacement &other) const { return ((*this == other) || (*this < other)); }
        bool operator  >(const StringReplacement &other) const { return !(*this <= other); }
        bool operator >=(const StringReplacement &other) const { return !(*this  > other); }

    private:
        // Replacement text
        std::string replacement;

        // Starting offset
        unsigned int startOffset;

        // Ending offset
        unsigned int endOffset;
    };

    // String Replacer that applies the replacements
    class StringReplacer {
    public:
        // To initialize a string replacer, we need the string in witch we want to apply the replacements
        StringReplacer(std::string text) : text(text) {}

        // Add a replacement
        void addReplacement(const StringReplacement& replacement);
        void addReplacement(std::string replacement, unsigned int startOffset, unsigned int endOffset);

        // Apply replacement returns the result of applying the currently collected replacements
        std::string applyReplacements();

    private:
        // The original text on whitch we'd like to apply the replacements
        std::string text;

        // The replacements to be applied on the text
        std::vector<StringReplacement> replacements;
    };

} /*namespace helper*/ } /*namespace backport*/

#endif // PATH_UTILITY_H
