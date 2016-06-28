#include "TypeAliasMatchers.h"

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;

// typedef with type alias
using TypedefTypeAliasReplacer = ReplacerTemplate<class TypeDefTypeAliasReplace>;

// type alias converter
using TypeAliasDeclReplacer = ReplacerTemplate<class TypeAliasDeclReplace>;
using TypeAliasTemplateDeclConverter = ReplacerTemplate<class TypeAliasTemplateDeclConverte>;

// template type alias everywhere
using TypeAliasTemplateVarTypeReplacer = ReplacerTemplate<class TypeAliasTemplateVarTypeReplace>;
using TypeAliasTemplateReturnTypeReplacer = ReplacerTemplate<class TypeAliasTemplateReturnTypeReplace>;
using TypeAliasTemplateTemplateArgumentReplacer = ReplacerTemplate<class TypeAliasTemplateTemplateArgumentReplace>;
using TypeAliasTemplateExprReplacer = ReplacerTemplate<class TypeAliasTemplateExprReplace>;
using TypeAliasTemplateClassTemplateSpecializationReplacer = ReplacerTemplate<class TypeAliasTemplateTemplateSpecializationReplace>;
using TypeAliasTemplateFunctionTemplateSpecializationReplacer = ReplacerTemplate<class TypeAliasTemplateFunctionTemplateSpecializationReplace>;
using TypeAliasTemplateClassMethodNameReplacer = ReplacerTemplate<class TypeAliasTemplateClassMethodNameReplace>;

namespace {

    /**
     * \brief This variable contains the name of the typedefs.
     */
    const char* backport_type_alias_underlying_type = "backport_type_alias_underlying_type";

    /**
     * \brief This function searches for nested template type aliases in template arguments and collects them for later use.
     */
    void searchTypeAliasTemplateTypes(const TemplateSpecializationType *TST, std::vector<std::pair<std::string, bool>>& typeAliasVector) {
        for (auto it = TST->begin(); it != TST->end(); ++it) {
            if (it->getKind() != TemplateArgument::ArgKind::Type)
                continue;

            auto qualType = it->getAsType();
            if (qualType.isNull())
                continue;

            while (!qualType->getPointeeType().isNull() && qualType->getTypeClass() != Type::TemplateSpecialization && qualType->getTypeClass() != Type::Elaborated)
                qualType = qualType->getPointeeType();

            const TemplateSpecializationType *tplType = castToTemplateSpecializationType(qualType);
            if (tplType != nullptr) {
                if (tplType->isTypeAlias()) {
                    typeAliasVector.push_back(std::make_pair(qualType.getAsString(getPrintingPolicy()), tplType->isInstantiationDependentType()));
                }
                searchTypeAliasTemplateTypes(tplType, typeAliasVector);
            }
        }
    }

    /**
     * \brief This function replaces template type aliases in types.
     */
    void replaceTypeAliasTemplateType(const QualType& type, std::string &replacement, bool skipTypeNameInMainType = false) {
        auto oldExpr = replacement;
        auto baseType = type;
        if (baseType.isNull()) {
            LOG(logERROR) << "This should not happen! Invalid template type alias type!";
            return;
        }
        while (!baseType->getPointeeType().isNull() && baseType->getTypeClass() != Type::TemplateSpecialization && baseType->getTypeClass() != Type::Elaborated)
            baseType = baseType->getPointeeType();

        auto TST = castToTemplateSpecializationType(baseType);
        if (TST == nullptr) {
            replacement = "";
            return;
        }

        if (replacement.empty()) {
            replacement = type.getAsString(getPrintingPolicy());
        }

        if (TST->isTypeAlias()) {
            auto str = baseType.withoutLocalFastQualifiers().getAsString(getPrintingPolicy());
            auto dstFormat = (TST->isInstantiationDependentType() && !skipTypeNameInMainType ? "typename " : "") + str + "::" + backport_type_alias_underlying_type;
            ReplaceAll(replacement, str, dstFormat);
        }

        // pair is qualtype as string and isInstantiationDependentType
        std::vector<std::pair<std::string, bool> > typeAliasVector;
        searchTypeAliasTemplateTypes(TST, typeAliasVector);
        for (auto& elem : typeAliasVector) {
            std::string str = llvm::Regex::escape(elem.first);
            std::string dstFormat = (elem.second ? "typename " : "") + str + "::" + backport_type_alias_underlying_type + "\\2";
            RegexReplaceAll("(" + str + "([^(::" + backport_type_alias_underlying_type + ")]))", replacement, dstFormat);
        }

        if ((!TST->isTypeAlias() && typeAliasVector.empty()) || oldExpr == replacement) {
            replacement = "";
        }
    }

