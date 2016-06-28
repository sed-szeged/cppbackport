#ifndef MULTIPROCESS_UTILITY_H
#define MULTIPROCESS_UTILITY_H

#include "Process/Process.h"
#include "Serialize/Serialize.h"
#include "Util/FileDatas.h"
#include <cstddef>
#include <iterator>
#include <algorithm>
#include <utility>
#include <functional>
#include <list>
#include <map>
#include "Util/ReplacementData.h"
#include <memory>
#include <vector>
#include <bitset>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace backport { namespace helper {

    CompilationDatabase* getPartialDatabase(const CompilationDatabase& database, const std::vector< std::string > &sourcefiles) {
        return new SimpleCompilationDatabase(database, sourcefiles);
    }

    CompilationDatabase* getPartialDatabase(const CompilationDatabase& database, const std::vector< const backport::helper::Path* > &sourcefiles) {
        std::vector<backport::helper::Path> helperVector;
        helperVector.reserve(sourcefiles.size());

        for (auto * item : sourcefiles) {
            helperVector.push_back(*item);
        }

        return getPartialDatabase(database, backport::helper::containerPathToString(helperVector));
    }

    template <class RangeType>
    CompilationDatabase* getPartialDatabase(const CompilationDatabase& database, const RangeType &sourcefiles) {
        auto const helperVector = std::vector<typename RangeType::value_type>(sourcefiles.begin(), sourcefiles.end());
        return getPartialDatabase(database, helperVector);
    }

    namespace {

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#if defined(BSD) || (defined(__APPLE__) && defined(__MACH__)) // BSD (FreeBSD, OpenBSD, Apple osx, etc)
#include <sys/param.h>
#include <sys/sysctl.h>
#elif defined(_hpux) || defined(hpux) || defined(__hpux) // HP-UX
#include <sys/mpctl.h>
#endif
#include <unistd.h>
#endif

    } /* Anonymous namespace */

    static unsigned hardware_concurrency() {
#ifdef _WIN32
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return sysinfo.dwNumberOfProcessors;
#elif defined( _SC_NPROCESSORS_ONLN ) // Linux, Solaris, & AIX and Mac OS X (for all OS releases >= 10.4, i.e., Tiger onwards)
        return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined( CTL_HW ) && defined(HW_AVAILCPU) && defined(HW_NCPU) // FreeBSD, MacOS X, NetBSD, OpenBSD, etc.:
        int nm[2];
        size_t len = 4;
        uint32_t count;

        nm[0] = CTL_HW;
        nm[1] = HW_AVAILCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);

        if (count < 1) {
            nm[1] = HW_NCPU;
            sysctl(nm, 2, &count, &len, NULL, 0);
            if (count < 1) { count = 1; }
        }
        return count;
#elif defined(MPC_GETNUMSPUS) // HP-UX
        return mpctl(MPC_GETNUMSPUS, NULL, NULL);
#elif defined(_SC_NPROC_ONLN) // IRIX
        return sysconf( _SC_NPROC_ONLN );
