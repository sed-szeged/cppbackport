#include "TransformBase/TransformHandler.h"
#include "Util/TransformUtility.h"
#include "Util/StringUtility.h"
#include "ReplaceRvaluerefMatchers.h"
#include "llvm/Support/Regex.h"
#include <utility>
#include <memory>
#include "Util/Log.h"

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;

typedef ReplacerTemplate<class OneTimeInsert> OneTimeInserterDecl;

typedef ReplacerTemplate<class RvaluerefReplace> RvaluerefReplacer;

typedef ReplacerTemplate<class ReplaceStdMove> StdMoveReplacer;
typedef ReplacerTemplate<class LvalueRefInAssignReplace> LvalueRefInCopyAssignReplacer;

typedef ReplacerTemplate<class InsertCopyableAndMovableMacro> CopyableAndMovableMacroInserter;
typedef ReplacerTemplate<class InsertNotCopyableAndMovableMacro> NotCopyableAndMovableMacroInserter;
typedef ReplacerTemplate<class InsertCopyableAndNotMovableMacro> CopyableAndNotMovableMacroInserter;

typedef ReplacerTemplate<class InsertReturnValueMacro> ReturnValueMacroInserter;
typedef ReplacerTemplate<class ReplaceRvalueCast> RvalueCastReplacer;
typedef ReplacerTemplate<class InsertTemporaryMoveMacro> TemporaryMoveMacroInserter;

typedef ReplacerTemplate<class ReplaceRvalueInVarDeclNotParamDecl> RvalueInVarDeclNotParamDeclReplacer;
typedef ReplacerTemplate<class ReplaceImplicitXvalueCast> XvalueImplicitCastReplacer;

typedef ReplacerTemplate<class ReplaceMoveInitialization> MoveInitializationReplacer;


typedef ReplacerTemplate<class ReplaceRvalueRefInTemplateTypeParmDecl> RvalueRefInTemplateTypeParmDeclReplacer;
typedef ReplacerTemplate<class ReplaceRvalueRefInTypedef> ReplaceRvalueRefInTypedefReplacer;

typedef ReplacerTemplate<class ReplaceRvalueRefInDeclaratorDecl> RvalueRefInDeclaratorDeclReplacer;
typedef ReplacerTemplate<class ReplaceRvalueRefInConstructExpr> RvalueRefInConstructExprReplacer;
typedef ReplacerTemplate<class ReplaceRvalueRefInTemplateSpecArg> RvalueRefInTemplateSpecArgReplacer;

typedef ReplacerTemplate<class ReplaceRvalueInDeclaratorDeclThroughDeclRef> RvalueInDeclaratorDeclThroughDeclRefReplacer;

//    _   _        _                         
//   | | | |  ___ | | _ __    ___  _ __  ___ 
//   | |_| | / _ \| || '_ \  / _ \| '__|/ __|
//   |  _  ||  __/| || |_) ||  __/| |   \__ \
//   |_| |_| \___||_|| .__/  \___||_|   |___/
//                   |_|                     
namespace {
    /*We don't transform variadic functions.*/
    bool isVariadicFunction(SourceManager &SM, FunctionDecl const *f) {

        for (auto c : f->parameters()) {
            if (c->isParameterPack())
                return true;
        }

        if (getSourceCode(SM, f->getSourceRange()).find("VariadicTempslateParameterPackSubstitute") != std::string::npos)
            return true;


        return false;
    }

    bool isVariadicFunction(SourceManager &SM, FunctionDecl const *f, CallExpr const *cexpr) {

        bool test = isVariadicFunction(SM, f);

        if (test)
            return true;

        if (getSourceCode(SM, cexpr->getSourceRange()).find("VariadicTemplateList") != std::string::npos)
            return true;


        return false;
    }

    /*Is the statement is a calll for ::std::move?*/
    bool isMoveStatement(Stmt const *s) {
        if (dyn_cast_or_null<CallExpr>(s) != nullptr) {
            if (dyn_cast_or_null<CallExpr>(s)->getNumArgs() == 1 && 
                dyn_cast_or_null<CallExpr>(s)->getDirectCallee() != nullptr) 
            {
                auto nameMatcher = internal::HasNameMatcher("::std::move");

                                                            /*Upcast*/
                return nameMatcher.matchesNode(*static_cast<NamedDecl const *>
                    (dyn_cast_or_null<CallExpr>(s)->getDirectCallee()));
            }
        }

        return false;
    }

    /*Selects the first chiled which compatible to the type T. (depth first search)*/
    template <class T>
    T const *getFirstChild(Stmt const *s) {
        if (dyn_cast_or_null<T>(s) != nullptr)
            return static_cast<T const *>(s);

        for (auto c : s->children()) {
            getFirstChild<T>(c);
        }

        return nullptr;
    }

    /*Helper function for general rvalue replacing, in the function body.
     *Only used in backport transformation mode. */
    void makeRvalueToLvalueInFunction(Transform& Owner, SourceManager &SM, Stmt const *stmt, 
        ParmVarDecl const *param, int number, bool onlyReinterpret = false) 
    {
        // Rename the paramters which are rvalues to have '_____backport_numParam_' prefix
        Owner.addTracedReplacement(SM, makeInsertionPoint(param->getLocation()), "_____backport_" + std::to_string(number) + "_");

        // If the given function is not just a declaration but also a definition, 
        // then do the named rvalue is an lvalue thing, in the function body. 
        // (In the initializer list of constructor it is done with the castVarDeclRefExprInInitializer function)
        if (stmt) {

            /* If the name of the parameter in not given than we don't transfrom that paramter at all, 
             * bc it won't (and can't) be used in the function body anyway.*/
            if (trim(param->getDeclName().getAsString()).empty())
                return;

            std::string const typeName = printToString(param->getType().getNonReferenceType());

            if (onlyReinterpret == false &&
                param->getType().getNonReferenceType()->getCanonicalTypeInternal()->isRecordType() &&
                param->getType().getNonReferenceType()->getCanonicalTypeInternal()->isUnionType() == false) {
                Owner.addTracedReplacement(SM, makeInsertionPoint(stmt->getLocStart().getLocWithOffset(1)),
                    typeName + " " + param->getDeclName().getAsString() +
                    "( _____backport_" + std::to_string(number) + "_" + param->getDeclName().getAsString() + " );\n");
            }
            else {
                Owner.addTracedReplacement(SM, makeInsertionPoint(stmt->getLocStart().getLocWithOffset(1)),
                    typeName + " &" + param->getDeclName().getAsString() + " = " +
                    "*reinterpret_cast< " + typeName + " *> " +
                    "(&const_cast<char &>(reinterpret_cast<const volatile char &>( _____backport_" + 
                    std::to_string(number) + "_" + param->getDeclName().getAsString() + " )));\n");
            }
        }
    }

    std::shared_ptr<SplittableString> doRvalueReplacement(QualType const &q, Transform &Owner,
        SourceManager &SM, ASTContext *Context);

