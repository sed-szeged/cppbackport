//===-- backport.cpp - Main file for Backport tool -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file implements the C++11 feature migration tool main function
/// and transformation framework.
///
/// See user documentation for usage instructions.
///
//===----------------------------------------------------------------------===//

#include "TransformBase/Transform.h"
#include "TransformBase/Transforms.h"
#include "TransformBase/TransformHandler.h"
#include "BackportManager/BackportManager.h"
#include "BackportManager/SourcePrepare.h"
#include "Util/Timer.h"
#include "Util/Log.h"
#include "Util/MultiprocessUtility.h"
#include "Util/PathUtility.h"
#include "Core/PerfSupport.h"
#include "Core/ReplacementHandling.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/Version.h"
#include "clang/Format/Format.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "TransformBase/Tooling.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Signals.h"

namespace cl = llvm::cl;
using namespace clang;
using namespace clang::tooling;

TransformOptions GlobalOptions;

// All options must belong to locally defined categories for them to get shown
// by -help. We explicitly hide everything else (except -help and -version).
static cl::OptionCategory GeneralCategory("Backport Options");
static cl::OptionCategory FormattingCategory("Formatting Options");
static cl::OptionCategory IncludeExcludeCategory("Inclusion/Exclusion Options");
static cl::OptionCategory SerializeCategory("Serialization Options");

const cl::OptionCategory *VisibleCategories[] = {
    &GeneralCategory, &FormattingCategory, &IncludeExcludeCategory,
    &SerializeCategory, &TransformCategory, &TransformsOptionsCategory,
};

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp(
    "EXAMPLES:\n\n"
    "Apply all transforms on a file that doesn't require compilation arguments:\n\n"
    "  backport file.cpp\n");

////////////////////////////////////////////////////////////////////////////////
/// General Options

// This is set to hidden on purpose. The actual help text for this option is
// included in CommonOptionsParser::HelpMessage.
static cl::opt<std::string> BuildPath("p", cl::desc("Build Path"), cl::Optional,
    cl::Hidden, cl::cat(GeneralCategory));

static cl::list<std::string> SourcePaths(cl::Positional,
    cl::desc("[<sources>...]"),
    cl::ZeroOrMore,
    cl::cat(GeneralCategory));

static cl::opt<TLogLevel, true> DebugLevel("log-level", cl::desc("Select log level"),
    cl::values(clEnumValN(logERROR, "error", "ERROR level"),
    clEnumValN(logWARNING, "warning", "WARNING level"),
    clEnumValN(logINFO, "info", "INFO level"),
    clEnumValN(logDEBUG, "debug", "DEBUG level"),
    clEnumValEnd),
    cl::location(Log::ReportingLevel()), cl::init(logINFO),
    cl::cat(GeneralCategory));

static cl::opt<bool> FinalSyntaxCheck(
    "final-syntax-check",
    cl::desc("Check for correct syntax after applying transformations"),
    cl::init(false), cl::cat(GeneralCategory));

/*
 * Command line argument for database path
 */
static cl::opt<std::string> dbFilePath(
    "dbfile",
    cl::desc("Filepath for the database"),
    cl::init("backport.db"), cl::cat(GeneralCategory));

/*
* Command line argument for destination directory
*/
static cl::opt<std::string> destinationDir(
    "dest",
    cl::desc("Before transforming files, all files will be copied to this directory"),
    cl::init("temp"), cl::cat(GeneralCategory));

/*
* Command line argument for target root directory
*  if not given, the tool will attempt to figure it out
*/
static cl::opt<std::string> sourceRootDir(
    "source-root",
    cl::desc("This is the root directory of the original project"),
    cl::init("AUTO DETECT"), cl::cat(GeneralCategory));

/*
* If this command line argument is set, the transformation will be applied to the original files
* FIXME : Needs testing or removing
*/
static cl::opt<bool> InPlace("in-place", cl::desc("Transform the files in place, don't make a copy"),
    cl::init(false), cl::cat(GeneralCategory));