#endif
    }

    static void sleep(unsigned int milliseconds) {
#ifdef _WIN32
        Sleep(milliseconds);
#else // POSIX
        usleep((milliseconds % 1000) * 1000); // the argument of usleep must be less than 1 million.
#endif
    }

    void setSTDOutToBinary() {
#ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
#endif
    }

    void setSTDInToBinary() {
#ifdef _WIN32
        _setmode(_fileno(stdin), _O_BINARY);
#endif
    }

    std::string drawProgress(std::size_t const &total, std::size_t const &current, std::size_t totalLength) {
        
        std::stringstream ss;
        double percent = ((double)current / (double)total);
        
        std::string const total_str = std::to_string(total);
        std::string const current_str = std::to_string(current);

        std::size_t const otherInfolength = 2 /*the [ and the ] simbols*/ + 1 /*space */ + 5 /*percent number*/ + 1 /*percent sign*/ + total_str.size() * 2 + 6/*exact numbers*/;


        std::size_t progressBarLength;
        if (otherInfolength > totalLength || otherInfolength > totalLength + 10)
            progressBarLength = 10;
        else
            progressBarLength = totalLength - otherInfolength;

        std::size_t const numberOfBars = (std::size_t)(percent * progressBarLength);
        std::size_t const numberOfSpaces = progressBarLength - numberOfBars;

        ss << '[';

        ss << std::string(numberOfBars, '|');

        ss << std::string(numberOfSpaces, ' ');

        ss << "] ";

        ss << std::setw(5) << std::right << std::fixed << std::setprecision(1) << (percent*100) << std::setprecision(0) << std::setw(1) << "% ("
            << std::setw(total_str.size()) << current_str << std::setw(3) << " / " << total_str << std::setw(1) << ")";

        return ss.str();

    }

    void drawProgressbarToLOG(std::size_t const &total, std::size_t const &current, std::string const &desc) {
        LOG(logINFO) << desc; // newline
        LOG(logINFO) << " " << drawProgress(total, current, 50) << " done\n";
    }

    void waitIndicatorToLOG() {
        static int counter = 0;
        ++counter;
        llvm::outs() << "\r" << std::string(40, ' ');
        llvm::outs().flush();
        llvm::outs() << std::string("\r Waiting.") + std::string(counter, '.');
        llvm::outs().flush();

        if (counter > 5)
            counter = 0;
    }

    template<class T, class K, class L, class F1, class F2, class F3, class F4>
    static void runProcesses(L self, std::size_t const &numCores, K const &childParams, T const &GrouppedSourcesToTransform, F1 resultHandler, F2 initializer, 
        F3 progressIndicator, F4 waitIndicator) 
    {
        Process *processes = reinterpret_cast<Process *>(new char[sizeof(Process) * numCores]);

        std::vector<bool> active;
        active.resize(numCores);

        std::size_t numActive = 0;

        std::size_t numDone = 0;

        std::size_t total = GrouppedSourcesToTransform.size();

        std::size_t waitCounter = 0;

        for (std::size_t i = 0; i < total; ++i) {
            std::size_t empty = (std::size_t) -1;
            do {
                bool full = true;
                for (std::size_t j = 0; j < numCores; ++j) {
                    if (active[j] == false) {
                        empty = j;
                        full = false;
                        break;
                    }

                    auto &c = processes[j];
                    if (c.isDone()) {
                        resultHandler(c);
                        c.~Process();
                        
                        active[j] = false;
                        --numActive;
                        ++numDone;
                        empty = j;
                        full = false;
                        waitCounter = 0;

                        progressIndicator(total, numDone);

                        break;
                    }
                }

                if (full) {
                    ++waitCounter;

                    if (waitCounter > 1000) {
                        if (waitCounter % 40 == 0) {
                            waitCounter = 1000;
                            waitIndicator();
                        }
                    }

                    backport::helper::sleep(10);
                }
            }
            while ((numActive < numCores) == false);

            auto newchild = new(&(processes[empty])) Process();
            newchild->set_binary_mode(Process::stream_kind_t::s_all);
            newchild->start(self, childParams.begin(), childParams.end()); // start for the file SourcesToTransform[i].
            initializer(*newchild, GrouppedSourcesToTransform[i]);
            active[empty] = true;
            ++numActive;
        }

        LOG(logDEBUG) << "Every group has been sent to a child. Now waiting to get all the remaining responses.";
        LOG(logINFO) << "Almost done with this transformation...";

        waitCounter = 0;

        while (numActive > 0) {

            if (numActive > numCores) {
                LOG(logERROR) << "More processes are active than all the processes. This shouldn't happen. WTF.";
                std::abort();
            }

            for (std::size_t j = 0; j < numCores; ++j) {
                if (active[j] == false) {
                    continue;
                }

                auto &c = processes[j];
                if (c.isDone()) {
                    resultHandler(c);
                    c.~Process();

                    active[j] = false;
                    --numActive;
                    ++numDone;
                    waitCounter = 0;

                    progressIndicator(total, numDone);
                }
            }

            ++waitCounter;

            if (waitCounter > 1000) {
                if (waitCounter % 40 == 0) {
                    waitCounter = 1000;
                    waitIndicator();
                }
            }

            backport::helper::sleep(10);
        }

        delete[] reinterpret_cast<char *>(processes);
    }


    void cleanupPossibleWaitingIndicator() {
        llvm::errs() << "\r" << std::string(40, ' ') << "\r";
        llvm::errs().flush();

        llvm::outs() << "\r" << std::string(40, ' ') << "\r";
        llvm::outs().flush();
    }

    static Process &handleChildProcessResult(Process &cp,
        unsigned int &AcceptedChanges, unsigned int &RejectedChanges, unsigned int &DeferredChanges, bool &ChangesNotMade,
        TUReplacementsMap &ReplacementsMapTMP, std::map<std::string, std::set<backport::helper::Path> > &TransformationSourceMap,
        std::set<backport::ReplacementData> &replacements, std::map<std::string, unsigned int> &modifiedFilesCurrentLength) {

        cleanupPossibleWaitingIndicator();

        while (cp.err().eof() == false) {
            int c = cp.err().get();

            if (c > 0) {
                char cc = (char)c;
                llvm::errs().write(&cc, 1);
            }
        }


        if (cp.exit_code() != 0) {
            LOG(logDEBUG) << "The child process returned nonzero; exiting with the same exit code ( " << cp.exit_code() << " ).";
            exit(cp.exit_code());
        }

        Timer timer;
        auto response = backport::serialization::buildDataResponse(cp.out());

        LOG(logDEBUG) << "Deserialization took: " << timer.duration();

        timer.reset();


        AcceptedChanges += response.AcceptedChanges;
        RejectedChanges += response.RejectedChanges;
        DeferredChanges += response.DeferredChanges;
        ChangesNotMade = ChangesNotMade || response.ChangesNotMade;

        for (auto &c : *response.ReplacementMap) {
            ReplacementsMapTMP[c.getKey()].MainSourceFile = c.getValue().MainSourceFile;

            if (ReplacementsMapTMP[c.getKey()].Context.empty() == false && c.getValue().Context.empty() == false)
                ReplacementsMapTMP[c.getKey()].Context += "\n\n\\\\\\\\\\\\|||||//////\n\n" + c.getValue().Context;
            else if (ReplacementsMapTMP[c.getKey()].Context.empty() == false)
                ReplacementsMapTMP[c.getKey()].Context = c.getValue().Context;

            ReplacementsMapTMP[c.getKey()].Replacements.insert(ReplacementsMapTMP[c.getKey()].Replacements.end(),
                c.getValue().Replacements.begin(), c.getValue().Replacements.end());
        }

        for (auto &c : response.SourceMap) {
            TransformationSourceMap[c.first].insert(c.second.begin(), c.second.end());
        }

        replacements.insert(response.replacements.begin(), response.replacements.end());
        modifiedFilesCurrentLength.insert(response.modifiedFilesCurrentLenght.begin(), response.modifiedFilesCurrentLenght.end());


        LOG(logDEBUG) << "Merging the results took: " << timer.duration();

        return cp;
    }

    static Process &handleChildDependencyResult(Process &cp, std::map< backport::FileDatas, std::set< backport::FileDatas > > &dependencies) {

        cleanupPossibleWaitingIndicator();

        while (cp.err().eof() == false) {
            int c = cp.err().get();

            if (c > 0) {
                char cc = (char)c;
                llvm::errs().write(&cc, 1);
            }
        }


        if (cp.exit_code() != 0) {
            LOG(logDEBUG) << "The child process returned nonzero; exiting with the same exit code ( " << cp.exit_code() << " ).";
            exit(cp.exit_code());
        }

        std::string data;
        char tmp = 0;

        while (cp.out().eof() == false && (cp.out().read(&tmp, 1))) {
            data += tmp;
        }

        if (data.empty()) {
            LOG(logDEBUG) << "The child returned nothing in its standard output stream. Exiting with error code 1.";
            exit(1);
        }

        Timer timer;

        auto result = backport::serialization::unserialize< std::map< backport::FileDatas, std::set< backport::FileDatas > > >::do_unserialization(data);

        LOG(logDEBUG) << "Deserialization took: " << timer.duration();

        for (const auto& data : result.data) {
            auto& key = data.first;
            auto& value = data.second;
            if (dependencies.count(key) == 0) {
                dependencies[key] = std::move(value);
            }
            else {
                auto& fileDataSet = dependencies[key];
                fileDataSet.insert(value.begin(), value.end());
            }
        }

        return cp;
    }

    template <class T>
    std::vector<std::set<typename T::const_pointer> > group_with_pointers(T const &r, int count) {
        std::vector<std::set<typename T::const_pointer> > groupped;

        groupped.reserve((r.size() / count) + (r.size()%count == 0 ? 0 : 1));

        for (std::size_t i = 0; i < r.size(); ++i)
        {
            if ((i % count) == 0)
                groupped.push_back({});

            groupped.back().insert(&r[i]);
        }

        return groupped;
    }

    template <class T>
    std::vector<std::set<typename T::value_type> > group(T const &r, int count) {
        std::vector<std::set<typename T::value_type> > groupped;

        groupped.reserve((r.size() / count) + (r.size() % count == 0 ? 0 : 1));

        for (std::size_t i = 0; i < r.size(); ++i)
        {
            if ((i % count) == 0)
                groupped.push_back({});

            groupped.back().insert(r[i]);
        }

        return groupped;
    }

} /*namespace helper*/ } /*namespace backport*/

#endif // MULTIPROCESS_UTILITY_H
