#ifndef BACKPORT_REPLACEMENTDATA_H
#define BACKPORT_REPLACEMENTDATA_H

#include <string>

namespace backport {

    struct ReplacementData {
        struct File {
            std::string filepath;
            int fileId;

            File() {}
            File(std::string const &filepath_, int fileId_) : filepath(filepath_), fileId(fileId_) {}
        } fileData;

        long replacementId;
        int startLine;
        int numberOfLinesReplaced;
        int numberOfLinesInReplacementText;
        short transformationId;
        bool unhandled;
        std::string replacement_text;

        struct FromData {
            int file_id;
            std::string filePath;
            int start_line;
            int end_line;

            FromData() {}
            FromData(int file_id_, std::string filePath_, int start_line_, int end_line_) : file_id(file_id_), filePath(filePath_), start_line(start_line_), end_line(end_line_) {}
        } from;

        ReplacementData() {}
        ReplacementData(short transformationId_, std::string const &filepath_, int fileId_, long replacementId_, int startLine_,
            int numberOfLinesReplaced_, int numberOfLinesInReplacementText_, int from_file_id, std::string from_file_path, int from_start_line, int from_end_line, bool unhandled_, std::string replacement_text_)
            : transformationId(transformationId_),
              fileData(filepath_, fileId_),
              replacementId(replacementId_),
              startLine(startLine_),
              numberOfLinesReplaced(numberOfLinesReplaced_),
              numberOfLinesInReplacementText(numberOfLinesInReplacementText_),
              from(from_file_id, from_file_path, from_start_line, from_end_line),
              unhandled(unhandled_),
              replacement_text(replacement_text_)
        {}

        bool operator==(const ReplacementData& rhs) const {
            return this->replacementId == rhs.replacementId;
        }

        bool operator<(const ReplacementData& rhs) const {
            return this->replacementId < rhs.replacementId;
        }

    };
}
#endif // BACKPORT_REPLACEMENTDATA_H
