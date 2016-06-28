#include "TransformBase/TransformHandler.h"
#include "Util/TransformUtility.h"
#include "Util/StringUtility.h"
#include "ReplaceMemberInitMatchers.h"
#include "llvm/Support/Regex.h"
#include <map>

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;

typedef ReplacerTemplate<class CtorMemberInit> CtorMemberInitReplacer;
typedef ReplacerTemplate<class RecordMemberInitWithoutCtor> RecordMemberInitWithoutCtorReplacer; // without constructor
typedef ReplacerTemplate<class RecordDeclMemberInit> RecordDeclMemberInitRemover;

namespace {
    /**
     * \brief Generates constructor initializer based on the given in-class initialized member.
     */
    void generateInitializationReplacement(FieldDecl* member, Expr* init, std::string& replacement, std::string printExpr) {
        auto Policy = getPrintingPolicy();
        if (member->getInClassInitStyle() == ICIS_ListInit) {
            backport::helper::RegexReplaceAll("^\\{(.*)\\}$", printExpr, "\\(\\1\\)");
            replacement += printExpr;
        }
        else { // there might be init-list in the other side of = mark.
            auto type = member->getType().getAsString(Policy);
            bool replaced = backport::helper::ReplaceAll(printExpr, type, "");
            if (replaced || member->getType()->isAggregateType()) {
                backport::helper::RegexReplaceAll("^\\((.*)\\)$", printExpr, "\\1");
            }
            backport::helper::RegexReplaceAll("^\\{(.*)\\}$", printExpr, "\\1");
            replacement += "(" + printExpr + ")";
        }
    }

    /**
     * \brief Generates constructor for aggregate types to fill in the possibility of in-class initialization
     */
    void generateConstructorForAggregateType(Transform& Owner, SourceManager const& SM, CXXRecordDecl *aggregate) {
        std::string genCtor = aggregate->getNameAsString();
        std::string params;
        std::string ctorInits;
        for (auto field : aggregate->fields()) {
            if (!params.empty()) {
                params += ", ";
            }
            if (!ctorInits.empty()) {
                ctorInits += ", ";
            }
            auto type = field->getType().getAsString(getPrintingPolicy());
            params += type + " " + field->getNameAsString() + " = ";
            if (field->getType()->isPointerType()) { // pointers will be initialized as null
                params += " NULL";
            }
            else {
                params += type + "()"; // who cares default ctor :)
            }
            ctorInits += field->getNameAsString() + "(" + field->getNameAsString() + ")";
        }

        genCtor += "(" + params + ") : " + ctorInits + " {}";
        auto range = CharSourceRange::getTokenRange(aggregate->getRBraceLoc(), aggregate->getRBraceLoc().getLocWithOffset(1));
        Owner.addTracedReplacement(SM, range, genCtor + "\n};");
    }
}
/**
 * \brief Replaces existing constructor initializer lists.
 */
template<>
void CtorMemberInitReplacer::run(const MatchFinder::MatchResult &Result) {
    const CXXConstructorDecl *Ctor{ Result.Nodes.getNodeAs<CXXConstructorDecl>(CtorDeclId) };
    const ClassTemplateDecl *TPL{ Result.Nodes.getNodeAs<ClassTemplateDecl>(ClassTemplateDeclId) };
    assert(Ctor && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, Ctor->getLocStart())) {
        return;
    }

    std::map<llvm::StringRef, FieldDecl*> generalMembers;
    if (TPL != nullptr) {
        for (auto member : TPL->getTemplatedDecl()->fields()) {
            if (!member->hasInClassInitializer())
                continue;
            generalMembers[member->getName()] = member;
        }
    }

    bool existConstructorInitializer = false;
    std::string replace;
    for (auto init : Ctor->inits()) {
        if (init->isInClassMemberInitializer()) {
            if (!replace.empty())
                replace += ", ";
            auto member = init->getMember();
            std::string exprStr;
            if (TPL == nullptr) {
                exprStr = printToString(member->getInClassInitializer());
            } else {
                auto initStr = printToString(generalMembers[member->getName()]->getInClassInitializer());
                exprStr += llvm::Regex("\\(\\( (.*) \\)\\)").sub("(\\1)", initStr);
            }
            replace += member->getNameAsString();
            generateInitializationReplacement(member, member->getInClassInitializer(), replace, exprStr);
            // generate constructor for aggregate types
            if (member->getType()->isAggregateType() && !member->getType()->isArrayType()) {
                generateConstructorForAggregateType(Owner, SM, member->getType()->getAsCXXRecordDecl());
            }
        }
        else if (init->isWritten())
            existConstructorInitializer = true;
    }
    if (replace.empty())
        return;

    if (existConstructorInitializer)
        replace = ", " + replace;
    else
        replace = " : " + replace;

    Owner.addTracedReplacement(SM, makeInsertionPoint(Ctor->getBody()->getLocStart()), replace);
    ++AcceptedChanges;
}

/**
 * \brief This function generates a default constructor in classes which have in-class initializers in case of no user provided constructor.
 */
template<>
void RecordMemberInitWithoutCtorReplacer::run(const MatchFinder::MatchResult &Result) {
    const CXXRecordDecl *Record{ Result.Nodes.getNodeAs<CXXRecordDecl>(ClassDeclId) };
    assert(Record && "Bad Callback. No node provided");
    
    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, Record->getLocStart())) {
        return;
    }

    std::string ctor;
    for (auto base : Record->bases()) {
        if (!ctor.empty())
            ctor += ", ";

        std::string type = base.getType().getAsString(getPrintingPolicy());
        ctor += type + "()";
    }


    for (auto member : Record->fields()) {
        if (!member->hasInClassInitializer()) {
            continue;
        }
        auto init = member->getInClassInitializer();
        if (!ctor.empty())
            ctor += ", ";

        auto initStr = printToString(init);
        ctor += member->getNameAsString();
        generateInitializationReplacement(member, init, ctor, initStr);
        // generate constructor for aggregate types
        if (member->getType()->isAggregateType() && !member->getType()->isArrayType()) {
            generateConstructorForAggregateType(Owner, SM, member->getType()->getAsCXXRecordDecl());
        }
    }

    ctor = "public: " + Record->getNameAsString() + "() : " + ctor + " {}\n";
    Owner.addTracedReplacement(SM, makeInsertionPoint(Record->getRBraceLoc()), ctor);
    ++AcceptedChanges;
}

/**
 * \brief This function removes in-class initializations in member declarations.
 */
template<>
void RecordDeclMemberInitRemover::run(const MatchFinder::MatchResult &Result) {
    const CXXRecordDecl *Record{ Result.Nodes.getNodeAs<CXXRecordDecl>(ClassDeclId) };
    assert(Record && "Bad Callback. No node provided");
    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, Record->getLocStart())) {
        return;
    }

    for (auto member : Record->fields()) {
        if (!member->hasInClassInitializer())
            continue;

        std::string memberRep;
        if (member->getType()->isArrayType()) {
            memberRep = getType(member);
            memberRep = llvm::Regex("\\[").sub(member->getNameAsString() + "[", memberRep);
        }
        else {
            memberRep = member->getType().getAsString(getPrintingPolicy()) + " " + member->getNameAsString();
        }

        CharSourceRange range = CharSourceRange::getTokenRange(member->getLocStart(), member->getInClassInitializer()->getLocEnd());
        Owner.addTracedReplacement(SM, range, memberRep);
    }
    ++AcceptedChanges;
}
