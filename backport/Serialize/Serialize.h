
#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <utility>
#include <ostream>
#include <istream>
#include <algorithm>
#include <type_traits>
#include "llvm/ADT/StringMap.h"
#include "TransformBase/Transforms.h"
#include "BackportManager/SimpleCompilationDatabase.h"
#include "Util/FileDatas.h"
#include "Util/ReplacementData.h"
#include <list>

namespace backport { namespace helper {

template <typename T>
struct typeinfo
{
    static const char* name();
};

#define decl_typeinfo(type_name, id) \
template <> \
struct typeinfo<type_name> \
{ \
    static const char* name() \
    { \
        return id; \
    } \
}

decl_typeinfo(bool, "\x18");

decl_typeinfo(char, "\x19");
decl_typeinfo(signed char, "\x1a");
decl_typeinfo(unsigned char, "\x1b");

decl_typeinfo(signed short, "\x1c");
decl_typeinfo(unsigned short, "\x1d");

decl_typeinfo(signed int, "\x1e");
decl_typeinfo(unsigned int, "\x1f");

decl_typeinfo(signed long, "\x20");
decl_typeinfo(unsigned long, "\x21");

#ifdef _MSC_VER
decl_typeinfo(signed __int64, "\x22");
decl_typeinfo(unsigned __int64, "\x23");
#endif

decl_typeinfo(std::string, "\x24");
decl_typeinfo(CompileCommand, "\x25");

decl_typeinfo(backport::ReplacementData::File, "\x26");

decl_typeinfo(backport::ReplacementData::FromData, "\x27");

decl_typeinfo(backport::ReplacementData, "\x28");

decl_typeinfo(Replacement, "\x29");

decl_typeinfo(CompilationDatabase, "\x2a");

decl_typeinfo(backport::FileDatas, "\x2b");

decl_typeinfo(TranslationUnitReplacements, "\x2c");

template <class T>
struct typeinfo<std::vector<T> >
{
    static const char* name()
    {
        return "\x2d";
    }
};

template <class T>
struct typeinfo<std::set<T> >
{
    static const char* name()
    {
        return "\x2e";
    }
};

template <class T>
struct typeinfo<std::list<T> >
{
    static const char* name()
    {
        return "\x2f";
    }
};

template <class T, class K>
struct typeinfo<std::pair<T, K> >
{
    static const char* name()
    {
        return "\x30";
    }
};

template <class T>
struct typeinfo<llvm::StringMap<T> >
{
    static const char* name()
    {
        return "\x31";
    }
};

template <class T, class K>
struct typeinfo<std::map<T, K> >
{
    static const char* name()
    {
        return "\x32";
    }
};

}

namespace serialization {

template<class T>
std::string header(std::size_t dataSize) {
    std::string res;

    res += '\x01'; // ASCII Start of header

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += typeinfo<T>::name();
    res += '\x03';
#endif

    for (std::size_t i = 0; i < sizeof(dataSize); ++i) {
        res += (reinterpret_cast<char const *>(&dataSize))[i];
    }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x03';

    res += '\x17'; // ASCII End of transmission block (here indicating the end of the header).
#endif

    return res;
}

struct headInfo {
    std::string name;
    std::size_t numberOfItems;
};

template <class T>
struct withSizeInfo {
    T data;
    std::size_t size;
};

withSizeInfo<headInfo> getHeadInfo(std::string const &data) {
    if (data[0] != '\x01' /* ASCII Start of header*/)
        abort();

    withSizeInfo<headInfo> res;
    std::size_t i = 1;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    for (; i < data.size() && data[i] != '\x03'; ++i) {
        res.data.name += data[i];
    }

    if (i >= data.size() || data[i] != '\x03')
        abort();

    ++i;
#endif

    res.data.numberOfItems = 0;
    for (std::size_t j = 0; j < sizeof(res.data.numberOfItems) && i < data.size(); ++i, ++j) {
        reinterpret_cast<char *>(&res.data.numberOfItems)[j] = data[i];
    }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    if (i >= data.size() || data[i] != '\x03')
        abort();

    if (i + 1 >= data.size() || data[i + 1] != '\x17' /* ASCII End of transmission block (here indicating the end of the header).*/)
        abort();

    res.size = i + 2;
#else
    res.size = i;
#endif

    return res;
}

/*
template <class T>
typename std::enable_if<std::is_fundamental<T>::value == false, std::string>::type serialize(T const &c);
*/

template<class T, class enable = void>
struct unserialize {
    static withSizeInfo<T> do_unserialization(std::string const &data);
};

template<typename T>
std::string serialize_primitive(T const &c) {
    std::string result = header<T>(sizeof(T));

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    result += "\x02";
#endif

    for (std::size_t i = 0; i < sizeof(T); ++i)
        result += (reinterpret_cast<char const *>(&c))[i];

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    result += "\x03";
#endif

    return result;
}

std::string serialize(std::string const &c);
std::string serialize(backport::helper::Path const &v);
std::string serialize(llvm::StringRef const &v);
std::string serialize(backport::ReplacementData::File const &f);
std::string serialize(backport::ReplacementData::FromData const &f);
std::string serialize(backport::ReplacementData const &repData);
std::string serialize(backport::FileDatas const &fds);
std::string serialize(Replacement const &TUR);
template<class T, class K>
std::string serialize(std::pair<T, K> const &p);
template <class T>
std::string serialize(std::vector<T> const &v);
template <class T>
std::string serialize(std::list<T> const &v);
template <class T>
std::string serialize(std::set<T> const &v);
std::string serialize(TranslationUnitReplacements const &TUR);
template <class T>
std::string serialize(llvm::StringMap<T> const &v);
template <class K, class L>
std::string serialize(std::map<K, L> const &v);
std::string serialize(CompileCommand const &cc);
std::string serialize(CompilationDatabase const &CDatabase);

std::string serialize(const int &p) { return serialize_primitive(p); }
std::string serialize(const unsigned int &p) { return serialize_primitive(p); }
std::string serialize(const bool &p) { return serialize_primitive(p); }
std::string serialize(const time_t &p) { return serialize_primitive(p); }
std::string serialize(const char &p) { return serialize_primitive(p); }
std::string serialize(const signed char &p) { return serialize_primitive(p); }
std::string serialize(const unsigned char &p) { return serialize_primitive(p); }

template<>
struct unserialize<int> {
    static withSizeInfo<int> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != backport::helper::typeinfo<int>::name())
            abort();

