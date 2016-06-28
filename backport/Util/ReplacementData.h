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

            bool operator ==(File const &rhs) const {
                    return this->filepath == rhs.filepath;
            }

            bool operator <(File const &rhs) const {
                    return this->filepath < rhs.filepath;
            }

        } fileData;

        short transformationId;
        long replacementId;
        int startLine;
        int numberOfLinesReplaced;
        int numberOfLinesInReplacementText;
        bool unhandled;
        std::string replacement_text;

        struct FromData {
            File file;
            int start_line;
            int end_line;

            FromData() {}
            FromData(int file_id_, std::string filePath_, int start_line_, int end_line_) : file(filePath_, file_id_), start_line(start_line_), end_line(end_line_) {}
            
            bool operator ==(FromData const &rhs) const {
                return this->file == rhs.file && this->start_line == rhs.start_line && this->end_line == rhs.end_line;
            }

            bool operator <(FromData const &rhs) const {
                return this->file < rhs.file || (this->file == rhs.file && (this->start_line < rhs.start_line || (this->start_line ==  rhs.start_line && (this->end_line < rhs.end_line ))));
            }
        
        } from;

        ReplacementData() {}
        ReplacementData(short transformationId_, std::string const &filepath_, int fileId_, long replacementId_, int startLine_,
            int numberOfLinesReplaced_, int numberOfLinesInReplacementText_, int from_file_id, std::string from_file_path, 
            int from_start_line, int from_end_line, bool unhandled_, std::string replacement_text_)
            : fileData(filepath_, fileId_),
              transformationId(transformationId_),
              replacementId(replacementId_),
              startLine(startLine_),
              numberOfLinesReplaced(numberOfLinesReplaced_),
              numberOfLinesInReplacementText(numberOfLinesInReplacementText_),
              unhandled(unhandled_),
              replacement_text(replacement_text_),
              from(from_file_id, from_file_path, from_start_line, from_end_line)
        {}

        bool operator==(const ReplacementData& rhs) const {
            return  this->fileData == rhs.fileData && this->startLine == rhs.startLine && this->numberOfLinesReplaced == rhs.numberOfLinesReplaced &&
                this->numberOfLinesInReplacementText == rhs.numberOfLinesInReplacementText && this->replacement_text == rhs.replacement_text;
        }

        bool operator<(const ReplacementData& rhs) const {
            return this->fileData < rhs.fileData || (this->fileData == rhs.fileData && (this->startLine < rhs.startLine || (this->startLine == rhs.startLine && 
                (this->numberOfLinesReplaced < rhs.numberOfLinesReplaced || (this->numberOfLinesReplaced == rhs.numberOfLinesReplaced && (this->replacement_text < rhs.replacement_text))))));
        }

    };
}
#endif // BACKPORT_REPLACEMENTDATA_H
