/**
 * Original version from http://www.drdobbs.com/cpp/logging-in-c/201804215?pgno=3
 */

#ifndef __LOG1_H__
#define __LOG1_H__

#include <sstream>
#include <string>
#include "llvm/Support/raw_os_ostream.h"

static bool child = false;

static inline void logSetChild(const bool& value) { child = value; }

inline std::string NowTime();

enum TLogLevel {logERROR, logWARNING, logINFO, logDEBUG};

class Log
{
public:
    Log();
    virtual ~Log();
    llvm::raw_string_ostream& Get(TLogLevel level = logINFO);
public:
    static TLogLevel& ReportingLevel();
    static std::string ToString(TLogLevel level);
    static TLogLevel FromString(const std::string& level);
protected:
    llvm::raw_string_ostream os;
    std::string temporary;
private:
    Log(const Log&);
    Log& operator =(const Log&);
};

inline Log::Log() : os(temporary)
{
}

inline llvm::raw_string_ostream& Log::Get(TLogLevel level)
{
    os << "- " << NowTime();
    os << " [" << ToString(level) << "] ";
    if (child) {
        os << "[CHILD] ";
    }
    os << std::string(level > logDEBUG ? level - logDEBUG : 0, '\t');
    return os;
}

inline Log::~Log()
{
    os << "\n";
    if (child) {
        llvm::errs() << os.str();
        llvm::errs().flush();
    } else {
        llvm::outs() << os.str();
        llvm::outs().flush();
    }
}

inline TLogLevel& Log::ReportingLevel()
{
    static TLogLevel reportingLevel = logINFO;
    return reportingLevel;
}

inline std::string Log::ToString(TLogLevel level)
{
    static const char* const buffer[] = {"ERROR", "WARNING", "INFO", "DEBUG"};
    return buffer[level];
}

inline TLogLevel Log::FromString(const std::string& level)
{
    if (level == "DEBUG")
        return logDEBUG;
    if (level == "INFO")
        return logINFO;
    if (level == "WARNING")
        return logWARNING;
    if (level == "ERROR")
        return logERROR;
    Log().Get(logWARNING) << "Unknown logging level '" << level << "'. Using INFO level as default.";
    return logINFO;
}

#define LOG(level) \
    if (level > Log::ReportingLevel()) ; \
    else Log().Get(level)

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

inline std::string NowTime()
{
    const int MAX_LEN = 200;
    char buffer[MAX_LEN];
    if (GetTimeFormatA(LOCALE_USER_DEFAULT, 0, 0, 
            "HH':'mm':'ss", buffer, MAX_LEN) == 0)
        return "Error in NowTime()";

    char result[100] = {0};
    static DWORD first = GetTickCount();
    std::sprintf(result, "%s.%03ld", buffer, (long)(GetTickCount() - first) % 1000); 
    return result;
}

#else

#include <sys/time.h>

inline std::string NowTime()
{
    char buffer[11];
    time_t t;
    time(&t);
    tm r = {0};
    strftime(buffer, sizeof(buffer), "%X", localtime_r(&t, &r));
    struct timeval tv;
    gettimeofday(&tv, 0);
    char result[100] = {0};
    std::sprintf(result, "%s.%03ld", buffer, (long)tv.tv_usec / 1000); 
    return result;
}

#endif //WIN32

#endif //__LOG_H__
