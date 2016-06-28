#include "DeduceAutoMatchers.h"

#include "TransformBase/TransformHandler.h"

#include "Util/TransformUtility.h"
#include "Util/StringUtility.h"
#include "Util/Log.h"

#include "clang/AST/RecursiveASTVisitor.h"

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;

// auto type
using SingleAutoReplacer = ReplacerTemplate<class SingleAuto>;
using MultipleAutoReplacer = ReplacerTemplate<class MultipleAuto>;
using AutoWithoutDeclStmtReplacer = ReplacerTemplate<class AutoWithoutDeclStmt>;
using MethodWithDeclTypeReplacer = ReplacerTemplate<class MethodWithDeclType>;

namespace {
    /**
    * \brief clang::RecursiveASTVisitor that searches for CXXConstructExpr node which satisfies the given requirements.
    */
    class NewExprFinderVisitor
        : public RecursiveASTVisitor<NewExprFinderVisitor> {
        friend class RecursiveASTVisitor<NewExprFinderVisitor>;

    public:
        NewExprFinderVisitor() { }

        /**
        * \brief Returns the first CXXConstructExpr which fulfills the given condition.
        */
        bool isCXXNewExprExists(const Stmt * stmt) {
            TraverseStmt(const_cast<Stmt *>(stmt));
            return found;
        }

    private:
        /**
        * \brief Returns true if new expr node is exists within the given stmt.
        */
        bool VisitCXXNewExpr(CXXNewExpr *) {
            found = true;
            return true;
        }
        
        bool found = false;
    };

    /**
    * \brief Deduces the given decl and it's init expression if neccessery. If prevDecl is not null then
    *        it deduces declarations in a multiple declaration context.
    */
    void createAutoTypeReplacement(Transform& Owner, SourceManager &SM, VarDecl const *decl, VarDecl const *prevDecl = nullptr) {
        // in case we don't have a prevDecl than it's a single decl or the first decl of a multiple declaration
        if (!prevDecl) {
            prevDecl = decl;
        }

        NewExprFinderVisitor Visitor;
        // new auto case
        if (Visitor.isCXXNewExprExists(decl->getInit())) {
            Owner.addTracedReplacement(SM, getFullRange(SM, decl->getInit()->getSourceRange()), printToString(decl->getInit()));
        }

        std::string typeName{ getType(decl) };
        // append storage classes only for declarations without previous decl.
        if (decl == prevDecl) {
            typeName = getStorageClassAsString(decl) + typeName;
        }
        else {
            QualType baseType = decl->getType();
            while (!baseType->getPointeeType().isNull())
                baseType = baseType->getPointeeType();
            std::string baseTypeName = printToString(baseType);
            // removes base type name and keeps the pointees
            typeName.erase(typeName.find_first_of(baseTypeName), baseTypeName.length());
        }

        auto locStart = decl->getLocStart();
        // correct the end location of the previous declaration if any
        if (decl != prevDecl) {
            locStart = getTokenEnd(SM, prevDecl->getLocEnd());
            locStart = locStart.getLocWithOffset(getLengthFromLocationUntilSeparator(SM, locStart, ',') + 1);
        }

        CharSourceRange typeRange;
        if (decl->getInit()->getType()->isFunctionPointerType()) {
            typeRange = CharSourceRange::getCharRange(locStart, decl->getLocation().getLocWithOffset(decl->getName().size()));
            typeName = getStorageClassAsString(decl) + backport::helper::rebuildFunctionPointerType(decl->getType(), decl->getNameAsString());
        }
        else {
            typeRange = CharSourceRange::getCharRange(locStart, decl->getLocation()); // SourceRange ends before the name of the variable.
        }
        Owner.addTracedReplacement(SM, typeRange, typeName);
    }
}

/**
 * \brief This function replaces single block scope auto declarations. Ex.: auto a = 32;
 */
template<>
void SingleAutoReplacer::run(const MatchFinder::MatchResult &Result) {
    const DeclStmt *D{ Result.Nodes.getNodeAs<DeclStmt>(SingleAutoDeclId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    const VarDecl* varDecl{ dyn_cast<VarDecl const>(D->getSingleDecl()) };
    createAutoTypeReplacement(Owner, SM, varDecl);

    ++AcceptedChanges;
}

/**
* \brief This function replaces multiple block scope auto declarations. Ex.: auto a = 32, b = 64;
*/
template<>
void MultipleAutoReplacer::run(const MatchFinder::MatchResult &Result) {
    const DeclStmt *D{ Result.Nodes.getNodeAs<DeclStmt>(MultipleAutoDeclId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    const VarDecl* varDecl{ nullptr };
    const VarDecl* prevVarDecl{ nullptr };
    for (auto decl : D->getDeclGroup()) {
        varDecl = dyn_cast<const VarDecl>(decl);
        assert(varDecl && "Expected VarDecl Node.");
        // after this we have to use the end location of the previous declaration in a multiple declaration to move after the comma separator and have a proper range
        // to be able to do this we use prevVarDecl
        createAutoTypeReplacement(Owner, SM, varDecl, prevVarDecl);
        prevVarDecl = varDecl;
    }
    ++AcceptedChanges;
}

/**
* \brief This function replaces single file scope auto declarations. Ex.: auto a = 32;
*/
template<>
void AutoWithoutDeclStmtReplacer::run(const MatchFinder::MatchResult &Result) {
    const VarDecl *V{ Result.Nodes.getNodeAs<VarDecl>(AutoDeclWithoutDeclStmtId) };
    assert(V && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, V->getLocStart())) {
        return;
    }

    createAutoTypeReplacement(Owner, SM, V);
    ++AcceptedChanges;
}

/**
* \brief This function replaces deduceable auto + suffix return type functions. Ex.: auto foo() -> decltype(3*2) { return 3*2; }
*/
template<>
void MethodWithDeclTypeReplacer::run(const MatchFinder::MatchResult &Result) {
    const FunctionDecl* M = Result.Nodes.getNodeAs<FunctionDecl>(DeclTypeMethodId);
    assert(M && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, M->getLocStart())) {
        return;
    }

    if (M->getReturnType()->isDependentType()) {
        return;
    }

    std::string type{ printToString(M->getReturnType().getCanonicalType()) + " " };
    SourceLocation retTypeLocEnd;
    // should not replace type qualifiers example namespace and scope resolvings
    if (M->getQualifierLoc().getBeginLoc().isValid()) {
        retTypeLocEnd = M->getQualifierLoc().getBeginLoc();
    }
    else {
        retTypeLocEnd = M->getLocation();
    }

    CharSourceRange range = CharSourceRange::getCharRange(M->getTypeSpecStartLoc(), retTypeLocEnd);
    Owner.addTracedReplacement(SM, range, type);

    SourceLocation afterPrototypeLoc = getAfterPrototypeLocation(SM, M);
    if (M->isThisDeclarationADefinition() && (M->getBody() != nullptr)) {
        range = CharSourceRange::getTokenRange(afterPrototypeLoc, M->getBody()->getLocStart().getLocWithOffset(-1));
        Owner.addTracedReplacement(SM, range, getFunctionQualifiers(M->getFunctionType()));
    } else {
        range = CharSourceRange::getTokenRange(
            SM.getSpellingLoc(afterPrototypeLoc),
            SM.getSpellingLoc(functionDeclRangeEnd(M)));
        Owner.addTracedReplacement(SM, range, getFunctionQualifiers(M->getFunctionType()) + ";\n");
    }

    ++AcceptedChanges;
}
