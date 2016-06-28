#ifndef BACKPORT_REPLACER_H
#define BACKPORT_REPLACER_H

#include "Core/Transform.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

/// \brief Wrapper class for Replacers
class Replacer
    : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    Replacer(unsigned &AcceptedChanges, Transform &Owner) : AcceptedChanges(AcceptedChanges), Owner(Owner) {}
    ~Replacer() {};

    /// \brief Entry point to the callback called when matches are made.
    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) = 0;
protected:
    unsigned &AcceptedChanges;
    Transform &Owner;
};

/// \brief Template class for Replacers to reduce code duplication
template<typename T>
class ReplacerTemplate
    : public Replacer {
public:
    ReplacerTemplate(unsigned &AcceptedChanges, Transform &Owner)
        : Replacer(AcceptedChanges, Owner) {}

    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override;
};

#endif // BACKPORT_REPLACER_H