    /**
    * \brief Recreates function types with replaced type aliases in return types and function parameters.
    */
    void correctFunctionProtoType(QualType const& type, std::string name, std::string &replacement) {
        auto baseType = type;
        // remove pointers and references types
        while (!baseType->getPointeeType().isNull()) {
            if (baseType->isReferenceType()) {
                if (baseType->isRValueReferenceType()) {
                    name = "&&" + name;
                }
                else {
                    name = "&" + name;
                }
            }
            else {
                name = "*" + name;
            }
            baseType = baseType->getPointeeType();
        }
        if (!name.empty()) {
            name = "(" + name + ")";
        }
        auto func = baseType->getAs<FunctionProtoType>();
        if (!func)
            return;
        std::string retType;
        std::string params;
        replaceTypeAliasTemplateType(func->getReturnType(), retType);
        if (retType.empty()) {
            retType = printToString(func->getReturnType());
        }

        for (auto &parmType : func->getParamTypes()) {
            std::string type;
            if (!params.empty()) {
                params += ", ";
            }
            replaceTypeAliasTemplateType(parmType, type);
            if (type.empty()) {
                correctFunctionProtoType(parmType, "", type);
                if (type.empty()) {
                    type = printToString(parmType);
                }
            }
            params += type;
        }
        replacement += retType + " " + name + "(" + params + ")";
    }

    /**
    * \brief Replaces type aliases in types. Returns true if type aliases is in a function type
    */
    bool replaceTypeAliasInType(QualType const& type, std::string &replacement, bool skipTypeNameInMainType = false, std::string name = "") {
        bool isFunctionPtr = false;
        replaceTypeAliasTemplateType(type, replacement, skipTypeNameInMainType);
        if (replacement.empty()) {
            correctFunctionProtoType(type, name, replacement);
            isFunctionPtr = true;
        }
        return isFunctionPtr;
    }

    /**
    * \brief Creates a typedef from a type alias type.
    */
    std::string createTypedef(QualType const&ULType, std::string type, std::string TADName) {
        std::string replacement = "typedef ";
        if (hasSubType(ULType, [](const QualType &type) { return type->isFunctionType(); })) {
            // creates function prototype with replaced type aliases and original pointer types.
            correctFunctionProtoType(ULType, TADName, replacement);
        }
        else if (ULType->isArrayType()) {
            replacement += llvm::Regex("\\[").sub(TADName + "[", type);
        }
        else {
            replacement += type + " " + TADName;
        }
        return replacement;
    }
}


/**
* \brief This function replaces template type alias types in typedef declarations.
*/
template<>
void TypedefTypeAliasReplacer::run(const MatchFinder::MatchResult &Result) {
    const TypedefDecl* TDD = Result.Nodes.getDeclAs<TypedefDecl>(TypedefDeclId);
    assert(TDD && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, TDD->getLocStart())) {
        return;
    }

    std::string replacement;
    replaceTypeAliasTemplateType(TDD->getUnderlyingType(), replacement);
    if (replacement.empty()) {
        replacement = TDD->getUnderlyingType().getAsString(getPrintingPolicy());
        replacement = createTypedef(TDD->getUnderlyingType(), replacement, TDD->getNameAsString());
    }
    else {
        replacement = "typedef " + replacement + " " + TDD->getNameAsString();
    }

    Owner.addTracedReplacement(SM, getFullRange(SM, TDD->getSourceRange()), replacement);
    ++AcceptedChanges;
}

/**
 * \brief This function replaces type alias declarations with equivalent typedef declarations.
 */