    std::pair<std::shared_ptr<SplittableString>, bool> replacer(clang::QualType const &q, Transform &Owner,
        SourceManager &SM, ASTContext *Context) {

        // If the current type is an rvalue type.
        if (q.getTypePtr()->isRValueReferenceType()) {
            // and if it contains no more rvalues inside of it then replace the current rvalue and just print the rest of the type.
            if (hasSubType(q.getCanonicalType().getNonReferenceType(), 
                    [](QualType const &t) { 
                    return t.getTypePtr()->isRValueReferenceType(); 
                }) == false) 
            {

                if (q.getNonReferenceType()->isRecordType() && q.getNonReferenceType()->getAsCXXRecordDecl() != nullptr &&
                    isUnnamed(q.getNonReferenceType()->getAsCXXRecordDecl())) {
                    return std::make_pair<std::shared_ptr<SplittableString>, bool>( 
                        std::make_shared<SplittableString>(std::string("::backport::rv< ") +
                        getQualifiedNameAsString(q.getNonReferenceType()->getAsCXXRecordDecl(), Owner, SM) + (q.getNonReferenceType().isConstQualified() ? " const" : "") + (q.getNonReferenceType().isVolatileQualified() ? " volatile" : "") + " > &"),
                        false 
                        );
                }
                else {
                    return std::make_pair<std::shared_ptr<SplittableString>, bool>( 
                        std::make_shared<SplittableString>(std::string("::backport::rv< ") +
                            printToString(q.getNonReferenceType().getUnqualifiedType()) + " > &"),
                        false 
                        );
                }
            }
            // or if it contains subsequential rvalues inside of it...
            else {
                assert(implementationKind == implementation::backport_impl && 
                    "We can't do complex replacements of rvalue types in non backport_impl mode!\n");
                if (implementationKind != implementation::backport_impl) {
                    LOG(TLogLevel::logERROR) << "We can't do complex replacements of rvalue types in non backport_impl mode!\n";
                    abort();
                }
                // then replace the current rvalue and then replace the rest of the rvalues in the type recursivly.
                return std::make_pair<std::shared_ptr<SplittableString>, bool>(  
                    std::make_shared<SplittableString>(
                        std::string("::backport::rv< ") + *doRvalueReplacement(q.getNonReferenceType(), Owner, SM, Context) + " > &"
                    ),
                    false 
                );
            }
        }

        // If the given type is not an rvalue and we don't need to put the variable name inside 
        // of the typename (like we would need to do in a function type),
        // then just print out the type.
        if (hasSubType(q.getCanonicalType(), [](QualType const &t) { return t.getTypePtr()->isRValueReferenceType(); }) == false &&
            q->isFunctionType() == false) {
            return std::make_pair<std::shared_ptr<SplittableString>, bool>(  std::make_shared<SplittableString>(printToString(q), ""), false );
        }

        // Otherwise expand the type.
        return std::make_pair<std::shared_ptr<SplittableString>, bool>( nullptr, false );

    }

    // WARNING: Only use this in backport implementation mode!
    std::shared_ptr<SplittableString> doRvalueReplacement(QualType const &q, Transform &Owner,
        SourceManager &SM, ASTContext *Context)
    {

        return replaceTypeRecursivly(q, Owner, SM, Context, replacer);
    }

    template <class T>
    void ignoreMsg(clang::SourceManager &SM, T D) {
        LOG(TLogLevel::logWARNING) << "Couldn't replace rvalue '" << getSourceCode(SM, getFullRange(SM, D->getSourceRange())) << "'\n";
        LOG(TLogLevel::logWARNING) << "Ignoring.\n";
    }

    /*Replace rvalue references in the function parameter list.*/
    template<class K>
    void replaceRvalueRefInParameters(llvm::ArrayRef<clang::ParmVarDecl *> params, Transform &Owner, clang::SourceManager &SM,
        K typedefsLoc, std::string uniqueId, bool onlyReinterpret = false) 
    {
        int counter = 0;
        for (auto p : params) {
            /*We only transform the paremeters which have rvalue ref type.*/
            if (hasSubType(p->getType(), [](QualType const &t) { return t->isRValueReferenceType(); })) {

                auto Policy = getPrintingPolicy();

                std::shared_ptr<SplittableString> aliasName;

                FunctionDecl const *parentF = static_cast<FunctionDecl const *>(p->getParentFunctionOrMethod());

                if (implementationKind == implementation::backport_impl) {
                    aliasName = doRvalueReplacement(p->getType(), Owner, SM, &p->getASTContext());

                    if (aliasName == nullptr) {
                        ignoreMsg(SM, p);
                        continue;
                    }

                    aliasName->insert(0, " ");
                    aliasName->first.insert(0, " ");

                    aliasName->append(" ");
                    aliasName->second.append(" ");

                    // If the parameter doesn't just have rvalue inside of it but is itself an rvalue so we need to move it
                    // than do the moving and the named rvalue is an lvalue in the function body.
                    if (p->getType()->isRValueReferenceType() ||p->getType()->getCanonicalTypeInternal()->isRValueReferenceType())
                        makeRvalueToLvalueInFunction(Owner, SM, parentF->getBody(), p, counter, onlyReinterpret);
                }
                else {
                    aliasName = std::make_shared<SplittableString>(" _____backport_replace_rvalue_alias_param_number_" + 
                        std::to_string(counter) + "_" + "uid_" + uniqueId + "_" +
                        std::to_string(SM.getSpellingLineNumber(p->getLocStart())) + "__" + 
                        std::to_string(SM.getSpellingColumnNumber(p->getLocStart())));

                    clang::SourceLocation typedefInsertLoc = typedefsLoc->getLocStart();

                    if (getFunctionTemplate(parentF, p->getASTContext()) != nullptr)
                        typedefInsertLoc = getFunctionTemplate(parentF, p->getASTContext())->getLocStart();

                    Owner.addTracedReplacement(SM, makeInsertionPoint(typedefInsertLoc), "\ntypedef " +
                        printToString(p->getType().getNonReferenceType()) + " " +
                        *aliasName
                        + ";\n");
                }
                
                if (implementationKind == implementation::backport_impl) {
                    Owner.addTracedReplacement(
                            SM, 
                            CharSourceRange::getCharRange(p->getLocStart(), p->getLocation()), 
                            aliasName->first + " "
                        );

                    replaceEnd(aliasName, Owner, SM, p->getLocation().getLocWithOffset(p->getName().size()), p->getLocEnd());
                }
                else {
                    std::string RVtype = std::string(getMoveMacro(rvalue_ref_macro) + " ( ") + *aliasName + " ) ";
                    Owner.addTracedReplacement(
                        SM, 
                        CharSourceRange::getCharRange(p->getLocStart(), p->getLocation()), 
                        RVtype + " "
                    );
                }
                
            }
            ++counter;
        }
    }

    /* Named rvalues are lvalues, in the initializer list we have to 'decay' from our rvalue type 
     * to lvalue every declRefExpr which is a ref to a paramVarDecl. In the function body it is not
     * needed bc we do this in the beginning of the function with the helper function: 'makeRvalueToLvalueInFunction' */
    void castVarDeclRefExprInInitializer(Transform &Owner, SourceManager &SM, ASTContext &AST, Stmt const *e) {
        if (dyn_cast<DeclRefExpr>(e) != nullptr) {
            ValueDecl const *decl = dyn_cast<DeclRefExpr>(e)->getDecl();
            if (dyn_cast_or_null<ParmVarDecl>(decl) != nullptr) {
                ParmVarDecl const *paramVarDecl = dyn_cast<ParmVarDecl>(decl);

                auto f = dyn_cast<FunctionDecl>(paramVarDecl->getDeclContext());

                int num = -1;

                for (auto c : f->parameters()) {
                    ++num;
                    if (paramVarDecl == c)
                        break;
                }

                if (num == -1 || num == f->getNumParams())
                    abort();

                Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(dyn_cast<DeclRefExpr>(e)->getLocStart(),
                    dyn_cast<DeclRefExpr>(e)->getLocStart().getLocWithOffset(paramVarDecl->getDeclName().getAsString().length())),
                    "(*reinterpret_cast< " + printToString(paramVarDecl->getType().getNonReferenceType()) + " *> " +
                    "(&const_cast<char &>(reinterpret_cast<const volatile char &>( _____backport_" + std::to_string(num) + 
                    "_" + paramVarDecl->getDeclName().getAsString() + " ))))");
            }
        }
        else {
            for (auto child : e->children()) {
                castVarDeclRefExprInInitializer(Owner, SM, AST, child);
            }
        }
    }

}