        if (hi.data.numberOfItems != sizeof(int))
            abort();

        if (data[hi.size] != '\x02' /*Start of data*/)
            abort();
#endif

        withSizeInfo<int> res;
        std::string temp;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        std::size_t i = hi.size + 1;
#else
        std::size_t i = hi.size;
#endif
        res.data = 0;
        for (std::size_t j = 0; i < data.size() && j < hi.data.numberOfItems; ++i, ++j) {
            (reinterpret_cast<char *>(&(res.data)))[j] = data[i];
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (i >= data.size() || data[i] != '\x03')
            abort();
#endif

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        res.size = i + 1;
#else
        res.size = i;
#endif

        return res;
    }
};

template<>
struct unserialize<unsigned int> {
    static withSizeInfo<unsigned int> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != backport::helper::typeinfo<unsigned int>::name())
            abort();

        if (hi.data.numberOfItems != sizeof(unsigned int))
            abort();

        if (data[hi.size] != '\x02' /*Start of data*/)
            abort();
#endif

        withSizeInfo<unsigned int> res;
        std::string temp;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        std::size_t i = hi.size + 1;
#else
        std::size_t i = hi.size;
#endif
        res.data = 0;
        for (std::size_t j = 0; i < data.size() && j < hi.data.numberOfItems; ++i, ++j) {
            (reinterpret_cast<char *>(&(res.data)))[j] = data[i];
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (i >= data.size() || data[i] != '\x03')
            abort();
#endif

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        res.size = i + 1;
#else
        res.size = i;
#endif

        return res;
    }
};

template<>
struct unserialize<bool> {
    static withSizeInfo<bool> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != backport::helper::typeinfo<bool>::name())
            abort();

        if (hi.data.numberOfItems != sizeof(bool))
            abort();

        if (data[hi.size] != '\x02' /*Start of data*/)
            abort();
#endif

        withSizeInfo<bool> res;
        std::string temp;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        std::size_t i = hi.size + 1;
#else
        std::size_t i = hi.size;
#endif
        res.data = 0;
        for (std::size_t j = 0; i < data.size() && j < hi.data.numberOfItems; ++i, ++j) {
            (reinterpret_cast<char *>(&(res.data)))[j] = data[i];
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (i >= data.size() || data[i] != '\x03')
            abort();
#endif

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        res.size = i + 1;
#else
        res.size = i;
#endif

        return res;
    }
};

template<>
struct unserialize<time_t> {
    static withSizeInfo<time_t> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != backport::helper::typeinfo<time_t>::name())
            abort();

        if (hi.data.numberOfItems != sizeof(time_t))
            abort();

        if (data[hi.size] != '\x02' /*Start of data*/)
            abort();
#endif

        withSizeInfo<time_t> res;
        std::string temp;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        std::size_t i = hi.size + 1;
#else
        std::size_t i = hi.size;