template<>
void TypeAliasDeclReplacer::run(const MatchFinder::MatchResult &Result) {
    const TypeAliasDecl* TAD = Result.Nodes.getDeclAs<TypeAliasDecl>(TypeAliasDeclId);
    assert(TAD && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, TAD->getLocStart())) {
        return;
    }

    auto ULType = TAD->getUnderlyingType();
    std::string type;
    SourceLocation endLoc;
    if (ULType->getTypeClass() == Type::Decltype) {
        endLoc = TAD->getLocEnd().getLocWithOffset(getLengthFromLocationUntilSeparator(SM, TAD->getLocEnd()));
        type = ULType.getCanonicalType().getAsString(getPrintingPolicy());
    }
    else {
        // Local types needs special treatment. like using Anon = class { int a; };
        if (ULType->hasUnnamedOrLocalType()) {
            CharSourceRange rng;
            auto tagDecl = ULType->getAsTagDecl();
            if (tagDecl == nullptr && TAD->getTypeSourceInfo()->getTypeLoc().getBeginLoc() != TAD->getTypeSourceInfo()->getTypeLoc().getEndLoc()) {
                rng = getFullRange(SM, TAD->getTypeSourceInfo()->getTypeLoc().getSourceRange());
            }
            else {
                rng = getFullRange(SM, tagDecl->getSourceRange());
            }
            type = getSourceCode(SM, rng);
            if (rng.getEnd() < TAD->getLocStart()) {
                endLoc = getTokenEnd(SM, TAD->getLocEnd());
            }
            else {
                endLoc = rng.getEnd();
            }
        }
        else {
            endLoc = TAD->getLocEnd().getLocWithOffset(getLengthFromLocationUntilSeparator(SM, TAD->getLocEnd()));
            type = ULType.getAsString(getPrintingPolicy());
        }
    }

    auto tmpType = type;
    replaceTypeAliasTemplateType(ULType, tmpType);
    if (!tmpType.empty()) {
        type = tmpType;
    }

    auto replacement = createTypedef(ULType, type, TAD->getNameAsString());

    auto replaceRng = CharSourceRange::getCharRange(TAD->getLocStart(), endLoc);
    Owner.addTracedReplacement(SM, replaceRng, replacement);
    ++AcceptedChanges;
}

/**
* \brief This function replaces template type alias declarations with equivalent templated structs with typedefs.
*/
template<>
void TypeAliasTemplateDeclConverter::run(const MatchFinder::MatchResult &Result) {
    const TypeAliasTemplateDecl* TATD = Result.Nodes.getDeclAs<TypeAliasTemplateDecl>(TypeAliasTemplateDeclId);
    assert(TATD && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, TATD->getLocStart())) {
        return;
    }

    std::string tplParams;
    for (auto param : TATD->getTemplateParameters()->asArray()) {
        auto typeParam = dyn_cast<TemplateTypeParmDecl>(param);
        auto nonTypeParam = dyn_cast<NonTypeTemplateParmDecl>(param);
        if (!tplParams.empty())
            tplParams += ", ";

        if (typeParam != nullptr) {
            tplParams += getSourceCode(SM, getFullRange(SM, typeParam->getSourceRange()));
        }
        else if (nonTypeParam != nullptr) {
            tplParams += getSourceCode(SM, getFullRange(SM, nonTypeParam->getSourceRange()));
        }
    }

    std::string type;
    replaceTypeAliasTemplateType(TATD->getTemplatedDecl()->getUnderlyingType(), type);
    if (type.empty()) {
        type = TATD->getTemplatedDecl()->getUnderlyingType().getAsString(getPrintingPolicy());
    }
    
    auto typedefStr = createTypedef(TATD->getTemplatedDecl()->getUnderlyingType(), type, backport_type_alias_underlying_type);
    std::string replacement = "template <" + tplParams + "> struct " + TATD->getNameAsString() + " { " + typedefStr + "; }";

    auto rng = CharSourceRange::getTokenRange(TATD->getSourceRange());
    Owner.addTracedReplacement(SM, rng, replacement);
    ++AcceptedChanges;
}

/**
 * \brief This function replaces variable declarations with types from type template aliases.
 */
