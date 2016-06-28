#include "TransformBase/TransformHandler.h"
#include "Util/TransformUtility.h"
#include "ReplaceDeclTypeMatchers.h"
#include "llvm/Support/Regex.h"
#include "Util/Log.h"
#include "Util/StringUtility.h"
#include <memory>
#include <utility>

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;

typedef ReplacerTemplate<class ReplaceDecltypeInDeclaratorDecl> DecltypeInDeclReplacer;
typedef ReplacerTemplate<class ReplaceDecltypeInStatments> DecltypeInStmtReplacer;
typedef ReplacerTemplate<class ReplaceDecltypeInTemplateArgument> DecltypeInTemplateArgReplacer;
typedef ReplacerTemplate<class ReplaceDecltypeInTypedef> DecltypeInTypedefReplacer;
typedef ReplacerTemplate<class ReplaceDecltypeInDefaultTemplateArg> DecltypeInDefaultTemplateArgReplacer;

namespace {
    namespace {
        std::shared_ptr<SplittableString> getDesugaredTypeString(QualType const &q, Transform &Owner,
            SourceManager &SM, ASTContext *Context);

        std::pair<std::shared_ptr<SplittableString>, bool> replacer(clang::QualType const &q, Transform &Owner,
            SourceManager &SM, ASTContext *Context) {
            // If it doesn't contain more decltype and the type is simple(the variable name is not inside of the type like in function types)
            // then just print.
            if (hasSubType(q, [](QualType const &t) { return dyn_cast<DecltypeType>(t.getTypePtr()) != nullptr; }) == false &&
                hasSubType(q, [](QualType const &t) { return t->isFunctionType() || t->isFunctionProtoType() || t->isPointerType(); }) == false &&
                hasSubType(q.getCanonicalType(), [](QualType const &t) { return dyn_cast<DecltypeType>(t.getTypePtr()) != nullptr; }) == false &&
                hasSubType(q.getCanonicalType(), [](QualType const &t) { return t->isFunctionType() || t->isFunctionProtoType() || t->isPointerType(); }) == false) {
                if (q.isConstQualified()) {
                    auto tmpttype = q.getCanonicalType();
                    tmpttype.removeLocalConst();
                    auto tmp = getDesugaredTypeString(tmpttype, Owner, SM, Context);
                    return std::make_pair<std::shared_ptr<SplittableString>, bool>(std::make_shared<SplittableString>(tmp->first + " const ", tmp->second), true);
                }
                else if (q.isVolatileQualified()) {
                    auto tmpttype = q.getCanonicalType();
                    tmpttype.removeLocalVolatile();
                    auto tmp = getDesugaredTypeString(tmpttype, Owner, SM, Context);
                    return std::make_pair<std::shared_ptr<SplittableString>, bool>(std::make_shared<SplittableString>(tmp->first + " volatile ", tmp->second), true);
                }
                else if (q->isRecordType() && q->getAsCXXRecordDecl() != nullptr) {
                    // if the type is an unnamed class the getName will name it.
                    return std::make_pair<std::shared_ptr<SplittableString>, bool>(std::make_shared<SplittableString>(getQualifiedNameAsStringWithTemplateArgs(q.getCanonicalType()->getAsCXXRecordDecl(), Owner, SM)), false);
                }
                else if (hasSubType(q.getCanonicalType(), [](QualType const &t) { return t->isRecordType() && t->getAsCXXRecordDecl() != nullptr && isUnnamed(t->getAsCXXRecordDecl()); })) {
                    return{ nullptr, false };
                }
                else {
                    return std::make_pair<std::shared_ptr<SplittableString>, bool>(std::make_shared<SplittableString>(printToString(q.getCanonicalType())), false);
                }
            }
            else if (dyn_cast<DecltypeType>(q.getTypePtr()) != nullptr) {
                // If desugaring doesn't get rid of a direct decltype try it again.

                if (dyn_cast<DecltypeType>(q.getTypePtr())->isSugared() && 
                    q.getCanonicalType() == q && dyn_cast<DecltypeType>(q.getCanonicalType().getTypePtr())) {
                    Owner.TransformationSourceMap[ReplaceDeclTypeID].insert(Owner.getCurrentSource());
                    return std::make_pair<std::shared_ptr<SplittableString>, bool>(nullptr, true);
                }
                else if (dyn_cast<DecltypeType>(q.getTypePtr())->isSugared() == false) {
                    return std::make_pair<std::shared_ptr<SplittableString>, bool>(getDesugaredTypeString(dyn_cast<DecltypeType>(q.getTypePtr())->getUnderlyingType(), Owner, SM, Context), true);
                }

                // Otherwise desugar current decltype and continue. (A desugared type can still contain 
                // decltype, for example as a function type's parameter type)
                return std::make_pair<std::shared_ptr<SplittableString>, bool>(getDesugaredTypeString(q.getCanonicalType(), Owner, SM, Context), true);
            }

            return std::make_pair<std::shared_ptr<SplittableString>, bool>(nullptr, false);
        }