#endif
        res.data = 0;
        for (std::size_t j = 0; i < data.size() && j < hi.data.numberOfItems; ++i, ++j) {
            (reinterpret_cast<char *>(&(res.data)))[j] = data[i];
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (i >= data.size() || data[i] != '\x03')
            abort();
#endif

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        res.size = i + 1;
#else
        res.size = i;
#endif

        return res;
    }
};

template<>
struct unserialize<char> {
    static withSizeInfo<char> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != backport::helper::typeinfo<char>::name())
            abort();

        if (hi.data.numberOfItems != sizeof(char))
            abort();

        if (data[hi.size] != '\x02' /*Start of data*/)
            abort();
#endif

        withSizeInfo<char> res;
        std::string temp;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        std::size_t i = hi.size + 1;
#else
        std::size_t i = hi.size;
#endif
        res.data = 0;
        for (std::size_t j = 0; i < data.size() && j < hi.data.numberOfItems; ++i, ++j) {
            (reinterpret_cast<char *>(&(res.data)))[j] = data[i];
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (i >= data.size() || data[i] != '\x03')
            abort();
#endif

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        res.size = i + 1;
#else
        res.size = i;
#endif

        return res;
    }
};

template<>
struct unserialize<signed char> {
    static withSizeInfo<signed char> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != backport::helper::typeinfo<signed char>::name())
            abort();

        if (hi.data.numberOfItems != sizeof(signed char))
            abort();

        if (data[hi.size] != '\x02' /*Start of data*/)
            abort();
#endif

        withSizeInfo<signed char> res;
        std::string temp;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        std::size_t i = hi.size + 1;
#else
        std::size_t i = hi.size;
#endif
        res.data = 0;
        for (std::size_t j = 0; i < data.size() && j < hi.data.numberOfItems; ++i, ++j) {
            (reinterpret_cast<char *>(&(res.data)))[j] = data[i];
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (i >= data.size() || data[i] != '\x03')
            abort();
#endif

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        res.size = i + 1;
#else
        res.size = i;
#endif

        return res;
    }
};

template<>
struct unserialize<unsigned char> {
    static withSizeInfo<unsigned char> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != backport::helper::typeinfo<unsigned char>::name())
            abort();

        if (hi.data.numberOfItems != sizeof(unsigned char))
            abort();

        if (data[hi.size] != '\x02' /*Start of data*/)
            abort();
#endif

        withSizeInfo<unsigned char> res;
        std::string temp;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        std::size_t i = hi.size + 1;
#else
        std::size_t i = hi.size;
#endif
        res.data = 0;
        for (std::size_t j = 0; i < data.size() && j < hi.data.numberOfItems; ++i, ++j) {
            (reinterpret_cast<char *>(&(res.data)))[j] = data[i];
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (i >= data.size() || data[i] != '\x03')
            abort();
#endif

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        res.size = i + 1;
#else
        res.size = i;
#endif

        return res;
    }
};

std::string serialize(std::string const &c) {
    std::string result = header<std::string>(c.size());

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    result += "\x02";
#endif

    result += c;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    result += "\x03";
#endif

    return result;
}

std::string serialize(backport::helper::Path const &v) {
    return serialize(v.str());
}

std::string serialize(llvm::StringRef const &v) {
    return serialize(v.str());
}

template<>
struct unserialize<std::string> {
    static withSizeInfo<std::string> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<std::string>::name())
            abort();

        if (data[hi.size] != '\x02' /*Start of text*/)
            abort();
#endif

        withSizeInfo<std::string> res;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        std::size_t i = hi.size + 1;
#else
        std::size_t i = hi.size;
#endif
        for (std::size_t j = 0; i < data.size() && j < hi.data.numberOfItems; ++i, ++j) {
            res.data += data[i];
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (i >= data.size() || data[i] != '\x03')
            abort();

        res.size = i + 1;
#else
        res.size = i;
#endif

        return res;
    }
};

std::string serialize(backport::ReplacementData::File const &f) {
    std::string res;

    res += header<backport::ReplacementData::File>(2);
    //res += serialize(f.fileId);
    res += serialize(f.filepath);


#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<>
struct unserialize<backport::ReplacementData::File> {
    static withSizeInfo<backport::ReplacementData::File> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<backport::ReplacementData::File>::name())
            abort();

        if (hi.data.numberOfItems != 2)
            abort();
#endif

        withSizeInfo<backport::ReplacementData::File > res;
        std::size_t totalSize = hi.size;

        /*{
            auto tmp = unserialize<int >::do_unserialization(data.substr(totalSize));
            res.data.fileId = std::move(tmp.data);
            totalSize += tmp.size;
        }*/

        res.data.fileId = -1;

        {
            auto tmp = unserialize<std::string>::do_unserialization(data.substr(totalSize));
            res.data.filepath = std::move(tmp.data);
            totalSize += tmp.size;
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (totalSize >= data.size() || data[totalSize] != '\x17')
            abort();

        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;
    }
};