/*
* If this command line argument is set, the program will use child processes to handle the transformations
*/
static cl::opt<bool> Multiprocess("multi", cl::desc("Use multiprocessing. Currently experimental"),
    cl::init(false), cl::cat(GeneralCategory));

/*
* If this command line argument is set, the program will run as a child process
*/
static cl::opt<bool> Children("child", cl::desc("For internal use only. This indicates that the process was called from a parent backport process."),
    cl::Hidden, cl::init(false), cl::cat(GeneralCategory));

/*
* Set the number of child processes used. Ignored when not multiprocessing. If not set, the default value will be the number of processors.
*/
static cl::opt<int> ChildProcessCount("proc", cl::desc("Set the number of child processes to use in multiprocessing. (Ignored if not multiprocessing)"),
    cl::init(-1), cl::cat(GeneralCategory));

/*
* Set the number of child processes used. Ignored when not multiprocessing. If not set, the default value will be the number of processors.
*/
static cl::opt<int> CUPerProcessCount("cu-per-proc", cl::desc("Set the number of compilation unit handled by each subprocess in multiprocessing. (Ignored if not multiprocessing)"),
    cl::init(2), cl::cat(GeneralCategory));


/*
* Command line argument to turn off the database feature
*  No db file will be created
*/
static cl::opt<bool> NoDatabase("no-db", cl::desc("Do not use database feature. Used for single file transformation testing"),
    cl::init(false), cl::cat(GeneralCategory));

/*
* Command line argument to reset the database at the start of every run
*  Used for testing
*/
static cl::opt<bool> ResetDatabase("reset-db", cl::desc("Reset the database before transformation. Will force all files to be processed. Used for testing"),
    cl::init(false), cl::cat(GeneralCategory));

static cl::opt<std::string>
TimingDirectoryName("perf",
cl::desc("Capture performance data and output to specified "
"directory. Default: ./migrate_perf"),
cl::ValueOptional, cl::value_desc("directory name"),
cl::cat(GeneralCategory));

cl::opt<std::string> SupportedCompilers(
    "for-compilers", cl::value_desc("string"),
    cl::desc("Select transforms targeting the intersection of\n"
    "language features not supported by the given compilers.\n"
    "Takes a comma-separated list of <compiler>-<version>.\n"
    "\t<compiler> can be any of: clang, gcc, icc, msvc\n"
    "\t<version> is <major>[.<minor>]\n"),
    cl::cat(GeneralCategory));

////////////////////////////////////////////////////////////////////////////////
/// Include/Exclude Options
static cl::opt<std::string>
IncludePaths("include",
cl::desc("Comma-separated list of paths to consider to be "
"transformed"),
cl::cat(IncludeExcludeCategory));

static cl::opt<std::string>
ExcludePaths("exclude", cl::desc("Comma-separated list of paths that can not "
"be transformed"),
cl::cat(IncludeExcludeCategory));

static cl::opt<std::string>
IncludeFromFile("include-from", cl::value_desc("filename"),
cl::desc("File containing a list of paths to consider to "
"be transformed"),
cl::cat(IncludeExcludeCategory));

static cl::opt<std::string>
ExcludeFromFile("exclude-from", cl::value_desc("filename"),
cl::desc("File containing a list of paths that can not be "
"transformed"),
cl::cat(IncludeExcludeCategory));

////////////////////////////////////////////////////////////////////////////////
/// Serialization Options

static cl::opt<bool>
SerializeOnly("serialize-replacements",
cl::desc("Serialize translation unit replacements to "
"disk instead of changing files."),
cl::init(false),
cl::cat(SerializeCategory));

static cl::opt<std::string>
SerializeLocation("serialize-dir",
cl::desc("Path to an existing directory in which to write\n"
"serialized replacements. Default behaviour is to\n"
"write to a temporary directory.\n"),
cl::cat(SerializeCategory));

////////////////////////////////////////////////////////////////////////////////

void printVersion() {
    llvm::outs() << "backport based on clang " CLANG_VERSION_STRING
        << "\n";
}

