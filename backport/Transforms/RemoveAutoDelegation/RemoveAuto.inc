#include "TransformBase/TransformHandler.h"
#include "Util/TransformUtility.h"
#include "RemoveAutoMatchers.h"

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;

typedef ReplacerTemplate<class AutoFuncDeclRem> AutoFuncDeclRemover;
typedef ReplacerTemplate<class AutoTemplFuncDeclRem> AutoTemplateFuncDeclRemover;
typedef ReplacerTemplate<class AutoTemplClassDeclRem> AutoTemplateClassDeclRemover;

// Removes functions that still contain auto expressions
template<>
void AutoFuncDeclRemover::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const FunctionDecl *FD = Result.Nodes.getNodeAs<clang::FunctionDecl>(RemAutoFunctionId);
    assert(FD && "Bad Callback. No node provided");

    if (!Owner.isFileModifiable(SM, FD->getLocStart())) {
        return;
    }

    auto range = CharSourceRange::getCharRange(
        SM.getSpellingLoc(FD->getLocStart()),
        SM.getSpellingLoc(functionDeclRangeEnd(FD)));

    Owner.addTracedReplacement(SM, range, "/* Unused Auto Containing Function Removed */\n");

    ++AcceptedChanges;
}

// Removes generic template function bodys that still contain auto expressions
template<>
void AutoTemplateFuncDeclRemover::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const FunctionTemplateDecl *TFD = Result.Nodes.getNodeAs<clang::FunctionTemplateDecl>(RemAutoTemplFunctionId);
    assert(TFD && "Bad Callback. No node provided");

    if (!Owner.isFileModifiable(SM, TFD->getLocStart())) {
        return;
    }

    auto range = getFullRange(SM, TFD->getAsFunction()->getBody()->getSourceRange());

    Owner.addTracedReplacement(SM, range, "/* Unused Auto Containing Template Function Body Removed */;\n");

    ++AcceptedChanges;
}

// Removes template classes that still contain auto expressions
template<>
void AutoTemplateClassDeclRemover::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const ClassTemplateDecl *TC = Result.Nodes.getNodeAs<clang::ClassTemplateDecl>(RemAutoTemplClassId);
    assert(TC && "Bad Callback. No node provided");

    if (!Owner.isFileModifiable(SM, TC->getLocStart())) {
        return;
    }

    auto Policy = getPrintingPolicy();

    std::string s1;
    llvm::raw_string_ostream rso1(s1);

    TC->print(rso1, Policy);

    // We only remove the class definition
    std::string classDeclaration = rso1.str().substr(0, rso1.str().find(TC->getNameAsString() + std::string(" {"), 0)) + TC->getNameAsString() + " /* Class Body Removed - Auto containing template class not instantiated */";

    auto range = CharSourceRange::getTokenRange(
        SM.getSpellingLoc(TC->getLocStart()),
        SM.getSpellingLoc(TC->getLocEnd()));

    Owner.addTracedReplacement(SM, range, classDeclaration);

    ++AcceptedChanges;
}