//  _____                      ____               _                   
// |_   _|_   _  _ __    ___  |  _ \  ___  _ __  | |  __ _   ___  ___ 
//   | | | | | || '_ \  / _ \ | |_) |/ _ \| '_ \ | | / _` | / __|/ _ \
//   | | | |_| || |_) ||  __/ |  _ <|  __/| |_) || || (_| || (__|  __/
//   |_|  \__, || .__/  \___| |_| \_\\___|| .__/ |_| \__,_| \___|\___|
//        |___/ |_|                       |_|                         

/*Replace rvalue reference in the vardecls' (/fielddecls'/nontypetemplateparmdecls') type (e.g.: my_vector<int&&, void (long &&)>). 
 *This is _NOT_ the named rvalue is an lvalue replacer.*/
template<>
void RvalueRefInDeclaratorDeclReplacer::run(const MatchFinder::MatchResult &Result) {
    const DeclaratorDecl *D{ Result.Nodes.getDeclAs<DeclaratorDecl>(replace_rvalue_in_declarator_type) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocEnd())) {
        return;
    }

    if (implementationKind != implementation::backport_impl) {
        LOG(TLogLevel::logERROR) <<"We can't do complex replacements of rvalue types in non backport_impl mode!\n";
        abort();
    }

    auto end = D->getQualifierLoc().getBeginLoc();
    if (end.isInvalid())
        end = D->getLocation();
    
    auto newType = doRvalueReplacement(D->getType(), Owner, SM, &D->getASTContext());

    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(D->getTypeSourceInfo()->getTypeLoc().getLocStart(), end),
        newType->first + " ");

    replaceEnd(newType, Owner, SM, D->getLocation().getLocWithOffset(D->getName().size()), D->getTypeSourceInfo()->getTypeLoc().getEndLoc());

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}


/*Replace rvalue reference in declarations. This is needed bc sometimes you can't match to them by declaratordecl, only using declrefexpr.*/
template<>
void RvalueInDeclaratorDeclThroughDeclRefReplacer::run(const MatchFinder::MatchResult &Result) {
    const DeclRefExpr *D{ Result.Nodes.getDeclAs<DeclRefExpr>(replace_rvalue_in_decl_through_refexpr) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocEnd())) {
        return;
    }

    if (implementationKind != implementation::backport_impl) {
        LOG(TLogLevel::logERROR) << "We can't do complex replacements of rvalue types in non backport_impl mode!\n";
        abort();
    }
    
    auto decl = D->getDecl();

    if (dyn_cast<NonTypeTemplateParmDecl>(decl) != nullptr) {

        auto tpdecl = dyn_cast<NonTypeTemplateParmDecl>(decl);

        auto newType = doRvalueReplacement(tpdecl->getType(), Owner, SM, &decl->getASTContext());

        auto end = tpdecl->getQualifierLoc().getBeginLoc();
        if (end.isInvalid())
            end = tpdecl->getLocation();

        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(tpdecl->getTypeSourceInfo()->getTypeLoc().getLocStart(), end), newType->first + " ");

        replaceEnd(newType, Owner, SM, tpdecl->getLocation().getLocWithOffset(tpdecl->getName().size()), tpdecl->getTypeSourceInfo()->getTypeLoc().getEndLoc());

        SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

        ++AcceptedChanges;
    }

}

template<>
void RvalueRefInTemplateSpecArgReplacer::run(const MatchFinder::MatchResult &Result) {
    const ClassTemplateSpecializationDecl *D{ Result.Nodes.getDeclAs<ClassTemplateSpecializationDecl>(replace_rvalue_in_template_specialization) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocEnd())) {
        return;
    }

    Owner.addTracedReplacement(SM, getFullRange(SM, D->getTypeAsWritten()->getTypeLoc().getSourceRange()), 
        *doRvalueReplacement(D->getTypeAsWritten()->getType(), Owner, SM, &D->getASTContext()) + " ");

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

/* ... = new my_vector<my_type (long &&)>*/
template<>
void RvalueRefInConstructExprReplacer::run(const MatchFinder::MatchResult &Result) {
    const CXXConstructExpr *D{ Result.Nodes.getDeclAs<CXXConstructExpr>(replace_rvalue_in_constructExpr) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocEnd())) {
        return;
    }

    if (implementationKind != implementation::backport_impl) {
        LOG(TLogLevel::logERROR) << "We can't do complex replacements of rvalue types in non backport_impl mode!\n";
        abort();
    }

    SourceLocation rendLoc = D->getParenOrBraceRange().getBegin();

    if (rendLoc.isInvalid())
        rendLoc = D->getLocEnd();

    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(D->getLocStart(), rendLoc), 
        *doRvalueReplacement(D->getType(), Owner, SM, Result.Context) + " ");

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

