#include "PathUtility.h"
#include "Util/StringUtility.h"

namespace backport { namespace helper {

    // If the path is file, return the containing directory
    // If the path is a directory returns itself
    Path Path::getLastDir() const {
        validityCheck();

        if (directory) {
            return Path(*this);
        } else {
            std::string lastDir = path;
            RegexReplaceAll("/[^/]+$", lastDir, "/");
            return Path(lastDir);
        }
    }

    // If the path is file, return the containing directory
    // If the path is a directory returns the directory one step up the file hierarchy
    Path Path::getOneStepUp() const {
        validityCheck();

        if (directory) {
            std::string upDir = path;
            RegexReplaceAll("/[^/]+/$", upDir, "/");
            return Path(upDir);
        } else {
            std::string upDir = path;
            RegexReplaceAll("/[^/]+$", upDir, "/");
            return Path(upDir);
        }
    }

    // Converts a relative path to an absolute one, leaves absolute paths alone
    // The results obviously depend on where the application is run from
    Path& Path::makeAbsolute() {
        validityCheck();
        if (!absolute) {
            // String can be converted into StringRef
            llvm::StringRef strRef = path;

            // StringRef can be converted into SmallString
            llvm::SmallString<1024> smallStr = strRef;

            // Turnes a path contained in SmallString absolute based on the current working directory
            llvm::sys::fs::make_absolute(smallStr);

            // Convert the SmallString back to StringRef
            strRef = smallStr;

            // Convert the StringRef back to an std::string
            path = strRef;

            // Normalize the result to the standard format
            normalizePath();

            // The path is now absolute
            absolute = true;
        }

        return *this;
    }

    // Converts a relative path to an absolute one, leaves absolute paths alone
    // The path will be considered relative to the given root directory
    // If the rootDir path contains a file, it'll use the folder containing the file as a base
    Path& Path::makeAbsoluteFrom(const Path& rootDir) {
        validityCheck();
        if (!absolute) {
            // If the path is not abolute, it will always start with "./"
            // Since the rootDir is also a path, we don't have to worry about the format
            path.replace(0, 2, rootDir.getLastDir().str());

            // The path is now absolute
            absolute = true;
        }

        return *this;
    }

    // Checks if the given parameter is a prefix of the path
    bool Path::hasPrefix(const Path& prefix) const {
        validityCheck();
        prefix.validityCheck();

        return (prefix.isDirectory() && (path.find(prefix.path) == 0));
    }

    // Returns the longest matching prefix as a directory path
    // If there's no match, returns empty path marked invalid
    Path Path::getMatchingPrefix(const Path& other) const {
        validityCheck();
        other.validityCheck();

        unsigned int match = 0;
        while ((path.size() > match) && (other.path.size() > match) && (path.at(match) == other.path.at(match))) { match++; }
        if (match == 0) {
            return Path();
        } else {
            return Path(path.substr(0, match));
        }
    }

    // Replaces the given prefix with the new one
    // The parameters need to be directories
    // If the prefix to be replaced is not actually a prefix, the function aborts execution
    Path& Path::replacePathPrefix(const Path& prefix, const Path& newPrefix) {
        // Make sure the paths are valid
        validityCheck();
        prefix.validityCheck();
        newPrefix.validityCheck();

        // Make sure the supposed prefix is actually a prefix
        if ((path.find(prefix.path) != 0) || (!prefix.isDirectory()) || (!newPrefix.isDirectory())) { abort(); }

        // The new path is the new prefix followed by the rest of the original path
        (*this) = Path(newPrefix.str() + path.substr(prefix.str().size()));

        return *this;
    }

