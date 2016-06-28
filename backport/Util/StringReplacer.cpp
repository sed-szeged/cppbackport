#include "StringReplacer.h"
#include "Log.h"

#include <algorithm>

namespace backport { namespace helper {

    // Add a replacement
    void StringReplacer::addReplacement(const StringReplacement& replacement) {
        if ((replacement.getStartOffset() < 0) || (replacement.getEndOffset() < replacement.getStartOffset()) || (replacement.getEndOffset() > text.size())) {
            LOG(logERROR) << "Invalid string replacement";
            exit(1);
        }
        replacements.push_back(replacement);
    }

    // Add a replacement
    void StringReplacer::addReplacement(std::string replacement, unsigned int startOffset, unsigned int endOffset) {
        addReplacement(StringReplacement(replacement, startOffset, endOffset));
    }

    // Apply replacement returns the result of applying the currently collected replacements
    std::string StringReplacer::applyReplacements() {
        std::sort(replacements.begin(), replacements.end());

        int textSizeAlteration = 0;
        for (const auto& replacement : replacements) {
            textSizeAlteration = textSizeAlteration + replacement.getReplacementText().size() - (replacement.getEndOffset() - replacement.getStartOffset());
        }

        unsigned int lastOffset = 0;
        unsigned int alterationOffset = 0;
        std::string replacementText = "";
        replacementText.reserve(text.size() + textSizeAlteration + 1);
        for (const auto& replacement : replacements) {
            unsigned int startOffset = replacement.getStartOffset();
            unsigned int endOffset = replacement.getEndOffset();

            if (startOffset < lastOffset) {
                LOG(logERROR) << "Overlapping string replacements";
                exit(1);
            }
            
            if (startOffset > lastOffset) {
                replacementText.append(text.substr(lastOffset, startOffset - lastOffset));
            }

            replacementText.append(replacement.getReplacementText());

            lastOffset = replacement.getEndOffset();
        }

        if (lastOffset < text.size()) {
            replacementText.append(text.substr(lastOffset));
        }

        return replacementText;
    }

} /*namespace helper*/ } /*namespace backport*/
