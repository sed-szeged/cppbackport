#ifndef BACKPORT_FILEDATAS_H
#define BACKPORT_FILEDATAS_H

#include <vector>
#include <ctime>
#include <vector>
#include <set>
#include <map>

namespace backport {

    struct FileDatas {
        std::string path;
        time_t modified = 0;
        std::string cmdLine;

        FileDatas() {}
        FileDatas(std::string path, time_t modified) : FileDatas(path, modified, "") {}
        FileDatas(std::string path, time_t modified, std::string args) : path(path), modified(modified), cmdLine(args) {}

        bool operator<(const FileDatas& rhs) const {
            return this->path < rhs.path;
        }

        bool operator==(const FileDatas& rhs) const {
            //return ((this->modified == rhs.modified) && (this->path == rhs.path));
            return this->path == rhs.path;
        }

    };

    typedef std::map<FileDatas, std::set<FileDatas>> DependencyMap;
    typedef std::vector<FileDatas> FileDatasVec;
    typedef std::set<FileDatas> FileDatasSet;
}
#endif // BACKPORT_FILEDATAS_H
