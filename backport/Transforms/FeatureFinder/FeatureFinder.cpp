#include "TransformBase/TransformHandler.h"
#include "FeatureFinderMatchers.h"

#include "../RemoveAttributes/RemoveAttributes.inc"
#include "../RemoveAttributes/RemoveAttributesMatchers.h"

#include <type_traits>

using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace clang;

// Generic IDs for binding the maches
const char *FoundStmtID = "found_stmt";
const char *FoundDeclID = "found_decl";

// A templated replacer to handle matches
// If a match is made it will call the add method of the template parameter
//   This method is expected to add the current source file to the relevant transformations
template<typename NodeType, typename Action>
class FeatureFinder
    : public Replacer {
public:
    FeatureFinder(unsigned &AcceptedChanges, Transform &Owner)
        : Replacer(AcceptedChanges, Owner) {}

    void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
        const NodeType *F = Result.Nodes.getNodeAs<NodeType>(std::is_same<NodeType, Stmt>::value ? FoundStmtID : FoundDeclID);
        SourceManager &SM = *Result.SourceManager;
        if (F != nullptr && Owner.isFileModifiable(SM, F->getLocStart())) {
            Action a(Owner);
        }
    }
};

// Templated action which is called upon matches from FeatureFinder.
template<const char*... TransformIds>
class FindAction {
public:
    FindAction(Transform& Owner) {
        const char* Ids[sizeof...(TransformIds)] = { TransformIds... };
        for (auto Id : Ids) {
            Owner.TransformationSourceMap[Id].insert(Owner.getCurrentSource());
        }
    }
};

template<const char*... TransformIds>
using DeclFinder = FeatureFinder<Decl, FindAction<TransformIds...>>;

template<const char*... TransformIds>
using StmtFinder = FeatureFinder<Stmt, FindAction<TransformIds...>>;


// Transform Factory
struct FeatureFinderFactory : TransformFactory {
    FeatureFinderFactory() {
        Since.Clang = Version(3, 0);
        Since.Gcc = Version(4, 6);
        Since.Icc = Version(13);
        Since.Msvc = Version(12);
    }

    Transform *createTransform(const TransformOptions &Opts) override {
        TransformHandler *handler = new TransformHandler("FeatureFinder", Opts, TransformPriorities::FIND_FEATURE);
        auto &acc = handler->getAcceptedChanges();

        handler->initReplacers(new AttributesReplacer(acc, *handler), makeRemoveAttributesMatcher);

        handler->initReplacers(
#ifdef IMPLICIT_MOVE_CTRS
            // add constructor assignment
            new DeclFinder<GenConstructorsID, ReplaceRvalueID>(acc, *handler), findGenCtrsDeclMatcher,
#endif
            // modify variadic
            new DeclFinder<ModifyFunctionsID>(acc, *handler), findModifyVariadicDeclMatcher,
            new StmtFinder<ModifyFunctionsID>(acc, *handler), findModifyVariadicStmtMatcher,
            // modify auto
            new DeclFinder<ModifyFunctionsID>(acc, *handler), findModifyAutoDeclMatcher,
            new StmtFinder<ModifyFunctionsID>(acc, *handler), findModifyAutoStmtMatcher,
            // instantiate template
            new DeclFinder<InstantiateTemplateID>(acc, *handler), findInstantiateTemplateDeclMatcher,
            // lambda
            new StmtFinder<ReplaceLambdaID>(acc, *handler), findLambdaExprStmtMatcher,
            // decltype
            new DeclFinder<ReplaceDeclTypeID>(acc, *handler), findReplaceDecltypeInDeclMatcher,
            new StmtFinder<ReplaceDeclTypeID>(acc, *handler), findReplaceDecltypeInStmtMatcher,
            // rvalue
            new DeclFinder<ReplaceRvalueID>(acc, *handler), findReplaceRvalueDeclMatcher,
            new StmtFinder<ReplaceRvalueID>(acc, *handler), findReplaceRvalueStmtMatcher,
            // range-based for
            new StmtFinder<MultipleTransformsID>(acc, *handler), findForRangeStmtMatcher,
            // in-class member initialization
            new DeclFinder<MultipleTransformsID>(acc, *handler), findReplaceMemberInitDeclMatcher,
            // various auto types
            new DeclFinder<MultipleTransformsID>(acc, *handler), findDeduceAutoDeclMatcher,
            new StmtFinder<MultipleTransformsID>(acc, *handler), findDeduceAutoStmtMatcher,
            // type aliases
            new DeclFinder<MultipleTransformsID>(acc, *handler), findTypeAliasDeclMatcher,
            new StmtFinder<MultipleTransformsID>(acc, *handler), findTypeAliasStmtMatcher,
            // remove auto
            new DeclFinder<RemoveAutoDelegationID>(acc, *handler), findAutoFunctionDeclMatcher,
            // delegating constructor
            new DeclFinder<RemoveAutoDelegationID>(acc, *handler), findDelegatingConstructorDeclMatcher
            // unneeded attributes
            // new DeclFinder<RemoveAutoDelegationID>(acc, *handler), findRemoveAttributesDeclMatcher
        );
        return handler;
    }
};

// Register the factory using this statically initialized variable.
static TransformFactoryRegistry::Add<FeatureFinderFactory>
X("find-features", "Finds out what type of features each file contains, so only the relevant transformations will run");
