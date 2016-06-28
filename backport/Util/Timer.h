#ifndef BACKPORT_TIMER_H
#define BACKPORT_TIMER_H

#include <string>
#include <chrono>
#include <ctime>

/* Helper class for performance measurement. */
class Timer
{
public:
    void reset() {
        start = std::chrono::system_clock::now();
    }

    std::string duration(bool global = false) {
        auto rawDiff = std::chrono::system_clock::now() - (global ? globalStart : start);
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(rawDiff);
        
        long count = duration.count();

        std::string suffix = " second";
        suffix += (count > 1) ? "s" : "";
        if (count == 0) {
            count = std::chrono::duration_cast<std::chrono::milliseconds>(rawDiff).count();

            suffix = " millisecond";
            suffix += (count > 1) ? "s" : "";
        }

        return std::to_string(count) + suffix;
    }

private:
    std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> globalStart = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> start = std::chrono::system_clock::now();
};

#endif /*BACKPORT_TIMER_H*/
