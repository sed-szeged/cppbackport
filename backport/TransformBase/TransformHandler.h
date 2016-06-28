#ifndef BACKPORT_TRANSFORM_HANDLER_H
#define BACKPORT_TRANSFORM_HANDLER_H

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "TransformBase/Tooling.h"
#include "TransformBase/Transform.h"
#include "Replacer.h"
#include <vector>
#include <algorithm>
#include "Util/ReplacementData.h"
#include "Database/DatabaseInterface.h"

using namespace clang::ast_matchers;
using namespace clang;
using namespace clang::tooling;

enum class TransformPriorities : char {
    /**
      * Find the used C++ 11 features in a file
      *  so only the relevant transformations run
      */
    FIND_FEATURE = 99,

    /**
      * Modify auto return type template functions
      *  so that the return type is an extra template parameter
      * Modify variadic template functions
      *  so that the variadic parameter is a special templated list
      */
    MODIFY_FUNCTIONS = 26,

    /**
      * Instantiate template classas and functions
      *  where the type of auto is template dependent
      */
    INSTANTIATE_TEMPLATE = 15,

    /**
    * Transformations that have no other prerequisites
    */
    DEFAULT_PRIORITY = 10,

    /**
    * Replace Decltype.
    */
    REPLACE_DECLTYPE = 8,

    /**
    * Generate constructors.
    */
    GEN_CONSTR = 5,

    /**
    * - Deduce the type of auto where necessary
    * - Replaces template type aliases
    * - Replaces range-based for statements
    * - Transforms in-class member initializations.
    */
    MULTIPLE_TRANSFORMS = 4,

    /**
    * Replace rvalue.
    */
    REPLACE_RVALUE = 2,

    /**
    * - Remove functions and classes that use auto,
    *  but where not invoked or instantiated
    * - Transform delegating constructors
    */
    REMOVE_AUTO_DELEGATION = 1
};

class TransformHandler : public Transform {
public:
    TransformHandler(llvm::StringRef Name, const TransformOptions &Options, TransformPriorities Priority = TransformPriorities::DEFAULT_PRIORITY)
        : Transform(Name, Options), Priority(Priority) {}

    ~TransformHandler() {
        for (auto elem : Replaces) {
            delete elem;
        }
    }

    template<typename R, typename M>
    void initReplacers(R action, M matcher) {
        Replaces.push_back(action);
        Finder.addMatcher(matcher(), action);
    }

    template<typename R, typename M, typename... Rest>
    void initReplacers(R action, M matcher, Rest... rest) {
        Replaces.push_back(action);
        Finder.addMatcher(matcher(), action);
        initReplacers(rest...);
    }

    /// \see Transform::run().
    int apply(const clang::tooling::CompilationDatabase &Database,
        const std::vector<std::string> &SourcePaths) {

        // Run the transformation on the required files
        ClangTool Tool(Database, SourcePaths);
        if (int Result = Tool.run(createActionFactory(Finder).get())) {
            llvm::errs() << "Error encountered during translation.\n";
            return Result;
        }
        
        setAcceptedChanges(AcceptedChanges);
        return 0;
    }

    unsigned& getAcceptedChanges() { return AcceptedChanges; }
    TransformPriorities getPriority() const { return Priority; }

private:
    unsigned AcceptedChanges = 0;
    TransformPriorities Priority;
    std::vector<Replacer*> Replaces;
    MatchFinder Finder;
};

#endif // BACKPORT_TRANSFORM_HANDLER_H