template<>
void TypeAliasTemplateVarTypeReplacer::run(const MatchFinder::MatchResult &Result) {
    const ValueDecl* Val = Result.Nodes.getNodeAs<ValueDecl>(AliasedTypeVariableDeclId);
    assert(Val && "Bad Callback. No node provided");
    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, Val->getLocStart())) {
        return;
    }

    std::string replacement;
    bool functionPtr = replaceTypeAliasInType(Val->getType(), replacement, false, Val->getNameAsString());
    if (replacement.empty()) {
        LOG(logWARNING) << "Type alias: Empty replacement in type " << printToString(Val->getType());
        return;
    }

    if (auto varDecl = dyn_cast<VarDecl>(Val)) {
        replacement = getStorageClassAsString(varDecl) + replacement;
    }

    replacement += " ";
    auto rng = CharSourceRange::getCharRange(Val->getLocStart(), functionPtr ? getTokenEnd(SM, Val->getLocEnd()) : Val->getLocation());
    Owner.addTracedReplacement(SM, rng, replacement);
    ++AcceptedChanges;
}

/**
 * \brief This function replaces template type aliases at function return types.
 */
template<>
void TypeAliasTemplateReturnTypeReplacer::run(const MatchFinder::MatchResult &Result) {
    const FunctionDecl* FD = Result.Nodes.getDeclAs<FunctionDecl>(AliasedFunctionReturnTypeDeclId);
    assert(FD && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, FD->getLocStart())) {
        return;
    }

    std::string replacement;
    replaceTypeAliasInType(FD->getReturnType(), replacement);
    if (replacement.empty()) {
        LOG(logWARNING) << "Type alias: Empty replacement in type " << printToString(FD->getReturnType());
        return;
    }

    if (FD->getStorageClass() == StorageClass::SC_Static) {
        replacement = "static " + replacement;
    }
    if (FD->isVirtualAsWritten()) {
        replacement = "virtual " + replacement;
    }

    replacement += " ";
    auto rng = CharSourceRange::getCharRange(FD->getInnerLocStart(), FD->getLocation());
    Owner.addTracedReplacement(SM, rng, replacement);
    ++AcceptedChanges;
}

/**
 * \brief This function replaces template type aliases at default template argument declarations.
 */
template<>
void TypeAliasTemplateTemplateArgumentReplacer::run(const MatchFinder::MatchResult &Result) {
    const TemplateTypeParmDecl* TTPD = Result.Nodes.getDeclAs<TemplateTypeParmDecl>(AliasedTypeVariableDeclId);
    assert(TTPD && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, TTPD->getLocStart())) {
        return;
    }

    std::string replacement;
    replaceTypeAliasInType(TTPD->getDefaultArgument(), replacement);
    if (replacement.empty()) {
        LOG(logWARNING) << "Type alias: Empty replacement in type " << printToString(TTPD->getDefaultArgument());
        return;
    }

    auto rng = CharSourceRange::getCharRange(TTPD->getDefaultArgumentLoc(), getTokenEnd(SM, TTPD->getLocEnd()));
    Owner.addTracedReplacement(SM, rng, replacement);
    ++AcceptedChanges;
}


/**
* \brief This function replaces template type aliases in expressions.
*/
template<>
void TypeAliasTemplateExprReplacer::run(const MatchFinder::MatchResult &Result) {
    const Expr* expr = Result.Nodes.getNodeAs<Expr>(ContainedExprId);
    assert(expr && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, expr->getLocStart())) {
        return;
    }

    std::string replacement = printToString(expr);

    // in some cases printpretty creates ((   )) like braces in parameter lists
    // we eliminate these with this regex
    RegexReplaceAll("\\(\\( (.*) \\)\\)", replacement, "\\(\\1\\)");
    // in case of sizeof we have to cast to UnaryExprOrTypeTraitExpr
    QualType type;
    if (auto unaryexpr = dyn_cast<UnaryExprOrTypeTraitExpr>(expr)) {
        type = unaryexpr->getTypeOfArgument();
    }
    else if (auto explCast = dyn_cast<ExplicitCastExpr>(expr)) {
        type = explCast->getTypeAsWritten();
    }
    else if (auto unresolvedConstruct = dyn_cast<CXXUnresolvedConstructExpr>(expr)) {
        type = unresolvedConstruct->getTypeAsWritten();
    }
    else {
        type = expr->getType();
    }

    replaceTypeAliasInType(type, replacement);
    if (replacement.empty()) {
        LOG(logWARNING) << "Type alias: Empty replacement in type " << printToString(type);
        return;
    }

    Owner.addTracedReplacement(SM, getFullRange(SM, expr->getSourceRange()), replacement);
    ++AcceptedChanges;
}