        std::shared_ptr<SplittableString> getDesugaredTypeString(QualType const &q, Transform &Owner,
            SourceManager &SM, ASTContext *Context) {


            return replaceTypeRecursivly(q, Owner, SM, Context, replacer);
        }
    }


    template <class T>
    bool doDesugaredReplacement(T N, TypeSourceInfo const *TI, Transform &Owner,
        SourceManager &SM, ASTContext *Context, SourceLocation const &end,
        std::string const &leftAppend = "", std::string const &rightAppend = "")
    {
        auto res = getDesugaredTypeString(TI->getType(), Owner, SM, Context);

        if (res == nullptr)
            return false;

        Owner.addTracedReplacement(SM,
            CharSourceRange::getCharRange(TI->getTypeLoc().getSourceRange().getBegin(), end),
            leftAppend + getStorageClassAsString(N) + *res + rightAppend);


        return true;
    }

    template <class T>
    bool doDesugaredReplacement(T N, TypeSourceInfo const *TI, Transform &Owner,
        SourceManager &SM, ASTContext *Context, SourceLocation const &end,
        std::string const &leftAppend, std::string const &rightAppend, std::string variableName)
    {
        auto res = getDesugaredTypeString(TI->getType(), Owner, SM, Context);

        if (res == nullptr)
            return false;

        Owner.addTracedReplacement(SM,
            CharSourceRange::getCharRange(TI->getTypeLoc().getSourceRange().getBegin(), end),
            leftAppend + res->first + rightAppend);

        auto l1 = end.getLocWithOffset(variableName.size() - 1);
        auto l2 = TI->getTypeLoc().getEndLoc();

        if (less(SM, l1, l2))
            l1 = l2;

        replaceEnd(res, Owner, SM, N->getLocation().getLocWithOffset(variableName.size()), l1);

        return true;
    }

    template <class T>
    bool doDesugaredReplacement(T N, TypeSourceInfo const *TI, Transform &Owner,
        SourceManager &SM, ASTContext *Context,
        std::string leftAppend = "", std::string rightAppend = "")
    {
        auto l1 = TI->getTypeLoc().getBeginLoc().getLocWithOffset(TI->getType().getAsString().length());
        auto l2 = getTokenEnd(SM, TI->getTypeLoc().getSourceRange().getEnd());

        if (less(SM, l1, l2))
            l1 = l2;

        return doDesugaredReplacement(N, TI, Owner, SM, Context,
            l1,
            leftAppend, rightAppend);
    }

    template <class T>
    void ignoreMsg(clang::SourceManager &SM, T D) {
        LOG(TLogLevel::logDEBUG) << "Couldn't replace decltype '" << getSourceCode(SM, getFullRange(SM, D->getSourceRange())) << "'";
    }
}