std::string serialize(backport::ReplacementData::FromData const &f) {
    std::string res;

    res += header<backport::ReplacementData::FromData>(3);
    res += serialize(f.end_line);
    res += serialize(f.file);
    //res += serialize(f.start_line);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<>
struct unserialize<backport::ReplacementData::FromData> {
    static withSizeInfo<backport::ReplacementData::FromData> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<backport::ReplacementData::FromData>::name())
            abort();

        if (hi.data.numberOfItems != 3)
            abort();
#endif

        withSizeInfo<backport::ReplacementData::FromData > res;
        std::size_t totalSize = hi.size;

        {
            auto tmp = unserialize<int >::do_unserialization(data.substr(totalSize));
            res.data.end_line = std::move(tmp.data);
            totalSize += tmp.size;
        }

        {
            auto tmp = unserialize<backport::ReplacementData::File >::do_unserialization(data.substr(totalSize));
            res.data.file = std::move(tmp.data);
            totalSize += tmp.size;
        }

        /*{
            auto tmp = unserialize<int>::do_unserialization(data.substr(totalSize));
            res.data.start_line = std::move(tmp.data);
            totalSize += tmp.size;
        }*/

        res.data.start_line = res.data.end_line;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (totalSize >= data.size() || data[totalSize] != '\x17')
            abort();

        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;
    }
};

std::string serialize(backport::ReplacementData const &repData) {
    std::string res;

    res += header<backport::ReplacementData>(9);
    res += serialize(repData.fileData);
    res += serialize(repData.from);
    res += serialize(repData.numberOfLinesInReplacementText);
    res += serialize(repData.numberOfLinesReplaced);
    //res += serialize(repData.replacementId);
    res += serialize(repData.replacement_text);
    res += serialize(repData.startLine);
    //res += serialize(repData.transformationId);
    //res += serialize(repData.unhandled);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<>
struct unserialize<backport::ReplacementData> {
    static withSizeInfo<backport::ReplacementData> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<backport::ReplacementData>::name())
            abort();

        if (hi.data.numberOfItems != 9)
            abort();
#endif

        withSizeInfo<backport::ReplacementData > res;
        std::size_t totalSize = hi.size;

        {
            auto tmp = unserialize<backport::ReplacementData::File >::do_unserialization(data.substr(totalSize));
            res.data.fileData = std::move(tmp.data);
            totalSize += tmp.size;
        }

        {
            auto tmp = unserialize<backport::ReplacementData::FromData >::do_unserialization(data.substr(totalSize));
            res.data.from = std::move(tmp.data);
            totalSize += tmp.size;
        }

        {
            auto tmp = unserialize<int >::do_unserialization(data.substr(totalSize));
            res.data.numberOfLinesInReplacementText = std::move(tmp.data);
            totalSize += tmp.size;
        }

        {
            auto tmp = unserialize<int >::do_unserialization(data.substr(totalSize));
            res.data.numberOfLinesReplaced = std::move(tmp.data);
            totalSize += tmp.size;
        }

        /*{
            auto tmp = unserialize<long >::do_unserialization(data.substr(totalSize));
            res.data.replacementId = std::move(tmp.data);
            totalSize += tmp.size;
        }*/

        res.data.replacementId = -1;

        {
            auto tmp = unserialize<std::string >::do_unserialization(data.substr(totalSize));
            res.data.replacement_text = std::move(tmp.data);
            totalSize += tmp.size;
        }

        {
            auto tmp = unserialize<int >::do_unserialization(data.substr(totalSize));
            res.data.startLine = std::move(tmp.data);
            totalSize += tmp.size;
        }

        /*{
            auto tmp = unserialize<short >::do_unserialization(data.substr(totalSize));
            res.data.transformationId = std::move(tmp.data);
            totalSize += tmp.size;
        }*/

        res.data.transformationId = -1;

        /*{
            auto tmp = unserialize<bool >::do_unserialization(data.substr(totalSize));
            res.data.unhandled = std::move(tmp.data);
            totalSize += tmp.size;
        }*/

        res.data.unhandled = true;

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (totalSize >= data.size() || data[totalSize] != '\x17')
            abort();

        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;
    }
};

