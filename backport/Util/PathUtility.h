#ifndef PATH_UTILITY_H
#define PATH_UTILITY_H

#include <string>
#include "llvm/Support/FileSystem.h"

namespace backport { namespace helper {

    class Path {
    public:
        // Default consatructor creates a local folder relative path
        Path() : path(""), directory(false), absolute(false), valid(false) {}

        // Copy constructor
        Path(const Path& other) : path(other.path), directory(other.directory), absolute(other.absolute), valid(other.valid) {}

        // NOTE : Constructors don't check if the given parameter is a valid path

        // Constructor from a string
        Path(const std::string& stringPath, bool forceDirectory = false) { constructFromString(stringPath, forceDirectory); }

        // Assignment from a string
        Path& operator= (const std::string& stringPath) { constructFromString(stringPath); return *this; }

        // Implicit conversion to string
        operator std::string() const { validityCheck(); return path; }

        // Explicit conversion to string
        std::string str() const { validityCheck(); return path; }

        // Constructor from an llvm::Twine
        Path(const llvm::Twine& twinePath, bool forceDirectory = false) { constructFromString(twinePath.str(), forceDirectory); }

        // Assignment from a Twine
        Path& operator= (const llvm::Twine twinePath) { constructFromString(twinePath.str()); return *this; }

        // Explicit conversion to Twine
        llvm::Twine twine() const { validityCheck(); return llvm::Twine(path); }

        // Constructor from an llvm::StringRef
        Path(const llvm::StringRef& stringRefPath, bool forceDirectory = false) { constructFromString(stringRefPath.str(), forceDirectory); }

        // Assignment from a StringRef
        Path& operator= (const llvm::StringRef stringRefPath) { constructFromString(stringRefPath.str()); return *this; }

        // Explicit conversion to StringRef
        llvm::StringRef strRef() const { validityCheck(); return llvm::StringRef(path); }

        // Constructor from const char*
        Path(const char* charPath, bool forceDirectory = false) { constructFromString(std::string(charPath), forceDirectory); }

        // Assignment from const char*
        Path& operator= (const char* charPath) { constructFromString(std::string(charPath)); return *this; }

        // Explicit conversion to const char*
        const char* c_str() const { validityCheck(); return path.c_str(); }
        
        // If the path is file, return the containing directory
        // If the path is a directory returns itself
        Path getLastDir() const;

        // If the path is file, return the containing directory
        // If the path is a directory returns the directory one step up the file hierarchy
        Path getOneStepUp() const;

        // Converts a relative path to an absolute one, leaves absolute paths alone
        // The results obviously depend on where the application is run from
        Path& makeAbsolute();

        // Converts a relative path to an absolute one, leaves absolute paths alone
        // The path will be considered relative to the given root directory
        // If the rootDir path contains a file, it'll use the folder containing the file as a base
        Path& makeAbsoluteFrom(const Path& rootDir);

        // Is this path a directory
        bool isDirectory() const { return directory; }

        // Is this path absolute
        bool isAbsolute() const { return absolute; }

        // Is this path valid
        bool isValid() const { return valid; }

        // Returns the longest matching prefix as a directory path
        // If there's no match, returns empty path marked invalid
        Path getMatchingPrefix(const Path& other) const;

        // Checks if the given parameter is a prefix of the path
        bool hasPrefix(const Path& prefix) const;

        // Replaces the given prefix with the new one
        // The parameters need to be directories
        // If the prefix to be replaced is not actually a prefix, the function aborts execution
        Path& replacePathPrefix(const Path& prefix, const Path& newPrefix);

        // Comparison operators
        bool operator ==(const Path &other) const { return (( valid == other.valid ) && ( directory == other.directory ) && ( absolute == other.absolute ) && ( path.compare(other.path) == 0 )); }
        bool operator  <(const Path &other) const { return
                                                        (
                                                           ((!valid && other.valid)) ||
                                                           (( valid == other.valid ) && (!directory && other.directory)) ||
                                                           (( valid == other.valid ) && ( directory == other.directory ) && (!absolute && other.absolute)) ||
                                                           (( valid == other.valid ) && ( directory == other.directory ) && ( absolute == other.absolute ) && ( path.compare(other.path) < 0 ))
                                                        );
                                                  }
        bool operator !=(const Path &other) const { return !(*this == other); }
        bool operator <=(const Path &other) const { return ((*this == other) || (*this < other)); }
        bool operator  >(const Path &other) const { return !(*this <= other); }
        bool operator >=(const Path &other) const { return !(*this  > other); }

    private:
        // The function that builds up the path class from a given string
        // You can force the path to be considered a directory, usefull if it describes a directory that doesn't exist
        void constructFromString(const std::string stringPath, bool forceDirectory = false);

        // Removes '.' and '..' from the path string
        void removeDotFromPath();

        // Normalize the path string to a standard format
        void normalizePath();

        // The path string
        std::string path;

        // Is this path a directory
        bool directory;

        // Is this path absolute
        bool absolute;

        // Is this path valid
        bool valid;

        // Path manipulation functions need valid paths to work with
        void validityCheck() const { if (!valid) { abort(); } }
    };

    // Helper function that converts a container of strings to a container of paths
    // TODO : Use templates to extend function to any std compatible container
    std::vector<Path> containerStringToPath(const std::vector<std::string>& container);

    // Helper function that converts a container of paths to a container of strings
    // TODO : Use templates to extend function to any std compatible container
    std::vector<std::string> containerPathToString(const std::vector<Path>& container);

} /*namespace helper*/ } /*namespace backport*/

#endif // PATH_UTILITY_H