/// \brief Extract the minimum compiler versions as requested on the command
/// line by the switch \c -for-compilers.
///
/// \param ProgName The name of the program, \c argv[0], used to print errors.
/// \param Error If an error occur while parsing the versions this parameter is
/// set to \c true, otherwise it will be left untouched.
static CompilerVersions handleSupportedCompilers(const char *ProgName,
    bool &Error) {
    if (SupportedCompilers.getNumOccurrences() == 0)
        return CompilerVersions();
    CompilerVersions RequiredVersions;
    llvm::SmallVector<llvm::StringRef, 4> Compilers;

    llvm::StringRef(SupportedCompilers).split(Compilers, ",");

    for (llvm::SmallVectorImpl<llvm::StringRef>::iterator I = Compilers.begin(),
        E = Compilers.end();
        I != E; ++I) {
        llvm::StringRef Compiler, VersionStr;
        std::tie(Compiler, VersionStr) = I->split('-');
        Version *V = llvm::StringSwitch<Version *>(Compiler)
            .Case("clang", &RequiredVersions.Clang)
            .Case("gcc", &RequiredVersions.Gcc).Case("icc", &RequiredVersions.Icc)
            .Case("msvc", &RequiredVersions.Msvc).Default(nullptr);

        if (V == nullptr) {
            llvm::errs() << ProgName << ": " << Compiler
                << ": unsupported platform\n";
            Error = true;
            continue;
        }
        if (VersionStr.empty()) {
            llvm::errs() << ProgName << ": " << *I
                << ": missing version number in platform\n";
            Error = true;
            continue;
        }

        Version Version = Version::getFromString(VersionStr);
        if (Version.isNull()) {
            llvm::errs()
                << ProgName << ": " << *I
                << ": invalid version, please use \"<major>[.<minor>]\" instead of \""
                << VersionStr << "\"\n";
            Error = true;
            continue;
        }
        // support the lowest version given
        if (V->isNull() || Version < *V)
            *V = Version;
    }
    return RequiredVersions;
}

std::unique_ptr<CompilationDatabase>
autoDetectCompilations(std::string &ErrorMessage) {
    // Auto-detect a compilation database from BuildPath.
    if (BuildPath.getNumOccurrences() > 0)
        return CompilationDatabase::autoDetectFromDirectory(BuildPath,
        ErrorMessage);
    // Try to auto-detect a compilation database from the first source.
    if (!SourcePaths.empty()) {
        if (std::unique_ptr<CompilationDatabase> Compilations =
            CompilationDatabase::autoDetectFromSource(SourcePaths[0],
            ErrorMessage)) {
            // FIXME: just pass SourcePaths[0] once getCompileCommands supports
            // non-absolute paths.
            SmallString<64> Path(SourcePaths[0]);
            llvm::sys::fs::make_absolute(Path);
            std::vector<CompileCommand> Commands =
                Compilations->getCompileCommands(Path);

            // Ignore a detected compilation database that doesn't contain source0
            // since it is probably an unrelated compilation database.
            if (!Commands.empty())
                return Compilations;
        }
        // Reset ErrorMessage since a fix compilation database will be created if
        // it fails to detect one from source.
        ErrorMessage = "";
        // If no compilation database can be detected from source then we create a
        // fixed compilation database with c++11 support.
        std::string CommandLine[] = { "-std=c++11" };
        return llvm::make_unique<FixedCompilationDatabase>(".", CommandLine);
    }

    ErrorMessage = "Could not determine sources to transform";
    return nullptr;
}

// Predicate definition for determining whether a file is not included.
static bool isFileNotIncludedPredicate(backport::helper::Path FilePath) {
    return !GlobalOptions.ModifiableFiles.isFileIncluded(FilePath.strRef());
}

// Predicate definition for determining if a file was explicitly excluded.
static bool isFileExplicitlyExcludedPredicate(llvm::StringRef FilePath) {
    if (GlobalOptions.ModifiableFiles.isFileExplicitlyExcluded(FilePath)) {
        llvm::errs() << "Warning \"" << FilePath << "\" will not be transformed "
            << "because it's in the excluded list.\n";
        return true;
    }
    return false;
}