std::string serialize(backport::FileDatas const &fds) {
    std::string res;

    res += header<backport::FileDatas>(3);

    res += serialize(fds.path);
    res += serialize(fds.modified);
    res += serialize(fds.cmdLine);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17'; // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<>
struct unserialize<backport::FileDatas> {
    static withSizeInfo<backport::FileDatas> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<backport::FileDatas>::name())
            abort();

        if (hi.data.numberOfItems != 3)
            abort();
#endif

        std::size_t totalSize = hi.size;

        std::string path;
        {
            auto tmp = unserialize<std::string >::do_unserialization(data.substr(totalSize));
            path = std::move(tmp.data);
            totalSize += tmp.size;
        }

        std::time_t modified;
        {
            auto tmp = unserialize<std::time_t >::do_unserialization(data.substr(totalSize));
            modified = std::move(tmp.data);
            totalSize += tmp.size;
        }

        std::string cmdline;
        {
            auto tmp = unserialize<std::string >::do_unserialization(data.substr(totalSize));
            cmdline = std::move(tmp.data);
            totalSize += tmp.size;
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (totalSize >= data.size() || data[totalSize] != '\x17')
            abort();

#endif

        withSizeInfo<backport::FileDatas> res{ { path, modified, cmdline }, totalSize };

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)

        ++res.size;

#endif

        return res;
    }
};

std::string serialize(Replacement const &TUR) {
    std::string res;

    res += header<Replacement>(4);
    res += serialize(TUR.getFilePath());
    res += serialize(TUR.getReplacementText());
    res += serialize(TUR.getOffset());
    res += serialize(TUR.getLength());

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<>
struct unserialize<Replacement> {
    static withSizeInfo<Replacement> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<Replacement>::name())
            abort();

        if (hi.data.numberOfItems != 4)
            abort();
#endif

        std::size_t totalSize = hi.size;

        std::string file_path;
        {
            auto tmp = unserialize<std::string >::do_unserialization(data.substr(totalSize));
            file_path = std::move(tmp.data);
            totalSize += tmp.size;
        }

        std::string replacement_text;
        {
            auto tmp = unserialize<std::string >::do_unserialization(data.substr(totalSize));
            replacement_text = std::move(tmp.data);
            totalSize += tmp.size;
        }

        unsigned int offset = 0;
        {
            auto tmp = unserialize<unsigned int >::do_unserialization(data.substr(totalSize));
            offset = std::move(tmp.data);
            totalSize += tmp.size;
        }

        unsigned int length = 0;
        {
            auto tmp = unserialize<unsigned int >::do_unserialization(data.substr(totalSize));
            length = std::move(tmp.data);
            totalSize += tmp.size;
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (totalSize >= data.size() || data[totalSize] != '\x17')
            abort();
#endif


        withSizeInfo<Replacement> res{ { file_path, offset, length, replacement_text }, totalSize};


#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        ++res.size;
#endif

        return res;
    }
};

template<class T, class K>
std::string serialize(std::pair<T, K> const &p) {
    std::string res;

    res += header<std::pair<T, K> >(2);

    res += serialize(p.first);
    res += serialize(p.second);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<class K, class L>
struct unserialize<std::pair<K, L> > {
    static withSizeInfo<std::pair<K, L> > do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<std::pair<K, L> >::name())
            abort();

        if (hi.data.numberOfItems != 2)
            abort();
#endif

        withSizeInfo<std::pair<K, L> > res;
        std::size_t totalSize = hi.size;

        {
            auto tmp = unserialize<K>::do_unserialization(data.substr(totalSize));

            res.data.first = std::move(tmp.data);
            totalSize += tmp.size;
        }

        {
            auto tmp = unserialize<L>::do_unserialization(data.substr(totalSize));

            res.data.second = std::move(tmp.data);
            totalSize += tmp.size;
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (totalSize >= data.size() || data[totalSize] != '\x17')
            abort();

        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;

    }
};

template <class T>
std::string serialize(std::vector<T> const &v) {
    std::string res;

    res += header<std::vector<T> >(v.size());

    for (auto &c : v) {
        res += serialize(c);
    }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<class K>
struct unserialize<std::vector<K> > {
    static withSizeInfo<std::vector<K> > do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<std::vector<K> >::name())
            abort();
#endif

        withSizeInfo<std::vector<K> > res;
        std::size_t totalSize = hi.size;

        std::string cdata = data.substr(hi.size);
        for (std::size_t i = 0; i < hi.data.numberOfItems; ++i) {
            auto tmp = unserialize<K>::do_unserialization(cdata);

            res.data.push_back(tmp.data);
             
            totalSize += tmp.size;
            cdata = data.substr(totalSize);

        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (cdata[0] != '\x17')
            abort();
#endif

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;

    }
};