/**
* \brief This function replaces template type aliases in template class specializations.
*/
template<>
void TypeAliasTemplateClassTemplateSpecializationReplacer::run(const MatchFinder::MatchResult &Result) {
    const ClassTemplateSpecializationDecl* CTSD = Result.Nodes.getDeclAs<ClassTemplateSpecializationDecl>(ClassTemplateSpecializationDeclId);
    assert(CTSD && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, CTSD->getLocStart())) {
        return;
    }

    std::string replacement;
    replaceTypeAliasInType(CTSD->getTypeAsWritten()->getType(), replacement);
    if (replacement.empty()) {
        LOG(logWARNING) << "Type alias: Empty replacement in type " << printToString(CTSD->getTypeAsWritten()->getType());
        return;
    }
    Owner.addTracedReplacement(SM, getFullRange(SM, CTSD->getTypeAsWritten()->getTypeLoc().getSourceRange()), replacement);
    ++AcceptedChanges;
}

/**
* \brief This function replaces template type aliases in function template specializations.
*/
template<>
void TypeAliasTemplateFunctionTemplateSpecializationReplacer::run(const MatchFinder::MatchResult &Result) {
    const FunctionDecl* FD = Result.Nodes.getDeclAs<FunctionDecl>(FunctionTemplateSpecializationDeclId);
    assert(FD && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, FD->getLocStart())) {
        return;
    }

    for (unsigned int i = 0; i < FD->getTemplateSpecializationArgsAsWritten()->NumTemplateArgs; ++i) {
        auto &ASTTemplateSpecArgs = (*FD->getTemplateSpecializationArgsAsWritten());
        std::string replacement;
        replaceTypeAliasInType(ASTTemplateSpecArgs[i].getTypeSourceInfo()->getType(), replacement);
        if (replacement.empty()) {
            LOG(logWARNING) << "Type alias: Empty replacement in type " << printToString(ASTTemplateSpecArgs[i].getTypeSourceInfo()->getType());
            continue;
        }
        Owner.addTracedReplacement(SM, getFullRange(SM, ASTTemplateSpecArgs[i].getTypeSourceInfo()->getTypeLoc().getSourceRange()), replacement);
        ++AcceptedChanges;
    }
}

/**
* \brief This function replaces template type aliases in type qualifiers example: void classAlias<T>::SampleClass()...
*/
template <>
void TypeAliasTemplateClassMethodNameReplacer::run(const MatchFinder::MatchResult &Result) {
    const CXXMethodDecl* method = Result.Nodes.getDeclAs<CXXMethodDecl>(AliasedTypeScopeResolveNameId);
    assert(method && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, method->getLocStart())) {
        return;
    }

    std::string replacement = printToString(method->getQualifierLoc().getTypeLoc().getType().getCanonicalType());
    if (replacement.empty()) {
        LOG(logWARNING) << "Type alias: Empty replacement in type " << printToString(method->getQualifierLoc().getTypeLoc().getType());
        return;
    }

    CharSourceRange rng;
    // it's weird but in explicit template specializations desugared type appends full qualifier so need to change the range of the replacement.
    if (method->getTemplateSpecializationKind() == TemplateSpecializationKind::TSK_ExplicitSpecialization) {
        rng = CharSourceRange::getCharRange(method->getQualifierLoc().getSourceRange());
    }
    else {
        rng = getFullRange(SM, method->getQualifierLoc().getTypeLoc().getSourceRange());
    }

    Owner.addTracedReplacement(SM, rng, replacement);
    ++AcceptedChanges;
}