int main(int argc, const char **argv) {
    llvm::sys::PrintStackTraceOnErrorSignal();
    Timer timer;
    Transforms TransformManager;
    ReplacementHandling ReplacementHandler;

    TransformManager.registerTransforms();

    // Hide all options we don't define ourselves. Move pre-defined 'help',
    // 'help-list', and 'version' to our general category.
    llvm::StringMap<cl::Option*> Options;
    cl::getRegisteredOptions(Options);
    const cl::OptionCategory **CategoryEnd =
        VisibleCategories + llvm::array_lengthof(VisibleCategories);
    for (llvm::StringMap<cl::Option *>::iterator I = Options.begin(),
        E = Options.end();
        I != E; ++I) {
        if (I->first() == "help" || I->first() == "version" ||
            I->first() == "help-list")
            I->second->setCategory(GeneralCategory);
        else if (std::find(VisibleCategories, CategoryEnd, I->second->Category) ==
            CategoryEnd)
            I->second->setHiddenFlag(cl::ReallyHidden);
    }
    cl::SetVersionPrinter(&printVersion);

    // Parse options and generate compilations.
    std::unique_ptr<CompilationDatabase> Compilations(
        FixedCompilationDatabase::loadFromCommandLine(argc, argv));
    cl::ParseCommandLineOptions(argc, argv);

    // Child processes have limited functionality
    if (Children) {
        // The child processes use their standard output for communicating with the main process
        // They need to alert the logger so instead of the normal output, it'll use the error stream for all logs
        logSetChild(true);

        // The in and out streams of the child processes need to be set to binary to avoid some automatic character conversions
        ::backport::helper::setSTDOutToBinary();
        ::backport::helper::setSTDInToBinary();
        auto c = backport::serialization::buildDataContext(std::cin);

        // Initialize the database if it's not turned off for the child process
        backport::BackportManager bm(backport::helper::containerStringToPath(c.CDatabase->getAllFiles()), dbFilePath, !NoDatabase, false, true);
        GlobalOptions.BackportMgr = &bm;

        // Populate the ModifiableFiles structure.
        GlobalOptions.ModifiableFiles.readListFromString(IncludePaths, ExcludePaths);
        GlobalOptions.ModifiableFiles.readListFromFile(IncludeFromFile, ExcludeFromFile);

        bool CmdSwitchError = false;
        GlobalOptions.TargetVersions = handleSupportedCompilers(argv[0], CmdSwitchError);
        if (CmdSwitchError)
            return 1;

        if (c.TransformId.compare("DEPENDENCY") == 0) {
            bm.scanSources(*c.CDatabase, GlobalOptions);

            for (const auto& data : bm.dependencies) {
                LOG(logDEBUG) << "Dependency analyzed for " << data.first.path;
            }

            backport::serialization::serializeData(std::cout, bm.dependencies);
            return 0;
        } else {
            // Select the needed transformations based on the compiler we are trying to backport for
            TransformManager.createSelectedTransforms(GlobalOptions);

            if (bm.useDB()) {
                bm.getDb()->beginTransaction();
            }

            for (auto &T : TransformManager) {
                if (T->getName() == c.TransformId) {
                    if (T->apply(*c.CDatabase, c.CDatabase->getAllFiles()) != 0) {
                        return 1;
                    }

                    auto ReplacementsMap = &T->getAllReplacements();

                    backport::serialization::serializeData(std::cout, T->getAcceptedChanges(), T->getRejectedChanges(), T->getDeferredChanges(),
                        T->getChangesNotMade(), *ReplacementsMap, T->TransformationSourceMap, T->replacements, T->modifiedFilesCurrentLength);
                    break;
                }
            }

            if (bm.useDB()) {
                bm.getDb()->endTransaction();
            }
            return 0;
        }
    } // Child Process End

    LOG(logINFO) << "Backport version 1.74.7";

    // Populate the ModifiableFiles structure.
    GlobalOptions.ModifiableFiles.readListFromString(IncludePaths, ExcludePaths);
    GlobalOptions.ModifiableFiles.readListFromFile(IncludeFromFile, ExcludeFromFile);

    if (!Compilations) {
        std::string ErrorMessage;
        Compilations = autoDetectCompilations(ErrorMessage);
        if (!Compilations) {
            llvm::errs() << llvm::sys::path::filename(argv[0]) << ": " << ErrorMessage
                << "\n";
            return 1;
        }
    }

    // Populate source files.
    std::vector<backport::helper::Path> Sources;
    if (!SourcePaths.empty()) {
        // Use only files that are not explicitly excluded.
        std::remove_copy_if(SourcePaths.begin(), SourcePaths.end(),
            std::back_inserter(Sources),
            isFileExplicitlyExcludedPredicate);
    } else {
        if (GlobalOptions.ModifiableFiles.isIncludeListEmpty()) {
            llvm::errs() << llvm::sys::path::filename(argv[0])
                << ": Use -include to indicate which files of "
                << "the compilatiion database to transform.\n";
            return 1;
        }
        // Use source paths from the compilation database.
        // We only transform files that are explicitly included.
        auto compilationSources = Compilations->getAllFiles();
        Sources.clear();
        Sources.insert(Sources.begin(), compilationSources.begin(), compilationSources.end());
        std::vector<backport::helper::Path>::iterator E = std::remove_if(Sources.begin(), Sources.end(), isFileNotIncludedPredicate);
        Sources.erase(E, Sources.end());
    }
    if (Sources.empty()) {
        llvm::errs() << llvm::sys::path::filename(argv[0])
            << ": Could not determine sources to transform.\n";
        return 1;
    }

    // Enable timming.
    GlobalOptions.EnableTiming = TimingDirectoryName.getNumOccurrences() > 0;

    bool CmdSwitchError = false;
    GlobalOptions.TargetVersions = handleSupportedCompilers(argv[0], CmdSwitchError);
    if (CmdSwitchError)
        return 1;

    // Select the needed transformations based on the compiler we are trying to backport for
    TransformManager.createSelectedTransforms(GlobalOptions);

    // Initialize the database if it's not turned off
    backport::BackportManager bm(Sources, dbFilePath, !NoDatabase, ResetDatabase && !Children);
    GlobalOptions.BackportMgr = &bm;

    for (auto T : TransformManager) {
        LOG(logINFO) << "Transformation selected: " << T->getName();
    }

    if (TransformManager.begin() == TransformManager.end()) {
        if (SupportedCompilers.empty())
            LOG(logERROR) << llvm::sys::path::filename(argv[0])
            << ": no selected transforms";
        else
            LOG(logINFO) << llvm::sys::path::filename(argv[0])
            << ": no transforms available for specified compilers";

        return 1;
    }

    // If SerializeReplacements is requested, then code reformatting must be
    // turned off and only one transform should be requested.
    if (SerializeOnly && std::distance(TransformManager.begin(), TransformManager.end()) > 1) {
        LOG(logERROR) << "Serialization of replacements requested for multiple "
            "transforms.\nChanges from only one transform can be "
            "serialized.";
        return 1;
    }

    // If we're asked to apply changes to files on disk, need to locate
    // clang-apply-replacements.
    if (!SerializeOnly) {
        if (!ReplacementHandler.findClangApplyReplacements(argv[0])) {
            LOG(logERROR) << "Could not find clang-apply-replacements";
            return 1;
        }
    }

    StringRef TempDestinationDir;
    if (SerializeLocation.getNumOccurrences() > 0)
        ReplacementHandler.setDestinationDir(SerializeLocation);
    else
        TempDestinationDir = ReplacementHandler.useTempDestinationDir();

    // -------------------------------------------------------------------------------
    // -------------------------------------------------------------------------------


    timer.reset();
    LOG(logINFO) << "Analyzing dependencies...";

    // Handle the parameters for potential calls to child processes
    std::vector<std::string> params(argv + 1, argv + argc);
    std::vector<std::string> childParams;
    for (const auto& param : params) {
        if (param.compare(std::string("-multi")) != 0) {
            childParams.push_back(param);
        }
    }

    // The number of child processes used is will be ChildProcessCount if it's set. If not it'll be the number of processor, but always at least one
    std::size_t const numCores = std::max((std::size_t) 1, (std::size_t)(ChildProcessCount != -1 ? ChildProcessCount : (::backport::helper::hardware_concurrency())));

    childParams.push_back("-child");

    if (Multiprocess) {
        std::vector<backport::helper::Path> SourcesToAnalyze = Sources;

        std::size_t actualCUPerProcessCount = (std::size_t)CUPerProcessCount;

        // If the number of source groups is not even enough to fill numCores proc, then reduce the group size
        // to have at least numCores groups. If it is not possible (bc we have less files than numCores) than
        // just do it with 1 sized 'groups'.
        while ((SourcesToAnalyze.size() / actualCUPerProcessCount) < numCores && actualCUPerProcessCount > 1)
            --actualCUPerProcessCount;

        auto groups = backport::helper::group_with_pointers(SourcesToAnalyze, actualCUPerProcessCount);

        auto handler = std::bind(backport::helper::handleChildDependencyResult, std::placeholders::_1, std::ref(bm.dependencies));

        auto initializer = [&](Process &p, std::common_type<decltype(groups)>::type::const_reference currentGroup) {
            auto partialDatabase = ::backport::helper::getPartialDatabase(*Compilations, currentGroup);
            backport::serialization::serializeData(p.in(), std::string("DEPENDENCY"), *partialDatabase);
            delete partialDatabase;
            p.close_in();
        };

        auto progress = std::bind(backport::helper::drawProgressbarToLOG, std::placeholders::_1, std::placeholders::_2, "Progress of the dependency analysis: ");

        backport::helper::runProcesses(argv[0], numCores, childParams, groups, handler, initializer, progress, backport::helper::waitIndicatorToLOG);

        for (const auto& data : bm.dependencies) {
            LOG(logDEBUG) << "Dependency analyzed for " << data.first.path;
        }

    } else /* If Single Thread */ {
        bm.scanSources(*Compilations, GlobalOptions);
    }

    LOG(logINFO) << "Dependencies analyzed. Time: " << timer.duration();

    timer.reset();
    LOG(logINFO) << "Filtering not required source files...";
    Sources = bm.getFilteredSources();
    LOG(logINFO) << "Source files filtered. Time: " << timer.duration();

    LOG(logDEBUG) << "Files selected for transformation:";
    for (const auto& source : Sources) {
        LOG(logDEBUG) << "  " << source;
    }

    // Make a copy of the files and work on those
    // Note that if the database is turned off dependencies won't get copied
    if (!InPlace) {
        timer.reset();
        LOG(logINFO) << "Copying required files... ";
        backport::PrepareSources(Compilations, Sources, sourceRootDir, destinationDir, bm);
        LOG(logINFO) << "Source preparation complete. Time: " << timer.duration();
    }

    // Initialize the feature finder function
    const std::string FeatureFinderID = "FeatureFinder";
    auto& transformationSourceMap = ((Transform*)(*TransformManager.begin()))->TransformationSourceMap;
    if (((Transform*)(*TransformManager.begin()))->getName().compare(FeatureFinderID) == 0) {
        // If the first transformation to run is the FeatureFinder, then fill the transformationSourceMap with empty sets
        // It will be the job of the FeatureFinder to determine which transformations need to be run on which files
        std::set< backport::helper::Path > SourceSet(Sources.begin(), Sources.end());
        transformationSourceMap[FeatureFinderID] = SourceSet;
        for (Transforms::const_iterator I = ++(TransformManager.begin()),
            E = TransformManager.end();
            I != E; ++I) {
            Transform *T = *I;
            std::set< backport::helper::Path > EmptySet;
            transformationSourceMap[T->getName()] = EmptySet;
        }
    } else {
        // If the FeatureFinder is disabled, every transformation will be run on every file
        for (Transforms::const_iterator I = TransformManager.begin(),
            E = TransformManager.end();
            I != E; ++I) {
            Transform *T = *I;
            std::set< backport::helper::Path > SourceSet(Sources.begin(), Sources.end());
            transformationSourceMap[T->getName()] = SourceSet;
        }
    }

    SourcePerfData PerfData;

    LOG(logINFO) << "Transforming source files...";

    // Begin the transformation cycle
    for (Transforms::const_iterator I = TransformManager.begin(),
        E = TransformManager.end();
        I != E;) {

        Transform *T = *I;

        std::string TransformationName = T->getName();
        auto& TransformationSourceMap = T->TransformationSourceMap;

        timer.reset();
        // If the transformation is registered in the TransformationSourceMap or it isn't but there are source files, then there is work to do
        if (((TransformationSourceMap.find(TransformationName) != TransformationSourceMap.end()) && (TransformationSourceMap[TransformationName].size() > 0)) ||
            ((TransformationSourceMap.find(TransformationName) == TransformationSourceMap.end()) && (Sources.size() > 0))) {
            
            LOG(logINFO) << "Starting transformation: " << TransformationName;

            // The TransformationSourceMap contains for each transformation (the transformation ID name is the key) a list of files that the transformation needs to process
            std::vector<backport::helper::Path> SourcesToTransform = Sources;
            if (TransformationSourceMap.find(TransformationName) != TransformationSourceMap.end()) {
                auto& sources = TransformationSourceMap[TransformationName];
                SourcesToTransform = std::move(std::vector<backport::helper::Path>(sources.begin(), sources.end()));
                sources.clear();
            }

            // Apply the transformations
            unsigned int AcceptedChanges = 0;
            unsigned int RejectedChanges = 0;
            unsigned int DeferredChanges = 0;
            bool ChangesNotMade = false;
            TUReplacementsMap const* ReplacementsMap;
            TUReplacementsMap ReplacementsMapTMP; // Used in multiprocess to merge.

            if (Multiprocess) {

                auto handler = std::bind(backport::helper::handleChildProcessResult, std::placeholders::_1, std::ref(AcceptedChanges),
                    std::ref(RejectedChanges), std::ref(DeferredChanges), std::ref(ChangesNotMade), std::ref(ReplacementsMapTMP), std::ref(TransformationSourceMap),
                    std::ref(T->replacements), std::ref(T->modifiedFilesCurrentLength));

                std::size_t actualCUPerProcessCount = (std::size_t)CUPerProcessCount;

                // If the number of source groups is not even enough to fill numCores proc, then reduce the group size
                // to have at least numCores groups. If it is not possible (bc we have less files than numCores) than
                // just do it with 1 sized 'groups'.
                while ((SourcesToTransform.size() / actualCUPerProcessCount) < numCores && actualCUPerProcessCount > 1)
                    --actualCUPerProcessCount;

                auto groups = ::backport::helper::group_with_pointers(SourcesToTransform, actualCUPerProcessCount);

                auto initializer = [&](Process &p, std::common_type<decltype(groups)>::type::const_reference currentGroup) {
                    auto partialDatabase = ::backport::helper::getPartialDatabase(*Compilations, currentGroup);
                    backport::serialization::serializeData(p.in(), T->getName(), *partialDatabase);
                    delete partialDatabase;
                    p.close_in();
                };

                auto progress = std::bind(backport::helper::drawProgressbarToLOG, std::placeholders::_1, std::placeholders::_2, std::string("Progress of the transformation: ") + (std::string)T->getName());

                backport::helper::runProcesses(argv[0], numCores, childParams, groups, handler, initializer, progress, backport::helper::waitIndicatorToLOG);

                // apply results in the result map.
                ReplacementsMap = &ReplacementsMapTMP;

            } else /* If Single Thread */ {

                if (T->apply(*Compilations, backport::helper::containerPathToString(SourcesToTransform)) != 0) {
                    // FIXME: Improve ClangTool to not abort if just one file fails.
                    return 1;
                }

                AcceptedChanges = T->getAcceptedChanges();
                RejectedChanges = T->getRejectedChanges();
                DeferredChanges = T->getDeferredChanges();
                ChangesNotMade = T->getChangesNotMade();
                ReplacementsMap = &T->getAllReplacements();
            }

            if (T->getName().compare(FeatureFinderID) == 0) {
                LOG(logDEBUG) << "Transforms selected after FeatureFinder";
                for (const auto& transfom : T->TransformationSourceMap) {
                    if ((transfom.first.compare(FeatureFinderID) != 0) && (transfom.second.size() != 0)) {
                        LOG(logDEBUG) << "\t- " << transfom.first;
                        for (const auto& source : transfom.second) {
                            LOG(logDEBUG) << "\t\t- " << source.str();
                        }
                    }
                }
            }

            if (bm.useDB()) {
                bm.getDb()->endTransaction();
            }

            if (GlobalOptions.EnableTiming)
                collectSourcePerfData(*T, PerfData);

            LOG(logINFO) << "Transform: " << TransformationName << " - Accepted: " << AcceptedChanges;
            if (ChangesNotMade) {
                LOG(logINFO) << " - Rejected: " << RejectedChanges << " - Deferred: " << DeferredChanges;
            }

            // If the following code is enabled, then if a transformation runs without doing anything, it'll be an error
#ifdef FAIL_ON_UNNEDED_TRANSFORMATION
            extern const char RemoveAutoDelegationID[];
            if ((AcceptedChanges == 0) && (TransformationName.compare(FeatureFinderID) != 0) && (TransformationName.compare(RemoveAutoDelegationID) != 0)) {
                LOG(logERROR) << "Transform: " << TransformationName << " was run unnecessarrily";
                exit(1);
            }
#endif

            LOG(logINFO) << TransformationName << " replacements created in: " << timer.duration();
            if (!ReplacementHandler.serializeReplacements(*ReplacementsMap))
                return 1;

            timer.reset();

            if (!SerializeOnly)
            if (!ReplacementHandler.applyReplacements())
                return 1;

            if (bm.useDB()) {
                updateMappings(bm.getDb(), T->replacements, T->modifiedFilesCurrentLength);
            }

            LOG(logINFO) << TransformationName << " replacements applied in: " << timer.duration();
        }

        // If there are still files in the TransformationSourceMap for this transformation, then that means that it to run another cycle on those files
        if ((TransformationSourceMap.find(TransformationName) != TransformationSourceMap.end()) && (TransformationSourceMap[TransformationName].size() > 0)) {
            T->Reset();
        } else {
            ++I;
        }
    }

    // -------------------------------------------------------------------------------
    // -------------------------------------------------------------------------------

    // Let the user know which temporary directory the replacements got written
    // to.
    if (SerializeOnly && !TempDestinationDir.empty())
        LOG(logINFO) << "Replacements serialized to: " << TempDestinationDir;

    if (FinalSyntaxCheck) {
        int passCount = 0;
        std::vector<std::string> failedSources;
        timer.reset();
        LOG(logINFO) << "Checking syntax of applied replacements... ";
        for (const auto& source : Sources) {
            std::vector<std::string> partialSources;
            partialSources.push_back(source.str());
            ClangTool SyntaxTool(*Compilations, partialSources);
            if (SyntaxTool.run(newFrontendActionFactory<SyntaxOnlyAction>().get()) == 0) {
                ++passCount;
            } else {
                failedSources.push_back(source.str());
            }
        }
        LOG(logINFO) << "Syntax checked done " << passCount << "/" << Sources.size() << " passed";
        LOG(logINFO) << "Syntax checked in " << timer.duration();

        if (!failedSources.empty()) {
            LOG(logERROR) << "Syntax check failed for the following " << Sources.size() - passCount << " translation units";
            for (const auto& source : failedSources) {
                LOG(logERROR) << "\t- " << source;
            }
            return 1;
        }
    }

    // Report execution times.
    if (GlobalOptions.EnableTiming && !PerfData.empty()) {
        std::string DirectoryName = TimingDirectoryName;
        // Use default directory name.
        if (DirectoryName.empty())
            DirectoryName = "./migrate_perf";
        writePerfDataJSON(DirectoryName, PerfData);
    }

    if (!NoDatabase) {
        timer.reset();
        LOG(logINFO) << "Updating database... ";
        bm.updateDatabase();
        LOG(logINFO) << "Database updated. Time: " << timer.duration();
    }

    LOG(logINFO) << "Total Time: " << timer.duration(true);

    return 0;
}