template <class T>
std::string serialize(std::list<T> const &v) {
    std::string res;

    res += header<std::list<T> >(v.size());

    for (auto &c : v) {
        res += serialize(c);
    }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<class K>
struct unserialize<std::list<K> > {
    static withSizeInfo<std::list<K> > do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<std::list<K> >::name())
            abort();
#endif

        withSizeInfo<std::list<K> > res;
        std::size_t totalSize = hi.size;

        std::string cdata = data.substr(hi.size);
        for (std::size_t i = 0; i < hi.data.numberOfItems; ++i) {
            auto tmp = unserialize<K>::do_unserialization(cdata);

            res.data.push_back(tmp.data);

            totalSize += tmp.size;
            cdata = data.substr(totalSize);

        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (cdata[0] != '\x17')
            abort();

        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;

    }
};

template <class T>
std::string serialize(std::set<T> const &v) {
    std::string res;

    res += header<std::set<T> >(v.size());

    for (auto &c : v) {
        res += serialize(c);
    }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<class K>
struct unserialize<std::set<K> > {
    static withSizeInfo<std::set<K> > do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<std::set<K> >::name())
            abort();
#endif

        withSizeInfo<std::set<K> > res;
        std::size_t totalSize = hi.size;

        std::string cdata = data.substr(hi.size);
        for (std::size_t i = 0; i < hi.data.numberOfItems; ++i) {
            auto tmp = unserialize<K>::do_unserialization(cdata);

            res.data.insert(tmp.data);

            totalSize += tmp.size;
            cdata = data.substr(totalSize);

        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (cdata[0] != '\x17')
            abort();

        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;

    }
};

std::string serialize(TranslationUnitReplacements const &TUR) {
    std::string res;

    res += header<TranslationUnitReplacements>(3);
    res += serialize(TUR.Context);
    res += serialize(TUR.MainSourceFile);
    res += serialize(TUR.Replacements);


#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<>
struct unserialize<TranslationUnitReplacements> {
    static withSizeInfo<TranslationUnitReplacements> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<TranslationUnitReplacements>::name())
            abort();

        if (hi.data.numberOfItems != 3)
            abort();
#endif

        withSizeInfo<TranslationUnitReplacements > res;
        std::size_t totalSize = hi.size;

        {
            auto tmp = unserialize<std::string >::do_unserialization(data.substr(totalSize));
            res.data.Context = std::move(tmp.data);
            totalSize += tmp.size;
        }


        {
            auto tmp = unserialize<std::string >::do_unserialization(data.substr(totalSize));
            res.data.MainSourceFile = std::move(tmp.data);
            totalSize += tmp.size;
        }

        {
            auto tmp = unserialize<std::vector<Replacement > >::do_unserialization(data.substr(totalSize));
            res.data.Replacements = std::move(tmp.data);
            totalSize += tmp.size;
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (totalSize >= data.size() || data[totalSize] != '\x17')
            abort();

        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;
    }
};

template <class T>
std::string serialize(llvm::StringMap<T> const &v) {
    std::string res;

    res += header<llvm::StringMap<T> >(v.size());

    for (auto &c : v) {
        res += serialize(std::make_pair(c.getKey(), c.getValue()));
    }


#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<class K>
struct unserialize<std::shared_ptr<llvm::StringMap<K> > > {
    static withSizeInfo<std::shared_ptr<llvm::StringMap<K> > > do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<llvm::StringMap<K> >::name())
            abort();
#endif

        withSizeInfo<std::shared_ptr<llvm::StringMap<K> > > res;
        std::size_t totalSize = hi.size;

        res.data = std::make_shared<llvm::StringMap<K> >();

        std::string cdata = data.substr(hi.size);
        for (std::size_t i = 0; i < hi.data.numberOfItems; ++i) {
            auto tmp = unserialize<std::pair<std::string, K> >::do_unserialization(cdata);

            (*(res.data))[tmp.data.first] = tmp.data.second;

            totalSize += tmp.size;
            cdata = data.substr(totalSize);

        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (cdata[0] != '\x17')
            abort();

        res.size = totalSize + 1;
#else 
        res.size = totalSize;
#endif

        return res;
    }
};

template <class K, class L>
std::string serialize(std::map<K, L> const &v) {
    std::string res;

    res += header<std::map<K, L> >(v.size());

    for (auto &c : v) {
        res += serialize(c);
    }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template <class K, class L>
struct unserialize<std::map<K, L> > {
    static withSizeInfo<std::map<K, L> > do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<std::map<K, L> >::name())
            abort();
#endif

        withSizeInfo<std::map<K, L> > res;
        std::size_t totalSize = hi.size;

        std::string cdata = data.substr(hi.size);
        for (std::size_t i = 0; i < hi.data.numberOfItems; ++i) {
            auto tmp = unserialize<std::pair<K, L> >::do_unserialization(cdata);

            res.data[tmp.data.first] = tmp.data.second;

            totalSize += tmp.size;
            cdata = data.substr(totalSize);

        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (cdata[0] != '\x17')
            abort();

        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;

    }
};