/*template <class T = int &&> class A {}; => template <class T = rv<T> &> class A {};*/
template<>
void RvalueRefInTemplateTypeParmDeclReplacer::run(const MatchFinder::MatchResult &Result) {
    const TemplateTypeParmDecl *D{ Result.Nodes.getDeclAs<TemplateTypeParmDecl>(replace_rvalue_in_template_def_val) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getLocation().isInvalid() || !Owner.isFileModifiable(SM, D->getLocation())) {
        return;
    }

    /*Don't transform implicit template specializations.*/
    auto parent = getFirstContainingDynTypedParent_which(D, *Result.Context, [](clang::ast_type_traits::DynTypedNode const &c) {
        return c.get<clang::CXXRecordDecl>() != nullptr ||
            c.get<clang::FunctionDecl>() != nullptr ||
            c.get<clang::TranslationUnitDecl>() != nullptr;
    });

    if (parent.get<clang::CXXRecordDecl>() != nullptr) {
        if (parent.get<clang::CXXRecordDecl>()->getTemplateSpecializationKind() ==
            clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {
            return;
        }
    }
    else if (parent.get<clang::FunctionDecl>() != nullptr) {
        if (parent.get<clang::FunctionDecl>()->getTemplateSpecializationKind() ==
            clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {
            return;
        }
    }

    if (implementationKind == implementation::backport_impl) {
        // We replace from the beginning bc the typeloc's beginloc, and the defaultargumentloc both ignore leading const that we want to remove
        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(D->getLocStart(), getTokenEnd(SM, D->getLocEnd())), std::string("class ") +
            (std::string)D->getName() + std::string(" = ") + *doRvalueReplacement(D->getDefaultArgument(), Owner, SM, &D->getASTContext()) + " ");
    }
    else {
        std::string const uid = std::to_string(SM.getFileID(D->getLocStart()).getHashValue());
        std::string const aliasName = " _____backport_replace_rvalue_type_alias_in_template_def_param_uid_" +
            uid +
            std::to_string(SM.getSpellingLineNumber(D->getLocStart())) + "__" + std::to_string(SM.getSpellingColumnNumber(D->getLocStart()));

        Owner.addTracedReplacement(SM, makeInsertionPoint(getFileSourceRange(D->getLocation(), SM).getBegin()), std::string("\n/*1000000000*/\n") + "typedef " +
            printToString(D->getDefaultArgument().getNonReferenceType()) + " " + aliasName + ";\n");

        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(D->getLocStart(), getTokenEnd(SM, D->getLocEnd())), std::string("class ") +
            (std::string)D->getName() + std::string(" = ") + getMoveMacro(move_keywords::rvalue_ref_macro) + " ( " + aliasName + " ) ");
    }

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

/*Replace rvalue in typedefs.*/
template<>
void ReplaceRvalueRefInTypedefReplacer::run(const MatchFinder::MatchResult &Result) {
    const TypedefDecl *D{ Result.Nodes.getDeclAs<TypedefDecl>(replace_rvalue_in_typedef) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getLocation().isInvalid() || !Owner.isFileModifiable(SM, D->getLocation())) {
        return;
    }

    /*Don't transform implicit template specializations.*/
    auto parent = getFirstContainingDynTypedParent_which(D, *Result.Context, [](clang::ast_type_traits::DynTypedNode const &c) {
        return c.get<clang::CXXRecordDecl>() != nullptr ||
            c.get<clang::FunctionDecl>() != nullptr ||
            c.get<clang::TranslationUnitDecl>() != nullptr;
    });

    if (parent.get<clang::CXXRecordDecl>() != nullptr) {
        if (parent.get<clang::CXXRecordDecl>()->getTemplateSpecializationKind() ==
            clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {
            return;
        }
    }
    else if (parent.get<clang::FunctionDecl>() != nullptr) {
        if (parent.get<clang::FunctionDecl>()->getTemplateSpecializationKind() ==
            clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {
            return;
        }
    }

    if (implementationKind == implementation::backport_impl) {
        auto typeT = doRvalueReplacement(D->getUnderlyingType(), Owner, SM, &D->getASTContext());
        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(D->getLocStart(), D->getLocation()), "typedef " + typeT->first);

        replaceEnd(typeT, Owner, SM, D->getLocation().getLocWithOffset(D->getName().size()), D->getLocEnd());
    }
    else {
        std::string const uid = std::to_string(SM.getFileID(D->getLocStart()).getHashValue());
        std::string const aliasName = " _____backport_replace_rvalue_type_alias_in_typedef_uid_" +
            uid +
            std::to_string(SM.getSpellingLineNumber(D->getLocStart())) + "__" + std::to_string(SM.getSpellingColumnNumber(D->getLocStart()));

        std::string const helperTypedef = std::string("\n/*1000000002*/\n") + "typedef " +
            printToString(D->getUnderlyingType().getNonReferenceType()) + " " + aliasName + ";\n";

        std::string const originalTypedef = std::string("typedef ") + getMoveMacro(move_keywords::rvalue_ref_macro) + " ( " + aliasName + " ) ";

        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(D->getLocStart(), D->getLocation()), helperTypedef + originalTypedef);

    }

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

//     ____  _                  
//    / ___|| |  __ _  ___  ___ 
//   | |    | | / _` |/ __|/ __|
//   | |___ | || (_| |\__ \\__ \
//    \____||_| \__,_||___/|___/
//                              

template<>
void CopyableAndMovableMacroInserter::run(const MatchFinder::MatchResult &Result) {
    const CXXRecordDecl *D{ Result.Nodes.getDeclAs<CXXRecordDecl>(InsertCopyableAndMoveableId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getRBraceLoc().isInvalid() || !Owner.isFileModifiable(SM, D->getRBraceLoc())) {
        return;
    }

    if (D->isUnion())
        return;

    /*Don't transform implicit template specializations.*/
    if (D->getTemplateSpecializationKind() ==
        clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {
        D = D->getTemplateInstantiationPattern();

        if (D->isUnion()) {
            return;
        }

        if (D->getRBraceLoc().isInvalid() || !Owner.isFileModifiable(SM, D->getRBraceLoc())) {
            return;
        }
    }

    std::string to_be_inserted;
    if (implementationKind == implementation::backport_impl) {
        return;
    }
    else {
        to_be_inserted = std::string("\n" + getMoveMacro(movable_and_copyable_macro) + "( ") + getNameWithoutTemplateParams(D, Owner, SM) + " ) " + " \n";
    }

    Owner.addTracedReplacement(SM,
        CharSourceRange::getCharRange(D->getRBraceLoc(), D->getRBraceLoc()),
        to_be_inserted);

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

template<>
void NotCopyableAndMovableMacroInserter::run(const MatchFinder::MatchResult &Result) {
    const CXXRecordDecl *D{ Result.Nodes.getDeclAs<CXXRecordDecl>(InsertNotCopyableAndMoveableId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getRBraceLoc().isInvalid() || !Owner.isFileModifiable(SM, D->getRBraceLoc())) {
        return;
    }

    if (D->isUnion())
        return;

    /*Don't transform implicit template specializations.*/
    if (D->getTemplateSpecializationKind() ==
        clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {
        D = D->getTemplateInstantiationPattern();

        if (D->isUnion()) {
            return;
        }

        if (D->getRBraceLoc().isInvalid() || !Owner.isFileModifiable(SM, D->getRBraceLoc())) {
            return;
        }
    }

    std::string to_be_inserted;
    if (implementationKind == implementation::backport_impl) {
        /*Delete the implicit assignment operators. (c++03 way)*/
        to_be_inserted = std::string("private:\n") + getNameWithoutTemplateParams(D, Owner, SM) + "(" + getNameWithoutTemplateParams(D, Owner, SM) + " &);\n" +
            getNameWithoutTemplateParams(D, Owner, SM) + " &operator=(" + getNameWithoutTemplateParams(D, Owner, SM) + " &);\n";
    }
    else {
        to_be_inserted = std::string("\n" + getMoveMacro(moveable_but_not_copyable_macro) + "( ") + getNameWithoutTemplateParams(D, Owner, SM) + " ) " + " \n";
    }

    Owner.addTracedReplacement(SM,
        CharSourceRange::getCharRange(D->getRBraceLoc(), D->getRBraceLoc()),
        to_be_inserted);

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

template<>
void CopyableAndNotMovableMacroInserter::run(const MatchFinder::MatchResult &Result) {
    /*It dosn't do anything right now so...*/
    return;

    const CXXRecordDecl *D{ Result.Nodes.getDeclAs<CXXRecordDecl>(InsertCopyableAndNotMoveableId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getRBraceLoc().isInvalid() || !Owner.isFileModifiable(SM, D->getRBraceLoc())) {
        return;
    }

    if (D->isUnion())
        return;

    /*Don't transform implicit template specializations.*/
    if (D->getTemplateSpecializationKind() ==
        clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {
        D = D->getTemplateInstantiationPattern();

        if (D->isUnion()) {
            return;
        }

        if (D->getRBraceLoc().isInvalid() || !Owner.isFileModifiable(SM, D->getRBraceLoc())) {
            return;
        }
    }

    // DO NOTHING
}

//    ____                                      _              
//   |  _ \  __ _  _ __  __ _  _ __ ___    ___ | |_  ___  _ __ 
//   | |_) |/ _` || '__|/ _` || '_ ` _ \  / _ \| __|/ _ \| '__|
//   |  __/| (_| || |  | (_| || | | | | ||  __/| |_|  __/| |   
//   |_|    \__,_||_|   \__,_||_| |_| |_| \___| \__|\___||_|   
//                                                             

/*replace copy assignment (operator=) parameter type*/
template<>
void LvalueRefInCopyAssignReplacer::run(const MatchFinder::MatchResult &Result) {
    const CXXMethodDecl *D{ Result.Nodes.getNodeAs<CXXMethodDecl>(LvalueRefInCopyAssignId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    if (D->getParent()->isUnion())
        return;

    if (implementationKind == implementation::backport_impl) {
        return;
    }

    auto kind = D->getTemplateSpecializationKind();

    if (kind == clang::TemplateSpecializationKind::TSK_ImplicitInstantiation)
        return;

    auto firstParam = D->parameters().front();
    auto policy = getPrintingPolicy();

    std::string const uid = std::to_string(SM.getFileID(D->getLocStart()).getHashValue()) + 
        "_" + std::to_string(D->getNumParams()) + "_" + std::to_string(D->getBuiltinID());

    std::string aliasName;

    aliasName = " _____backport_replace_lvalue_alias_in_copy_assignm_uid_" +
        uid +
        std::to_string(SM.getSpellingLineNumber(D->getLocStart())) + "__" + 
        std::to_string(SM.getSpellingColumnNumber(D->getLocStart()));

    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(D->getLocStart(), D->getLocStart()), "\ntypedef " +
        printToString(firstParam->getType().getNonReferenceType().getLocalUnqualifiedType()) + " " + aliasName + ";\n");


    std::string refStringReplace;
    refStringReplace = std::string(getMoveMacro(assignment_op_macro) + "( ") + aliasName + " ) ";


    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(firstParam->getLocStart(),
        firstParam->getTypeSourceInfo()->getTypeLoc().getLocEnd().getLocWithOffset(1)), refStringReplace);

    replaceRvalueRefInParameters(D->parameters().slice(1), Owner, SM, D, uid);

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

/*General rvalue in function paramter replacer*/
template<>
void RvaluerefReplacer::run(const MatchFinder::MatchResult &Result) {
    const FunctionDecl *D{ Result.Nodes.getNodeAs<FunctionDecl>(RvalueRefId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    if (isVariadicFunction(SM, D) || 
        (
            getFunctionTemplate(D, D->getASTContext()) && 
            isVariadicFunction(SM, getFunctionTemplate(D, D->getASTContext())->getAsFunction())
        )
       )
        return;

    auto kind = D->getTemplateSpecializationKind();

    if (kind == clang::TemplateSpecializationKind::TSK_ImplicitInstantiation)
        return;

    if (D->getReturnType()->isRValueReferenceType() || D->getReturnType().getCanonicalType()->isRValueReferenceType() ||
        hasSubType(D->getReturnType(), [](QualType const &t) { return t->isRValueReferenceType(); })) {

        const auto range = clang::SourceRange(D->getReturnTypeSourceRange().getBegin(), D->getReturnTypeSourceRange().getEnd().getLocWithOffset(2));

        if (implementationKind == implementation::backport_impl) {
            Owner.addTracedReplacement(SM, range, *doRvalueReplacement(D->getReturnType(), Owner, SM, &D->getASTContext()) + " ");
        }
        else {
            Owner.addTracedReplacement(SM, range, getMoveMacro(rvalue_ref_macro) + " ( " + printToString(D->getReturnType().getNonReferenceType()) + " ) ");
        }
    }
        

    std::string const uid = std::to_string(SM.getFileID(D->getLocStart()).getHashValue()) + "_" + std::to_string(D->getNumParams()) + "_" + std::to_string(D->getBuiltinID());

    bool operatorOverload = 
        (dyn_cast<CXXMethodDecl>(D) != nullptr && dyn_cast<CXXMethodDecl>(D)->isOverloadedOperator() /*&&
        (dyn_cast<CXXMethodDecl>(D)->isCopyAssignmentOperator() || dyn_cast<CXXMethodDecl>(D)->isMoveAssignmentOperator())*/);
    bool constructorDecl = dyn_cast<CXXConstructorDecl>(D) != nullptr;
    bool onlyReinterpret = operatorOverload || constructorDecl;

    // Transform the initializer list.
    if (constructorDecl && implementationKind == implementation::backport_impl) {
        auto constructor = dyn_cast<CXXConstructorDecl>(D);
        for (auto init : constructor->inits()) {
            castVarDeclRefExprInInitializer(Owner, SM, *Result.Context, init->getInit());
        }
    }

    replaceRvalueRefInParameters(D->parameters(), Owner, SM, D, uid, onlyReinterpret);

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

//   __     __            ____               _ 
//   \ \   / /__ _  _ __ |  _ \   ___   ___ | |
//    \ \ / // _` || '__|| | | | / _ \ / __|| |
//     \ V /| (_| || |   | |_| ||  __/| (__ | |
//      \_/  \__,_||_|   |____/  \___| \___||_|
//                                             

/*Turn A &&varName = A(); => A varName = A(); (Named rvalue is an lvalue.)*/
template<>
void RvalueInVarDeclNotParamDeclReplacer::run(const MatchFinder::MatchResult &Result) {
    const VarDecl *D{ Result.Nodes.getDeclAs<VarDecl>(rvlaue_ref_in_vardecl_not_paramdecl) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getSourceRange().getBegin())) {
        return;
    }

    //FIXME: Merge into the declaratordecl type replacer.
    Owner.addTracedReplacement(SM,
        CharSourceRange::getCharRange(D->getLocStart(), D->getLocation()),
        printToString(D->getType().getNonReferenceType().getUnqualifiedType()) + " ");

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}


/*Turn A name = std::move(other); into valid move (A name ((rvalue_type<A>(other))));*/
template<>
void MoveInitializationReplacer::run(const MatchFinder::MatchResult &Result) {
    const CXXConstructExpr *D{ Result.Nodes.getDeclAs<CXXConstructExpr>(replace_move_initialization) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getSourceRange().getBegin())) {
        return;
    }

    if (implementationKind != backport_impl)
        return;

    auto parent = getFirstContainingDynTypedParent_which(D, *Result.Context, [](ast_type_traits::DynTypedNode const &node) { return true; });
    VarDecl const *parentVarDecl = nullptr;
    if (parent.get<VarDecl>() == nullptr && parent.get<ExprWithCleanups>() != nullptr) {
        parent = getFirstContainingDynTypedParent_which(parent.get<ExprWithCleanups>(), *Result.Context, [](ast_type_traits::DynTypedNode const &node) { return true; });
    }

    parentVarDecl = parent.get<VarDecl>();

    /*The type is not a class type or doesn't have a move constructor defined by the user or 
     * or it is in a file which we can't modify (STL classes like std::string).*/
    if (parentVarDecl == nullptr ||
       parentVarDecl->getType()->isUnionType() ||
       parentVarDecl->getType()->isRecordType() == false)
        return;

    if (parentVarDecl->getType()->getAsCXXRecordDecl()->hasUserDeclaredMoveConstructor() == false ||
        Owner.isFileModifiable(SM, parentVarDecl->getType()->getAsCXXRecordDecl()->getRBraceLoc()) == false)
        return;

    if (dyn_cast<AutoType>(parentVarDecl->getType()) != nullptr) {
        auto typeRange = CharSourceRange::getCharRange(parentVarDecl->getTypeSourceInfo()->getTypeLoc().getLocStart(),
            parentVarDecl->getTypeSourceInfo()->getTypeLoc().getLocStart().getLocWithOffset(4));

        Owner.addTracedReplacement(SM, typeRange, printToString(parentVarDecl->getType().getCanonicalType()));
    }

    CharSourceRange initializerCharRange = CharSourceRange::getCharRange(parentVarDecl->getAnyInitializer()->getLocStart(), getTokenEnd(SM, parentVarDecl->getAnyInitializer()->getLocEnd()));

    CharSourceRange begin = CharSourceRange::getCharRange(parentVarDecl->getLocation(), parentVarDecl->getAnyInitializer()->getLocStart());

    std::string code = getSourceCode(SM, begin);

    llvm::Regex r = llvm::Regex("([^\\=]\\s*)=");

    code = r.sub("\\1", code);

    Owner.addTracedReplacement(SM, begin, code + " /*move initialize*/(( ");

    Owner.addTracedReplacement(SM, makeInsertionPoint(initializerCharRange.getEnd()), " ))/*move initialize end*/ ");

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

//         _       _                                    
//    ___ | |_  __| | _  _  _ __ ___    ___ __   __ ___ 
//   / __|| __|/ _` |(_)(_)| '_ ` _ \  / _ \\ \ / // _ \
//   \__ \| |_| (_| | _  _ | | | | | || (_) |\ V /|  __/
//   |___/ \__|\__,_|(_)(_)|_| |_| |_| \___/  \_/  \___|
//                                                      

/*Replace std::move calls.*/
template<>
void StdMoveReplacer::run(const MatchFinder::MatchResult &Result) {
    const CallExpr *D{ Result.Nodes.getNodeAs<CallExpr>(StdMoveId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    auto callNameRange = CharSourceRange::getCharRange(D->getLocStart(), D->getArg(0)->getLocStart().getLocWithOffset(-1));
    std::string call = getSourceCode(SM, callNameRange);
    
    
    if (implementationKind == implementation::backport_impl) {
        auto parent = getFirstContainingDynTypedParent_which(D, *Result.Context, [](ast_type_traits::DynTypedNode const &c) {return c.get<ImplicitCastExpr>() == nullptr; });

        if (parent.template get<CXXConstructExpr>() != nullptr) {
            // construction from already existing object with calling std::move().
            
            bool remove = true;
            if ((D->getArg(0)->getType()->isRecordType() && D->getArg(0)->getType()->isUnionType() == false &&
                D->getArg(0)->getType()->getAsCXXRecordDecl()->hasUserDeclaredMoveConstructor() &&
                Owner.isFileModifiable(SM, D->getArg(0)->getType()->getAsCXXRecordDecl()->getRBraceLoc()))
                || (D->getArg(0)->getType()->isDependentType() || D->getArg(0)->getType()->isInstantiationDependentType()) // RESTRICTION: IN TEMPLATES ONLY MOVE CLASS TYPED FIELDS. 
                ) {                                                                                                        // IN TURN PARTIAL SUPPORT FOR MOVE IN TEMPLATE CLASSES.
                remove = false;
            }

            llvm::Regex m("^\\s*((std::)|(::std::))?move");
            call = m.sub(remove? "" : getMoveMacro(std_move), call);

            Owner.addTracedReplacement(SM, callNameRange, "/*construction with move-enabled: " + std::to_string(!remove) + " */" + call);



        }
        else if (parent.template get<CXXOperatorCallExpr>() != nullptr) {
            // in operator, this should work if we have modified the operator.
            bool remove = true;

            if (D->getArg(0)->getType()->isRecordType() && D->getArg(0)->getType()->isUnionType() == false &&
                Owner.isFileModifiable(SM, D->getArg(0)->getType()->getAsCXXRecordDecl()->getRBraceLoc())) {
                remove = false;
            }

            llvm::Regex m("^\\s*((std::)|(::std::))?move");
            call = m.sub(remove? "" : getMoveMacro(std_move), call);

            Owner.addTracedReplacement(SM, callNameRange, "/*op call with move-enabled: " + std::to_string(!remove) + " */" + call);
        }
        else {

            bool remove = false;

            llvm::Regex m("^\\s*((std::)|(::std::))?move");
            call = m.sub(remove ? "" : getMoveMacro(std_move), call);

            Owner.addTracedReplacement(SM, callNameRange, "/*function call? with move-enabled: " + std::to_string(!remove) + " */" +  call);
        }
    }
    else {
        llvm::Regex m("^\\s*((std::)|(::std::))?move");
        call = m.sub(getMoveMacro(std_move), call);

        Owner.addTracedReplacement(SM, callNameRange, call);
    }

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

//    ___                    _  _        _  _                                 
//   |_ _| _ __ ___   _ __  | |(_)  ___ (_)| |_   _ __ ___    ___ __   __ ___ 
//    | | | '_ ` _ \ | '_ \ | || | / __|| || __| | '_ ` _ \  / _ \\ \ / // _ \
//    | | | | | | | || |_) || || || (__ | || |_  | | | | | || (_) |\ V /|  __/
//   |___||_| |_| |_|| .__/ |_||_| \___||_| \__| |_| |_| |_| \___/  \_/  \___|
//                   |_|                                                      
#ifndef NO_IMPLICIT_MOVE
/*Handle implicit moves from function return statements.*/
template<>
void ReturnValueMacroInserter::run(const MatchFinder::MatchResult &Result) {
    const ReturnStmt *D{ Result.Nodes.getDeclAs<ReturnStmt>(insertReturnValueMacro) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocEnd())) {
        return;
    }

    FunctionDecl const *parentFunction = getFirstContainingParent_which_as<FunctionDecl>(D, *Result.Context,
        [](clang::ast_type_traits::DynTypedNode const &item)
    { return item.template get<FunctionDecl>() != nullptr; });

    if (parentFunction == nullptr || (parentFunction->getReturnType().getTypePtr()->isLValueReferenceType() ||
        parentFunction->getReturnType().getTypePtr()->isRValueReferenceType())) {
        return;
    }

    /*If the type is in an external header (mostly this includes STL stuff) we don't try to move.*/
    if (parentFunction->getReturnType()->getAsCXXRecordDecl()->getSourceRange().isInvalid() || 
        !Owner.isFileModifiable(SM, parentFunction->getReturnType()->getAsCXXRecordDecl()->getSourceRange().getEnd()))
        return;

    CharSourceRange range = CharSourceRange::getCharRange(D->getSourceRange().getBegin(), getTokenEnd(SM, D->getRetValue()->getSourceRange().getEnd()));

    std::string source = getSourceCode(SM, range);

    llvm::Regex r = llvm::Regex("^(\\s*return )");

    source = r.sub(std::string("\\1") + " /*implicit move transform begin*/" + getMoveMacro(std_move) + " ( ", source);

    r = llvm::Regex("$");

    source = r.sub(" )/*implicit move transform end*/", source);

    Owner.addTracedReplacement(SM, range, source);

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

/*Move from temporaries whenever possible.*/
template<>
void TemporaryMoveMacroInserter::run(const MatchFinder::MatchResult &Result) {
    const CXXTemporaryObjectExpr *D{ Result.Nodes.getDeclAs<CXXTemporaryObjectExpr>(moveTemporaryWhenICan) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocEnd()) || 
        !Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    if (implementationKind != implementation::backport_impl)
        return;

    /*it cannot be moved. It maybe a class but it is in a file that we cannot access so...*/
    if (D->getType()->isRecordType() == false || D->getType()->getAsCXXRecordDecl() == nullptr ||
        D->getType()->getAsCXXRecordDecl()->getSourceRange().isInvalid() ||
        Owner.isFileModifiable(SM, D->getType()->getAsCXXRecordDecl()->getRBraceLoc()) == false)
    {
        //Owner.addTracedReplacement(SM, makeInsertionPoint(D->getLocEnd()), " /*it cannot be moved. It maybe a class but it is in a file that we cannot access so...*/ ");
        return;
    }
    
    /*it cannot be moved. It maybe a class but it doesn't have user defined move constuctor.*/
    if (D->getType()->getAsCXXRecordDecl()->hasUserDeclaredMoveConstructor() == false)
    {
        //Owner.addTracedReplacement(SM, makeInsertionPoint(D->getLocEnd()), " /*it cannot be moved. It maybe a class but it doesn't have user defined move constuctor.*/ ");
        return;
    }

    auto bindExprConstrExprOrTU = getFirstContainingDynTypedParent_which(D, *Result.Context, [](ast_type_traits::DynTypedNode const &c){
        return c.get<TranslationUnitDecl>() != nullptr ||
            c.get<CXXBindTemporaryExpr>() != nullptr ||
            c.get<CXXConstructExpr>() != nullptr;
    });

    /*Don't try to move into variadic function parameter.*/
    if (bindExprConstrExprOrTU.get<CXXBindTemporaryExpr>() != nullptr || bindExprConstrExprOrTU.get<CXXConstructExpr>() != nullptr) {
        auto parent = getFirstContainingDynTypedParent_which(bindExprConstrExprOrTU.get<Expr>(), *Result.Context,
            [](ast_type_traits::DynTypedNode const &) { return true; });

        if (parent.get<CallExpr>())
        {
            auto called_function = parent.get<CallExpr>()->getDirectCallee();

            assert(called_function);

            if (isVariadicFunction(SM, called_function, parent.get<CallExpr>()) ||
                (getFunctionTemplate(called_function, called_function->getASTContext()) && 
                isVariadicFunction(SM, getFunctionTemplate(called_function, called_function->getASTContext())->getAsFunction(), parent.get<CallExpr>())))
                return;
        }
    }

    CharSourceRange range = CharSourceRange::getCharRange(D->getSourceRange().getBegin(), getTokenEnd(SM, D->getSourceRange().getEnd()));

    std::string source = getSourceCode(SM, range);

    if (implementationKind == implementation::backport_impl) {

        llvm::Regex r = llvm::Regex("^");

        source = r.sub("/*implicit move from temporary transform begin*/(*reinterpret_cast< ::backport::rv < " + getNameWithPossiblyTemplateParameters(D, Owner, SM) + " > *>(" +
            "(&const_cast<char &>(reinterpret_cast<const volatile char &>( static_cast< " + getNameWithPossiblyTemplateParameters(D, Owner, SM) + " const & >(", source);

        r = llvm::Regex("$");

        source = r.sub(" /*implicit move from temporary transform end*/))))))", source);
    }
    else {

        llvm::Regex r = llvm::Regex("^");

        source = r.sub("/*implicit move from temporary transform begin*/const_cast< ::backport::rv< " +
            getNameWithPossiblyTemplateParameters(D, Owner, SM) + " > &>(static_cast< ::backport::rv< " +
            getNameWithPossiblyTemplateParameters(D, Owner, SM) + " > const &>" + " ( ", source);

        r = llvm::Regex("$");

        source = r.sub(" /*implicit move from temporary transform end*/))", source);
    }

    Owner.addTracedReplacement(SM, range, source);

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}
#endif

//     ____             _   
//    / ___| __ _  ___ | |_ 
//   | |    / _` |/ __|| __|
//   | |___| (_| |\__ \| |_ 
//    \____|\__,_||___/ \__|
//                          

/*Replace explicit casts to rvalue.*/
template<>
void RvalueCastReplacer::run(const MatchFinder::MatchResult &Result) {
    const ExplicitCastExpr *D{ Result.Nodes.getDeclAs<ExplicitCastExpr>(rvalue_cast) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getLocEnd())) {
        return;
    }

    bool removeCast = false;

    auto parent = getFirstContainingDynTypedParent_which(D, *Result.Context, [](ast_type_traits::DynTypedNode const & c) {
        return c.get<TranslationUnitDecl>() != nullptr ||
            c.get<VarDecl>() != nullptr ||
            c.get<CallExpr>() != nullptr ||
            c.get<CXXConstructExpr>() != nullptr ||
            c.get<CXXOperatorCallExpr>() != nullptr ||
            c.get<CXXCtorInitializer>() != nullptr;
    });

    /*Trying to cast to rvalue the members of the STL (like std::string to std::string &&) will be just ignored.
     * this looks at object construction. 
     */
    if (parent.get<CXXConstructExpr>() != nullptr ||
        parent.get<CXXCtorInitializer>() != nullptr)
    {
        if ((D->getType()->isRecordType() && D->getType()->isUnionType() == false &&
            D->getType()->getAsCXXRecordDecl()->hasUserDeclaredMoveConstructor() &&
            Owner.isFileModifiable(SM, D->getType()->getAsCXXRecordDecl()->getRBraceLoc())) == false) {

            removeCast = true;
        }
    }

    /*Same as above except it looks at variableName = static_cast<variableType &&>(other) calls.*/
    if (parent.get<CXXOperatorCallExpr>() != nullptr)
    {
        if ((D->getType()->isRecordType() && D->getType()->isUnionType() == false &&
            D->getType()->getAsCXXRecordDecl()->hasUserDeclaredMoveAssignment() &&
            Owner.isFileModifiable(SM, D->getType()->getAsCXXRecordDecl()->getRBraceLoc())) == false) {

            removeCast = true;
        }
    }

    if (dyn_cast<CStyleCastExpr>(D) != nullptr) {
        CStyleCastExpr const *CCast = dyn_cast<CStyleCastExpr>(D);

        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(CCast->getLParenLoc(),
            CCast->getRParenLoc().getLocWithOffset(1)), " /*c-style cast transformed */ " +
            (removeCast? "" : getMoveMacro(move_keywords::std_move)) +
            " ( ");

        Owner.addTracedReplacement(SM, makeInsertionPoint(getTokenEnd(SM, CCast->getLocEnd())), " ) ");

        SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

        ++AcceptedChanges;
    }
    else if (dyn_cast<CXXFunctionalCastExpr>(D) != nullptr) {
        CXXFunctionalCastExpr const *CXXCast = dyn_cast<CXXFunctionalCastExpr>(D);

        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(CXXCast->getLocStart(),
            CXXCast->getLParenLoc().getLocWithOffset(-1)), "/*functional cast transformed*/" +
            (removeCast ? "" : getMoveMacro(move_keywords::std_move)));

        SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

        ++AcceptedChanges;
    }
    else if (dyn_cast<CXXNamedCastExpr>(D) != nullptr) {
        CXXNamedCastExpr const *CXXNamedC = dyn_cast <CXXNamedCastExpr>(D);

        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(CXXNamedC->getLocStart(),
            CXXNamedC->getAngleBrackets().getEnd().getLocWithOffset(1)), "/*named cast transformed*/" +
            (removeCast ? "" : getMoveMacro(move_keywords::std_move)));

        SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

        ++AcceptedChanges;
    }
  

}

/*Magic*/
template<>
void XvalueImplicitCastReplacer::run(const MatchFinder::MatchResult &Result) {
    const ImplicitCastExpr *D{ Result.Nodes.getDeclAs<ImplicitCastExpr>(implicitCastXvalue) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (D->getSourceRange().isInvalid() || !Owner.isFileModifiable(SM, D->getSourceRange().getBegin())) {
        return;
    }

    if (D->getSubExpr()->isXValue() == false) {
        return;
    }

    CallExpr const * moveCall = nullptr;

    for (auto c : const_cast<ImplicitCastExpr *>(D)->children()) {
        if (isMoveStatement(c)) {
            moveCall = static_cast<CallExpr const *>(c);
            break;
        }
    }

    std::string code = getSourceCode(SM, D->getSourceRange());

    auto beginRange = CharSourceRange::getCharRange(D->getLocStart().getLocWithOffset(-1), D->getLocStart());

    /*Fuuuuuu*/
    code = getSourceCode(SM, beginRange) + code;
    char beginChar = code[0];

    if (moveCall != nullptr) {
        beginRange = CharSourceRange::getCharRange(beginRange.getBegin(), moveCall->getArg(0)->getLocStart().getLocWithOffset(-1));
    }

    bool isMoveable = false;

    if (D->getType()->isRecordType() && D->getType()->isUnionType() == false) {
        auto k = D->getType()->getAsCXXRecordDecl();
        if (k->hasUserDeclaredMoveOperation() && Owner.isFileModifiable(SM, k->getRBraceLoc())) {
            isMoveable = true;
        }
    }
    else {
        if (D->getType()->isRecordType() == false || D->getType()->isUnionType() == true) {
            isMoveable = true;
        }
    }

    if (D->isXValue() == true && isMoveable) {

        if (implementationKind == implementation::backport_impl) {
            std::string const h = "static_cast<" + getNameWithPossiblyTemplateParameters(D->getSubExpr(), Owner, SM) + " const &>";
            Owner.addTracedReplacement(SM, beginRange, std::string(1, beginChar) +
                "/*implicit xvalue cast transform begin*/(*reinterpret_cast< ::backport::rv< " + getNameWithPossiblyTemplateParameters(D, Owner, SM) +
                " > *>(&const_cast<char &>(reinterpret_cast<const volatile char &>( static_cast< " + getNameWithPossiblyTemplateParameters(D, Owner, SM) + 
                " & >(*reinterpret_cast< " + getNameWithPossiblyTemplateParameters(D->getSubExpr(), Owner, SM) + " *>(" +
                "(&const_cast<char &>(reinterpret_cast<const volatile char &>( " + (/*moveCall == nullptr*/false ? h : "") + "(");

            Owner.addTracedReplacement(SM, makeInsertionPoint(D->getLocEnd()), " /*implicit xvalue cast transform end*/))))))))))");
        }
        else {
            Owner.addTracedReplacement(SM, beginRange, std::string(1, beginChar) + std::string("static_cast< ") + getNameWithPossiblyTemplateParameters(D, Owner, SM) + " &>( " + getMoveMacro(std_move));
            Owner.addTracedReplacement(SM, makeInsertionPoint(D->getLocEnd()), " ) ");
        }
    }
    else {
            Owner.addTracedReplacement(SM, beginRange, std::string(1, beginChar) +
                "/*implicit xvalue cast transform (to lvalue) begin*/( static_cast< " + getNameWithPossiblyTemplateParameters(D, Owner, SM) +
                " & >(*reinterpret_cast< " + getNameWithPossiblyTemplateParameters(D->getSubExpr(), Owner, SM) + " * >(" +
                "(&const_cast<char &>(reinterpret_cast<const volatile char &>( ");

            Owner.addTracedReplacement(SM, makeInsertionPoint(D->getLocEnd()), " /*implicit xvalue cast transform end*/))))))");
    }

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

/*Insert move utilities.*/
template<>
void OneTimeInserterDecl::run(const MatchFinder::MatchResult &Result) {
    const Decl *D{ Result.Nodes.getNodeAs<Decl>(GlobalInsertDeclId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    SourceLocation sourceLoc = getFileSourceRange(D, SM).getBegin();
    Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(sourceLoc, sourceLoc), backport_rv(""));

    ++AcceptedChanges;
}

const char ReplaceRvalueID[] = "ReplaceRvalueRef";

struct ReplaceRvaluerefFactory : TransformFactory {
    ReplaceRvaluerefFactory() {
        Since.Clang = Version(2, 9);
        Since.Gcc = Version(4, 4);
        Since.Icc = Version(12);
        Since.Msvc = Version(10);
    }

    Transform *createTransform(const TransformOptions &Opts) override {
        TransformHandler *handler = new TransformHandler(ReplaceRvalueID, Opts, TransformPriorities::REPLACE_RVALUE);
        auto &acc = handler->getAcceptedChanges();
        handler->initReplacers(new RvaluerefReplacer(acc, *handler), makeRvalueMatcher,
            new StdMoveReplacer(acc, *handler), makeMoveStatementMatcher,
            new LvalueRefInCopyAssignReplacer(acc, *handler), makeLvalueInCopyAssignMatcher,

            new CopyableAndMovableMacroInserter(acc, *handler), makeCopyableAndMoveableMatcher,
            new NotCopyableAndMovableMacroInserter(acc, *handler), makeNotCopyableAndMoveableMatcher,
            new CopyableAndNotMovableMacroInserter(acc, *handler), makeCopyableAndNotMoveableMatcher,

#ifndef NO_IMPLICIT_MOVE
            new ReturnValueMacroInserter(acc, *handler), makeReturnValueMatcher,
            new TemporaryMoveMacroInserter(acc, *handler), makeTemporaryToXValueMatcher,
#endif
            new RvalueCastReplacer(acc, *handler), makeRvalueCastMatcher,

            new MoveInitializationReplacer(acc, *handler), makeMoveInitializationMatcher,

            new RvalueInVarDeclNotParamDeclReplacer(acc, *handler), makeRvalueVarDeclNotParamDeclMatcher,
            new XvalueImplicitCastReplacer(acc, *handler), makeimplicitCastXvalue,

            //new OneTimeInserterDecl(acc, *handler), makeDeclMatcher,

            new RvalueRefInTemplateTypeParmDeclReplacer(acc, *handler), makeRvalueRefInTemplateTypeParmDeclMacher,
            new ReplaceRvalueRefInTypedefReplacer(acc, *handler), makeRvalueRefInTypedefMatcher,

            new RvalueRefInDeclaratorDeclReplacer(acc, *handler), makeRvalueInDeclaratorMatcher,
            new RvalueRefInConstructExprReplacer(acc, *handler), makeRvalueInConstrExprMatcher,
            new RvalueRefInTemplateSpecArgReplacer(acc, *handler), makeRvalueInTemplateSpecializationParameterMatcher,
            new RvalueInDeclaratorDeclThroughDeclRefReplacer(acc, *handler), makeDeclRefToRvalueDeclaration
            );
        return handler;
    }
};

// Register the factory using this statically initialized variable.
static TransformFactoryRegistry::Add<ReplaceRvaluerefFactory>
X("replace-rvalue", "Replace c++11 rvalue with equivalent construction.");