/*template <class T = decltype(k)> class A {};*/
template<>
void DecltypeInDefaultTemplateArgReplacer::run(const MatchFinder::MatchResult &Result) {
    const TemplateTypeParmDecl *D{ Result.Nodes.getDeclAs<TemplateTypeParmDecl>(declTypeInTemplateParamDecl) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    if (doDesugaredReplacement(D, D->getDefaultArgumentInfo(), Owner, SM, Result.Context, "", " ")) {
        ++AcceptedChanges;
    }
    else ignoreMsg(SM, D);
}

/*typdef decltype(k) k_alias;*/
template<>
void DecltypeInTypedefReplacer::run(const MatchFinder::MatchResult &Result) {
    const TypedefDecl *D{ Result.Nodes.getDeclAs<TypedefDecl>(declTypeInTypedef) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    if (doDesugaredReplacement(D, D->getTypeSourceInfo(), Owner, SM, Result.Context, D->getLocation(), "", " ", D->getName())) {
        ++AcceptedChanges;
    }
    else ignoreMsg(SM, D);

}

/*Decltype replacing in parmVarDecl, fieldDecl, varDecl, and stuff like that.*/
template<>
void DecltypeInDeclReplacer::run(const MatchFinder::MatchResult &Result) {
    const DeclaratorDecl *D{ Result.Nodes.getDeclAs<DeclaratorDecl>(declTypeInDeclarator) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->isImplicit() || D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    auto end = D->getQualifierLoc().getBeginLoc();
    if (end.isInvalid())
        end = D->getLocation();

    if (doDesugaredReplacement(D, D->getTypeSourceInfo(), Owner, SM, Result.Context, end, "", " ", D->getName())) {
        ++AcceptedChanges;
    }
    else ignoreMsg(SM, D);

}

/*std::vector<decltype(k)> asdf;*/
template<>
void DecltypeInTemplateArgReplacer::run(const MatchFinder::MatchResult &Result) {
    const DeclaratorDecl *D{ Result.Nodes.getDeclAs<DeclaratorDecl>(declTypeInTemplateArgument) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    if (doDesugaredReplacement(D, D->getTypeSourceInfo(), Owner, SM, Result.Context, D->getLocation(), "", " ", D->getName())) {
        ++AcceptedChanges;
    }
    else ignoreMsg(SM, D);
}

/* Replace decltype in the following expressions: castExpr (including c-style and
 * c++-style named casts), UnaryExprOrTypeTraitExpr (sizeof) */
template<>
void DecltypeInStmtReplacer::run(const MatchFinder::MatchResult &Result) {
    const Stmt *D{ Result.Nodes.getDeclAs<Stmt>(declTypeInStatement) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    if (dyn_cast<Expr>(D) != nullptr) {
        Expr const *E = dyn_cast<Expr>(D);

        if (dyn_cast<ExplicitCastExpr>(E) != nullptr) {
            ExplicitCastExpr const *CastE = dyn_cast<ExplicitCastExpr >(E);

            if (doDesugaredReplacement(D, CastE->getTypeInfoAsWritten(), Owner, SM, Result.Context)) {
                ++AcceptedChanges;
            }
            else ignoreMsg(SM, CastE);

        } 
        else if (dyn_cast<UnaryExprOrTypeTraitExpr>(E) != nullptr) {
            UnaryExprOrTypeTraitExpr const *UE = dyn_cast<UnaryExprOrTypeTraitExpr>(E);

            if (doDesugaredReplacement(D, UE->getArgumentTypeInfo(), Owner, SM, Result.Context)) {
                ++AcceptedChanges;
            }
            else ignoreMsg(SM, UE);
        }
        else if (dyn_cast<CXXNewExpr>(E) != nullptr) {
            CXXNewExpr const *NE = dyn_cast<CXXNewExpr>(E);

            if (doDesugaredReplacement(D, NE->getAllocatedTypeSourceInfo(), Owner, SM, Result.Context, NE->getDirectInitRange().getBegin())) {
                ++AcceptedChanges;
            }
            else ignoreMsg(SM, NE);
        }
        else if (dyn_cast<DeclRefExpr>(E) != nullptr) {

            DeclRefExpr const *DRE = dyn_cast<DeclRefExpr>(E);

            if (dyn_cast<NonTypeTemplateParmDecl>(DRE->getDecl()) == nullptr) {
                ignoreMsg(SM, DRE);
                return;
            }

            auto parent = getFirstContainingDynTypedParent_which(DRE, *Result.Context, [](clang::ast_type_traits::DynTypedNode const &) { return true; });

            if (parent.get<VarDecl>() == nullptr) {
                ignoreMsg(SM, DRE);
                return;
            }

            auto decl = dyn_cast<NonTypeTemplateParmDecl>(DRE->getDecl());

            if (decl != nullptr) {
                auto end = decl->getQualifierLoc().getBeginLoc();
                if (end.isInvalid())
                    end = decl->getLocation();

                if (doDesugaredReplacement(decl, decl->getTypeSourceInfo(), Owner, SM, Result.Context, end, "", " ", decl->getName())) {
                    ++AcceptedChanges;
                }
                else ignoreMsg(SM, DRE);
            }

        }
        else ignoreMsg(SM, E);
    }
    else ignoreMsg(SM, D);
}



const char ReplaceDeclTypeID[] = "ReplaceDecltypeInDeclarations";

struct ReplaceDecltypeFactory : TransformFactory {
    ReplaceDecltypeFactory() {
        Since.Clang = Version(2, 9);
        Since.Gcc = Version(4, 3);
        Since.Icc = Version(12);
        Since.Msvc = Version(12);
    }

    Transform *createTransform(const TransformOptions &Opts) override {
        TransformHandler *handler = new TransformHandler(ReplaceDeclTypeID, Opts, TransformPriorities::REPLACE_DECLTYPE);
        auto &acc = handler->getAcceptedChanges();
        handler->initReplacers(
            new DecltypeInDeclReplacer(acc, *handler), makeDecltypeInDeclarationMatcher,
            new DecltypeInStmtReplacer(acc, *handler), makeDecltypeInStatementMatcher,
            new DecltypeInTemplateArgReplacer(acc, *handler), makeDecltypeInTemplateArgumentMatcher,
            new DecltypeInTypedefReplacer(acc, *handler), makeDeclTypeInTypedefMatcher,
            new DecltypeInDefaultTemplateArgReplacer(acc, *handler), makeDeclTypeInTemplateTypeParmDeclMacher
            );
        return handler;
    }
};

// Register the factory using this statically initialized variable.
static TransformFactoryRegistry::Add<ReplaceDecltypeFactory>
X("replace-decltype", "Replace decltype in declarations.");