std::string serialize(CompileCommand const &cc) {
    std::string res;

    res += header<CompileCommand>(3);
    res += serialize(cc.CommandLine);
    res += serialize(cc.Directory);
    res += serialize(cc.MappedSources);


#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17';  // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<>
struct unserialize<CompileCommand> {
    static withSizeInfo<CompileCommand> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<CompileCommand>::name())
            abort();

        if (hi.data.numberOfItems != 3)
            abort();
#endif

        withSizeInfo<CompileCommand > res;
        std::size_t totalSize = hi.size;

        {
            auto tmp = unserialize<std::vector<std::string> >::do_unserialization(data.substr(totalSize));
            res.data.CommandLine = std::move(tmp.data);
            totalSize += tmp.size;
        }

        {
            auto tmp = unserialize<std::string>::do_unserialization(data.substr(totalSize));
            res.data.Directory = std::move(tmp.data);
            totalSize += tmp.size;
        }

        {
            auto tmp = unserialize<std::vector<std::pair<std::string, std::string> > >::do_unserialization(data.substr(totalSize));
            res.data.MappedSources = std::move(tmp.data);
            totalSize += tmp.size;
        }

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (totalSize >= data.size() || data[totalSize] != '\x17')
            abort();

        res.size = totalSize + 1;
#else
        res.size = totalSize;
#endif

        return res;
    }
};

std::string serialize(CompilationDatabase const &CDatabase) {
    std::string res;

    auto files = CDatabase.getAllFiles();

    res += header<CompilationDatabase>(files.size());

    for (auto &c : files) {
        res += serialize(c);
        res += serialize(CDatabase.getCompileCommands(c));
    }


#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    res += '\x17'; // ASCII C0 control char End of transmission block.
#endif

    return res;
}

template<>
struct unserialize<SimpleCompilationDatabase> {
    static withSizeInfo<SimpleCompilationDatabase> do_unserialization(std::string const &data) {
        auto hi = getHeadInfo(data);

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (hi.data.name != typeinfo<CompilationDatabase>::name())
            abort();
#endif

        std::size_t totalSize = hi.size;

        std::map<std::string, std::vector<CompileCommand> > tmp;

        std::string cdata = data.substr(totalSize);
        for (std::size_t i = 0; i < hi.data.numberOfItems; ++i) {
            auto tmp1 = unserialize<std::string>::do_unserialization(cdata);
            totalSize += tmp1.size;

            cdata = data.substr(totalSize);

            auto tmp2 = unserialize<std::vector<CompileCommand> >::do_unserialization(cdata);
            totalSize += tmp2.size;

            cdata = data.substr(totalSize);

            tmp[tmp1.data] = tmp2.data;

        }

        withSizeInfo<SimpleCompilationDatabase > res{ SimpleCompilationDatabase(tmp), totalSize };

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
        if (totalSize >= data.size() || data[totalSize] != '\x17')
            abort();

        res.size = totalSize + 1;
#endif

        return res;
    }
};

struct Context {
    std::string TransformId;
    std::unique_ptr<CompilationDatabase> CDatabase;

    Context() = default;
    Context(Context &) = delete;

    Context(Context &&other) {
        this->TransformId = std::move(other.TransformId);
        this->CDatabase = std::move(other.CDatabase);
    }
};

struct Response {
    unsigned int AcceptedChanges;
    unsigned int RejectedChanges;
    unsigned int DeferredChanges;
    bool ChangesNotMade;
    std::shared_ptr<TUReplacementsMap> ReplacementMap;
    std::map<std::string, std::set<std::string> > SourceMap;
    std::set<backport::ReplacementData> replacements;
    std::map<std::string, unsigned int> modifiedFilesCurrentLenght;

    Response() {}

    Response(Response const &o) {
        this->AcceptedChanges = o.AcceptedChanges;
        this->RejectedChanges = o.RejectedChanges;
        this->DeferredChanges = o.DeferredChanges;
        this->ChangesNotMade = o.ChangesNotMade;
        this->ReplacementMap = (o.ReplacementMap);
        this->SourceMap = o.SourceMap;
        this->replacements = o.replacements;
        this->modifiedFilesCurrentLenght = o.modifiedFilesCurrentLenght;
    }