    // The function that builds up the path class from a given string
    void Path::constructFromString(const std::string stringPath, bool forceDirectory) {
        path = stringPath;

        // NOTE : find a way to actually properly test if a path is valid
        valid = true;

        // Trim any whitespaces from the beginning and end of the string
        trim(path);

        // Determine if this path is a directory or not
        if (forceDirectory) {
            // If it's forced then it's a directory
            directory = true;
            // Convert the path to the standard format
            normalizePath();
        } else if (((path.size() > 2) && (path.at(path.size() - 1) == '/')) ||
            ((path.size() > 2) && (path.at(path.size() - 1) == '\\'))) {
            // If the path string ends with a forward or backslash, assume it's a directory
            directory = true;
            // Convert the path to the standard format
            normalizePath();
        } else {
            // In other cases we'll use the llvm function to figure it out

            // Convert the path to the standard format, set directory to false so there'll be no trailing slash
            directory = false;
            normalizePath();

            // NOTE : If the path is supposed to be a directory, but it doesn't exist, it'll be considered a file
            llvm::Twine twinePath(path);
            directory = llvm::sys::fs::is_directory(twinePath);

            // If it's a directory, add back the trailing slash
            if (directory) {
                if (path.at(path.size() - 1) != '/') {
                    path += '/';
                }
            }
        }

        absolute = false;
        // If the path starts with a '/' it's absolute
        if (path.at(0) == '/') {
            makeAbsolute();
        } else {
            // Else the path is absolute if making it absolute has no effect
            std::string backup = path;
            makeAbsolute();
            absolute = (path.compare(backup) == 0);
            path = backup;
        }

        // If the path is relative, make sure it starts with "./"
        if ((!absolute) && (path.find(std::string("./")) != 0)) {
            path = std::string("./") + path;
        }
    }

    // Removes '.' and '..' from a string containing an absolute path
    void Path::removeDotFromPath() {
        // String vector to contain the tokens
        std::vector<std::string> tokens;

        // The final slash used at the end of directory paths
        //  is a problem with the tokenizer so we'll temporarily remove it
        if (directory) {
            if (path.at(path.size() - 1) == '/') {
                path.erase(path.size() - 1);
            }
        }

        // Split the path apart at the slashes
        // At this point all backslashes have been converted into forwardslash
        tokens = split(path, '/');

        // The tokens left after removing the excess dots
        std::vector<std::string> finalTokens;

        for (auto& token : tokens) {
            // If we find a ".." token, we step back in the folder hierarchy, meaning we remove the previous token if we can
            if (token.compare("..") == 0) {
                if ((!finalTokens.empty()) && (finalTokens.at(finalTokens.size() - 1).compare("..") != 0)) {
                    finalTokens.pop_back();
                } else {
                    // Relative paths start with a "." in the representation
                    if (finalTokens.empty()) {
                        finalTokens.push_back(".");
                    }
                    finalTokens.push_back("..");
                }
            }
            // We can simply skip over "." tokens
            else if (token.compare(".") != 0) {
                finalTokens.push_back(token);
            }
        }
        // If we end up with nothing, use a single "." to indicate a relative path
        if (finalTokens.empty()) {
            finalTokens.push_back(".");
        }

        // Put the final tokens together to form the new path string
        path = "";
        for (auto& token : finalTokens) {
            path += token + std::string("/");
        }

        // The way we did it will always leave a forwardslash at the end of the path
        // If this is supposed to be a file we'll need to delete that
        if (!directory) {
            path.erase(path.size() - 1);
        }
    }

    // Normalize the path string to a standard format
    void Path::normalizePath() {
        // Due to escaping madness the regex below matches single backslashes
        RegexReplaceAll(std::string("\\\\"), path, std::string("/"));

        // Remove excess "." and ".." from the path
        removeDotFromPath();

        // Directory paths end with a slash, file paths don't
        if (directory) {
            if (path.at(path.size() - 1) != '/') {
                path += '/';
            }
        } else {
            if (path.at(path.size() - 1) == '/') {
                path.erase(path.size() - 1);
            }
        }

        // If some cases the result can end up having double forwardslashes, we'll reduce those to single ones
        while (path.find(std::string("//")) != std::string::npos) {
            RegexReplaceAll(std::string("//"), path, std::string("/"));
        }
    }

    // Helper function that converts a container of strings to a container of paths
    // TODO : Use templates to extend function to any std compatible container
    std::vector<Path> containerStringToPath(const std::vector<std::string>& container) {
        std::vector<Path> newContainer;
        newContainer.reserve(container.size());
        for (const auto& stringPath : container) {
            newContainer.push_back(Path(stringPath));
        }
        return newContainer;
    }

    // Helper function that converts a container of paths to a container of strings
    // TODO : Use templates to extend function to any std compatible container
    std::vector<std::string> containerPathToString(const std::vector<Path>& container) {
        std::vector<std::string> newContainer;
        newContainer.reserve(container.size());
        for (const auto& path : container) {
            newContainer.push_back(path.str());
        }
        return newContainer;
    }

} /*namespace helper*/ } /*namespace backport*/