    Response(Response &&o) {
        this->AcceptedChanges = o.AcceptedChanges;
        this->RejectedChanges = o.RejectedChanges;
        this->DeferredChanges = o.DeferredChanges;
        this->ChangesNotMade = o.ChangesNotMade;
        this->ReplacementMap = std::move(o.ReplacementMap);
        this->SourceMap = std::move(o.SourceMap);
        this->replacements = std::move(o.replacements);
        this->modifiedFilesCurrentLenght = std::move(o.modifiedFilesCurrentLenght);
    }

    Response &operator =(Response &&o) {
        this->AcceptedChanges = o.AcceptedChanges;
        this->RejectedChanges = o.RejectedChanges;
        this->DeferredChanges = o.DeferredChanges;
        this->ChangesNotMade = o.ChangesNotMade;
        this->ReplacementMap = std::move(o.ReplacementMap);
        this->SourceMap = std::move(o.SourceMap);
        this->replacements = std::move(o.replacements);
        this->modifiedFilesCurrentLenght = std::move(o.modifiedFilesCurrentLenght);
        return *this;
    }

    Response &operator =(Response const &o) {
        this->AcceptedChanges = o.AcceptedChanges;
        this->RejectedChanges = o.RejectedChanges;
        this->DeferredChanges = o.DeferredChanges;
        this->ChangesNotMade = o.ChangesNotMade;
        this->ReplacementMap = o.ReplacementMap;
        this->SourceMap = o.SourceMap;
        this->replacements = o.replacements;
        this->modifiedFilesCurrentLenght = o.modifiedFilesCurrentLenght;
        return *this;
    }
};

std::ostream &serializeData(std::ostream &os) {
    os.flush();
    return os;
}

template<typename T, typename... Args>
std::ostream &serializeData(std::ostream &os, const T& t, const Args&... args) {
    std::string tmp = serialize(t);
    os.write(tmp.c_str(), tmp.size());

#if (defined(_DEBUG ) || defined(DEBUG)) && defined(SERIALIZE_DEBUG)
    if (&os != &std::cout) {
        serializeData(std::cout, t);
    }
#endif

    return serializeData(os, args...);
}

Context buildDataContext(std::istream &is)  {

    std::string data;
    char tmp = 0;

    while (is.eof() == false && (is.read(&tmp, 1))) {
        data += tmp;
    }

    if (trim(data).empty()) {
        exit(1);
    }

    LOG(logDEBUG) << "Parent text length: " << data.size();

    Context result;

    {
        auto tmp = unserialize<std::string>::do_unserialization(data);
        result.TransformId = std::move(tmp.data);
        data = data.substr(tmp.size);
    }

    {
        auto tmp = unserialize<SimpleCompilationDatabase>::do_unserialization(data);
        result.CDatabase = llvm::make_unique<SimpleCompilationDatabase>(tmp.data);
        data = data.substr(tmp.size);
    }

    return std::move(result);
}

Response buildDataResponse(std::istream &is)  {


    std::string data;
    char tmp = 0;

    while (is.eof() == false && (is.read(&tmp, 1))) {
        data += tmp;
    }

    if (trim(data).empty()) {
        exit(1);
    }

    LOG(logDEBUG) << "Child text response length: " << data.size();

    Response result;

    {
        auto tmp = unserialize<unsigned int>::do_unserialization(data);
        result.AcceptedChanges = std::move(tmp.data);
        data = data.substr(tmp.size);
    }

    {
        auto tmp = unserialize<unsigned int>::do_unserialization(data);
        result.RejectedChanges = std::move(tmp.data);
        data = data.substr(tmp.size);
    }

    {
        auto tmp = unserialize<unsigned int>::do_unserialization(data);
        result.DeferredChanges = std::move(tmp.data);
        data = data.substr(tmp.size);
    }

    {
        auto tmp = unserialize<bool>::do_unserialization(data);
        result.ChangesNotMade = std::move(tmp.data);
        data = data.substr(tmp.size);
    }

    {
        auto tmp = unserialize<std::shared_ptr<TUReplacementsMap> >::do_unserialization(data);
        result.ReplacementMap = std::move(tmp.data);
        data = data.substr(tmp.size);
    }

    {
        auto tmp = unserialize<std::map<std::string, std::set<std::string> > >::do_unserialization(data);
        result.SourceMap = std::move(tmp.data);
        data = data.substr(tmp.size);
    }

    {
        auto tmp = unserialize<std::set<backport::ReplacementData> >::do_unserialization(data);
        result.replacements = std::move(tmp.data);
        data = data.substr(tmp.size);
    }

    {
        auto tmp = unserialize<std::map<std::string, unsigned int> >::do_unserialization(data);
        result.modifiedFilesCurrentLenght = std::move(tmp.data);
        data = data.substr(tmp.size);
    }


    return result;
}

}}

#endif
