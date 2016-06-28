#include "TransformUtility.h"
#include "PathUtility.h"
#include "StringUtility.h"
#include "PrintingPolicySingleton.h"
#include "clang/Lex/Lexer.h"
#include "Log.h"

#include <algorithm>
#include <iterator>
#include <set>
#include "Util/ReplacementData.h"
#include <memory>
#include <utility>

namespace backport { namespace helper {

    /**
    * \brief Returns the given function pointer variable with the given name.
    */
    std::string rebuildFunctionPointerType(QualType const& type, std::string name) {
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
        if (!func) {
            LOG(logERROR) << "Assuming Function pointer but no FunctionProtoType found.";
            abort();
        }

        std::string retType = printToString(func->getReturnType());
        std::string params;
        for (auto &parmType : func->getParamTypes()) {
            if (!params.empty()) {
                params += ", ";
            }
            params += printToString(parmType);
        }

        return retType + " " + name + "(" + params + ")";
    }

    clang::SourceRange getFileSourceRange(clang::SourceLocation const &loc, clang::SourceManager const & SM) {
        auto FID = SM.getFileID(loc);
        if (FID.isInvalid()) {
            LOG(logERROR) << "SourceRange requested for invalid file";
            exit(1);
        }
        auto fileStart = SM.getLocForStartOfFile(FID);
        auto fileEnd = SM.getLocForEndOfFile(FID);
        clang::SourceRange fileRange(fileStart, fileEnd);
        return fileRange;
    }

    clang::SourceLocation getTokenEnd(clang::SourceManager & sm, const clang::SourceLocation& loc) {
        clang::LangOptions lopt;

        assert(loc.isValid());

        auto const result = clang::Lexer::getLocForEndOfToken(loc, 0, sm, lopt);
        if (result.isInvalid() || backport::helper::less(sm, result, loc)) {
            return loc;
        } else {

            return result;
        }
    }

    clang::CharSourceRange makeInsertionPoint(const clang::SourceLocation& loc) {
        return clang::CharSourceRange::getCharRange(loc, loc);
    }

    clang::CharSourceRange getFullRange(clang::SourceManager& sm, const clang::SourceRange& sr) {
        return clang::CharSourceRange::getCharRange(sr.getBegin(), getTokenEnd(sm, sr.getEnd()));
    }

    const FunctionTemplateDecl* getFunctionTemplate(const FunctionDecl* FD, clang::ASTContext& ASTCon) {
        const FunctionTemplateDecl* primaryTemplate = FD->getPrimaryTemplate();
        const FunctionTemplateDecl* parentCast = (ASTCon.getParents(*FD)[0]).template get<FunctionTemplateDecl>();

        if (primaryTemplate != nullptr) {
            return primaryTemplate;
        }
        else {
            return parentCast;
        }
    }

    namespace {
        inline bool _removeConstexprNoexcept_helper(std::string &text, std::string const &keyword, unsigned int &currPos) {
            if (hasBeginning(text.substr(currPos), keyword)) {
                int const start = currPos;

                currPos += keyword.size();

                int depth = 0;

                /*Skip whitespaces*/
                while (currPos < text.size() && clang::isWhitespace(text[currPos])) {
                    currPos += 1;
                }

                if (currPos >= text.size() || text[currPos] != '(') {
                    auto s = text.begin();
                    auto e = s;

                    if (currPos >= text.size()) {
                        currPos = text.size() - 1;
                    }

                    std::advance(s, start);
                    std::advance(e, currPos);

                    text.erase(s, e);

                    text.insert(s, ' ');

                    currPos = start;
                    return true;
                }

                /*text[currPos] == '('*/
                currPos += 1;
                depth += 1;

                while (depth > 0) {
                    if (text[currPos] == '(') {
                        depth += 1;
                    }
                    else if (text[currPos] == ')') {
                        depth -= 1;
                    }

                    currPos += 1;

                    if (currPos >= text.size())
                    {
                        abort();
                    }
                }

                auto s = text.begin();
                auto e = s;

                std::advance(s, start);
                std::advance(e, currPos);

                text.erase(s, e);

                text.insert(s, ' ');

                currPos = start;
            }
            else {
                return false;
            }

            return true;
        }

        inline void _removeConstexprNoexcept_loop_helper(std::string &text, std::string const &keyword) {
            if (text.find(keyword) != std::string::npos) {
                unsigned int currPos = 0;
                assert(false && "This is no bug. This assert is here only to make a breakpoint in debug mode "
                       "because as far as I am concerned the printingpoliciy can be set to don't print things like noexcept/contexpr "
                       "which is a better solution than parsing and deleting those from the string manually. Still this function "
                       "should work, and do just that.");
                while (currPos < text.size()) {

                    if (_removeConstexprNoexcept_helper(text, keyword, currPos)) continue;

                    currPos += 1;

                }
            }
        }
    }

    std::string removeConstexprNoexcept(std::string text) {

        _removeConstexprNoexcept_loop_helper(text, "constexpr");

        _removeConstexprNoexcept_loop_helper(text, "noexcept");

        return text;
    }

    clang::SourceLocation functionDeclRangeEnd(const Decl* FD) {
        if ((FD->isFunctionOrFunctionTemplate()) && (FD->getAsFunction()->isThisDeclarationADefinition()) && (FD->getAsFunction() != nullptr) && (FD->getAsFunction()->getBody() != nullptr)) {
            return FD->getAsFunction()->getBody()->getSourceRange().getEnd().getLocWithOffset(1);
        }
        clang::SourceLocation range_end;
        auto ND = FD->getNextDeclInContext();
        while (ND != nullptr && ND->getLocation() < FD->getLocation()) {
            ND = ND->getNextDeclInContext();
        }

        if (ND != nullptr) {
            range_end = ND->getLocStart().getLocWithOffset(-1);
        }
        else {
            auto FDPC = FD->getAsFunction()->getLexicalParent();
            if (FDPC->isFunctionOrMethod()) {
                auto FDP = dyn_cast<clang::FunctionDecl>(FDPC);
                range_end = FDP->getLocEnd().getLocWithOffset(-1);
            }
            else if (FDPC->isNamespace()) {
                auto FDP = dyn_cast<clang::NamespaceDecl>(FDPC);
                auto n = FDP->getNameAsString();
                range_end = FDP->getLocEnd().getLocWithOffset(-1);
            }
            else if (FDPC->isRecord()) {
                auto FDP = dyn_cast<clang::CXXRecordDecl>(FDPC);
                range_end = FDP->getLocEnd().getLocWithOffset(-1);
            }
            else if (FDPC->isTranslationUnit()) {
                auto FDP = dyn_cast<clang::TranslationUnitDecl>(FDPC);
                //auto ANS = FDP->getAnonymousNamespace();
                //auto rng = ANS->getSourceRange();
                range_end = FDP->getLocEnd();
            }
        }
        return range_end;
    }

    clang::PrintingPolicy& getPrintingPolicy() {
        return PrintingPolicySingleton::instance();
    }

    std::string getType(const ValueDecl* var) {
        return var->getType().getAsString(getPrintingPolicy()) + " ";
    }

    std::string getSourceCode(const clang::SourceManager& SM, const clang::CharSourceRange& range) {
        auto begin = SM.getCharacterData(range.getBegin());
        auto end = SM.getCharacterData(range.getEnd());
        std::string text(begin, end);
        return text;
    }

    std::string getSourceCode(const clang::SourceManager& SM, const clang::SourceRange& range) {
        return getSourceCode(SM, clang::CharSourceRange::getCharRange(range));
    }

    int getLengthFromLocationUntilSeparator(clang::SourceManager const& SM, clang::SourceLocation const& loc, const char sep) {
        auto source = SM.getCharacterData(loc);
        int i = 0;
        for (; source[i] != sep; ++i);
        return i;
    }

    int getLengthFromLocationUntilClosingParentheses(clang::SourceManager const& SM, clang::SourceLocation const& loc, const char open, const char close) {
        auto source = SM.getCharacterData(loc);
        int i = 0;
        int p = 0;
        for (; source[i] != open; ++i);
        ++p;
        ++i;
        for (; p != 0; ++i) {
            if (source[i] == open) {
                ++p;
            } else if (source[i] == close) {
                --p;
            }
        }
        return i;
    }

    bool hasEnding(std::string const &fullString, std::string const &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        }
        else {
            return false;
        }
    }

    bool hasBeginning(std::string const &fullString, std::string const &beginning) {
        if (fullString.length() >= beginning.length()) {
            return (0 == fullString.compare(0, beginning.length(), beginning));
        }
        else {
            return false;
        }
    }

    bool isUnnamed(NamedDecl const *D) {
        return (bool)D->getDeclName() == false;
    }

    const TemplateSpecializationType* castToTemplateSpecializationType(QualType const& qualType) {
        const TemplateSpecializationType *TST = nullptr;
        if (qualType->getTypeClass() == Type::TemplateSpecialization) {
            TST = dyn_cast<TemplateSpecializationType>(qualType.getTypePtr());
        }
        else if (qualType->getTypeClass() == Type::Elaborated) {
            auto type = dyn_cast<ElaboratedType>(qualType.getTypePtr());
            TST = dyn_cast<TemplateSpecializationType>(type->getNamedType());
        }

        return TST;
    }

    bool containsTypeAliasTemplateTypesInType(const TemplateSpecializationType *TST) {
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
                    return true;
                }
                else if (containsTypeAliasTemplateTypesInType(tplType)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool isTemplateTypeAliasedTypeOrFunctionType(QualType const& type) {
        if (type.isNull()) {
            return false;
        }

        auto baseType = type;
        while (!baseType->getPointeeType().isNull()) {
            baseType = baseType->getPointeeType();
        }

        if (auto func = baseType->getAs<FunctionProtoType>()) {
            // checking type alias for in function prototype return type
            if (backport::helper::isTemplateTypeAliasedType(func->getReturnType())) {
                return true;
            }
            // checking type alias in parameter types.
            for (auto &param : func->getParamTypes()) {
                if (backport::helper::isTemplateTypeAliasedType(param)) {
                    return true;
                }
            }
            return false;
        }
        else {
            return backport::helper::isTemplateTypeAliasedType(type);
        }
    }

    bool isTemplateTypeAliasedType(QualType const& type) {
        if (type.isNull()) {
            return false;
        }

        auto baseType = type;
        while (!baseType->getPointeeType().isNull() && baseType->getTypeClass() != Type::TemplateSpecialization && baseType->getTypeClass() != Type::Elaborated)
            baseType = baseType->getPointeeType();

        auto TST = castToTemplateSpecializationType(baseType);
        if (TST == nullptr)
            return false;
        if (TST->isTypeAlias())
            return true;

        return containsTypeAliasTemplateTypesInType(TST);
    }

    SourceLocation getAfterPrototypeLocation(SourceManager& SM, const FunctionDecl *fd) {
        SourceLocation afterPrototypeLoc = getTokenEnd(SM, fd->getLocation()).getLocWithOffset(2);
        if (fd->parameters().size()) {
            auto lastParam = fd->parameters().back();
            afterPrototypeLoc = getTokenEnd(SM, lastParam->getLocEnd()).getLocWithOffset(1);
        }

        return afterPrototypeLoc;
    }

    std::string printToString(QualType const &D) {
        return print(D);
    }

    std::string printToString(Decl const *D) {
        return print(*D);
    }

    std::string printToString(Stmt const *N) {
        std::string s;
        llvm::raw_string_ostream oss(s);
        N->printPretty(oss, nullptr, getPrintingPolicy());
        return oss.str();
    }

    bool hasSubType(QualType q, std::function<bool(QualType const &)> matcher) {
        auto expandable = [](Type const &t) {
            return t.isPointerType() ||
                t.isFunctionPointerType() ||
                t.isMemberPointerType() ||
                t.isMemberDataPointerType() ||
                t.isMemberFunctionPointerType() ||
                t.isReferenceType() ||
                t.isFunctionType() ||
                t.isFunctionPointerType() ||
                t.isRecordType();
        };

        while (expandable(*q.getTypePtr())) {
            if (matcher(q))
                return true;
            else if (q.getTypePtr()->isPointerType() || q.getTypePtr()->isFunctionPointerType() ||
                q.getTypePtr()->isMemberDataPointerType() || q.getTypePtr()->isMemberFunctionPointerType() ||
                q.getTypePtr()->isMemberPointerType() || q.getTypePtr()->isReferenceType()) {
                q = q.getTypePtr()->getPointeeType();
            }
            else if (q->getTypeClass() == Type::Paren) {
                q = dyn_cast<ParenType>(q.getTypePtr())->getInnerType();
            }
            else if (q.getTypePtr()->isFunctionType()) {
                auto code = printToString(q);
                FunctionType const *F = nullptr;
                if (q->getAsTagDecl() == nullptr)
                    F = dyn_cast<FunctionType>(q.getTypePtr());
                else
                    F = dyn_cast<FunctionType>(q->getAsTagDecl()->getAsFunction()->getType().getTypePtr());

                if (F == nullptr)
                    F = dyn_cast<FunctionType>(q.getTypePtr());

                if (F == nullptr)
                    return false;

                if (matcher(F->getReturnType()) || hasSubType(F->getReturnType(), matcher))
                    return true;

                auto const FP = dyn_cast<FunctionProtoType>(F);
                if (FP == nullptr)
                    return false;


                for (int i = 0, numP = FP->getNumParams(); i < numP; ++i) {
                    if (matcher(FP->getParamType(i)) || hasSubType(FP->getParamType(i), matcher))
                        return true;
                }

                return false;
            }
            else if (q.getTypePtr()->getAsCXXRecordDecl() != nullptr) {
                auto const C = q.getTypePtr()->getAsCXXRecordDecl();
                
                auto const TC = dyn_cast<ClassTemplateSpecializationDecl>(C);

                if (TC != nullptr) {
                    for (auto &c : TC->getTemplateArgs().asArray()) {
                        if (c.getKind() == clang::TemplateArgument::ArgKind::Type && 
                            c.getAsType().isNull() == false) {
                            auto const ct = c.getAsType();
                            if (matcher(ct) || hasSubType(ct, matcher)) {
                                return true;
                            }
                        }
                    }
                }

                
                return false;
            }
            else {
                return false;
            }
        }

        return matcher(q);
    }

    namespace {
        bool _typeReplaceHelper_desugar(QualType &q, ASTContext *Context) {
            if (q != q.getCanonicalType()) {
                q = q.getCanonicalType();
                return true;
            }
            else {
                return false;
            }
        }

        std::shared_ptr<SplittableString> _typeReplaceHelper_abort_or_ignore(QualType &q, bool failIfTypeIsNotHandled) {
            if (failIfTypeIsNotHandled) {
                LOG(TLogLevel::logERROR) << "Unknown type. This should never happen. Maybe the user handler is incorrect, in the way that it doesn't handle types which it should. \n";
                abort();
            }
            else {
                // Just print out the type.
                return std::make_shared<SplittableString>(printToString(q));
            }
        }
    }

    std::shared_ptr<SplittableString> replaceTypeRecursivly(clang::QualType q, Transform &Owner,
        SourceManager &SM, ASTContext *Context, userTypeReplacerFunction userReplacer, bool failIfTypeIsNotHandled) {
        std::string type;
        std::string suff;

        auto userRes = userReplacer(q, Owner, SM, Context);

        if (userRes.first.get() != nullptr) {
            return userRes.first;
        }
        else if (userRes.second == true) {
            return nullptr;
        }

        bool desugaringSuccess = true;

        for (; true && desugaringSuccess; (desugaringSuccess = _typeReplaceHelper_desugar(q, Context))) {
            if (dyn_cast<DecltypeType>(q.getTypePtr()) != nullptr)
                continue;
            if (q.getTypePtr()->isPointerType()) {
                std::tie(type, suff) = *replaceTypeRecursivly(q->getPointeeType(), Owner, SM, Context, userReplacer, failIfTypeIsNotHandled);

                if (q->getPointeeType()->isFunctionType()) {
                    type += "(";
                    suff = ") " + suff;
                }
                type += " * ";
                if (!q->getPointeeType()->isFunctionType() && q.isConstQualified()) {
                    suff += " const ";
                }

                return std::make_shared<SplittableString>(type, suff);
            }
            else if (q->getTypeClass() == Type::Paren) {
                return replaceTypeRecursivly(dyn_cast<ParenType>(q.getTypePtr())->getInnerType(), Owner, SM, Context, userReplacer, failIfTypeIsNotHandled);
            }
            else if (q->isLValueReferenceType()) {
                std::tie(type, suff) = *replaceTypeRecursivly(q->getPointeeType(), Owner, SM, Context, userReplacer, failIfTypeIsNotHandled);

                if (q->getPointeeType()->isFunctionType()) {
                    type += "(";
                    suff = ") " + suff;
                }
                type += " & ";
                if (!q->getPointeeType()->isFunctionType() && q.isConstQualified()) {
                    suff += " const ";
                }

                return std::make_shared<SplittableString>(type, suff);
            }
            else if (q->isRValueReferenceType()) {
                std::tie(type, suff) = *replaceTypeRecursivly(q->getPointeeType(), Owner, SM, Context, userReplacer, failIfTypeIsNotHandled);

                if (q->getPointeeType()->isFunctionType()) {
                    type += "(";
                    suff = ") " + suff;
                }
                type += " && ";
                if (!q->getPointeeType()->isFunctionType() && q.isConstQualified()) {
                    suff += " const ";
                }

                return std::make_shared<SplittableString>(type, suff);
            }
            else if (q->isFunctionType()) {
                auto F = dyn_cast<FunctionType>(q.getTypePtr());

                if (F == nullptr)
                    continue;

                type = *replaceTypeRecursivly(F->getReturnType(), Owner, SM, Context, userReplacer, failIfTypeIsNotHandled) + " ";

                suff = "(";
                auto PF = dyn_cast<FunctionProtoType>(F);
                bool first = true;
                if (PF) {
                    for (int i = 0, numP = PF->getNumParams(); i < numP; ++i) {
                        if (first == false)
                            suff += " , ";
                        suff += *replaceTypeRecursivly(PF->getParamType(i), Owner, SM, Context, userReplacer, failIfTypeIsNotHandled) + " ";
                        first = false;
                    }
                }

                suff += ")";

                return std::make_shared<SplittableString>(type, suff);
            }
            else if (q->getAsCXXRecordDecl() != nullptr) {
                auto C = q->getAsCXXRecordDecl();

                if (C == nullptr)
                    continue;

                auto TC = dyn_cast<ClassTemplateSpecializationDecl>(C);

                type = getQualifiedNameAsString(C, Owner, SM);
                bool first = true;
                if (TC != nullptr) {
                    // MUST avoid things like 'vector<::std::pair<::std::string, ::std::string> >' bc '<:' is a digraph
                    type += "< ";
                    for (auto &cp : TC->getTemplateArgs().asArray()) {
                        if (first == false)
                            type += " , ";
                        if (cp.getKind() == clang::TemplateArgument::ArgKind::Type)
                            type += " " + *replaceTypeRecursivly(cp.getAsType(), Owner, SM, Context, userReplacer, failIfTypeIsNotHandled) + " ";
                        else
                            type += printToStringReverse(cp);
                        first = false;
                    }

                    type += " > ";
                }

                if (q.isConstQualified()) {
                    type = " const " + type;
                }

                return std::make_shared<SplittableString>(type);
            }
            else {
                // Unkown type?
                return _typeReplaceHelper_abort_or_ignore(q, failIfTypeIsNotHandled);
            }
        }

        // We shouldn't get here.
        return _typeReplaceHelper_abort_or_ignore(q, failIfTypeIsNotHandled);
    }


    /* Comparing the positions SourceLocations.*/
    bool less(SourceManager const &SM, clang::SourceLocation const &a, clang::SourceLocation const &b) {
        assert(SM.getFileID(a) == SM.getFileID(b));
        return SM.getFileID(a) == SM.getFileID(b) && (SM.getSpellingLineNumber(a) < SM.getSpellingLineNumber(b) ||
            (SM.getSpellingLineNumber(a) == SM.getSpellingLineNumber(b) && SM.getSpellingColumnNumber(a) < SM.getSpellingColumnNumber(b)));
    }

    /* Comparing the positions SourceLocations.*/
    bool equal(SourceManager const &SM, clang::SourceLocation const &a, clang::SourceLocation const &b) {
        assert(SM.getFileID(a) == SM.getFileID(b));
        return SM.getFileID(a) == SM.getFileID(b) && 
            (SM.getSpellingLineNumber(a) == SM.getSpellingLineNumber(b) && SM.getSpellingColumnNumber(a) == SM.getSpellingColumnNumber(b));
    }

    void replaceEnd(std::shared_ptr<SplittableString> typeT, Transform &Owner, SourceManager &SM, SourceLocation const &nameEnd, SourceLocation const &codeEnd) {
        if (codeEnd.isInvalid() == false) {

            if (less(SM, codeEnd, nameEnd))
                Owner.addTracedReplacement(SM, makeInsertionPoint(nameEnd), typeT->second);
            else
                Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(nameEnd, getTokenEnd(SM, codeEnd)), typeT->second);
        }
        else if (typeT->second.size() > 0) {
            Owner.addTracedReplacement(SM, makeInsertionPoint(nameEnd), typeT->second);
        }
    }


    void updateMappings(std::shared_ptr<DatabaseInterface> dbc, std::set<backport::ReplacementData> const &rlist, std::map<std::string, unsigned int > const &fileLengts) {

        std::vector<backport::ReplacementData> replacements;

        replacements.reserve(rlist.size());

        std::copy(rlist.begin(), rlist.end(), std::back_inserter(replacements));

        auto comp = [](backport::ReplacementData const &a, backport::ReplacementData const &b) {
            return a.fileData.fileId < b.fileData.fileId || (a.fileData.fileId == b.fileData.fileId && (
                a.startLine < b.startLine || (a.startLine == b.startLine && (
                    a.startLine + a.numberOfLinesReplaced < b.startLine + b.numberOfLinesReplaced || (a.startLine + a.numberOfLinesReplaced == b.startLine + b.numberOfLinesReplaced && (
                            a.replacement_text > b.replacement_text
                        ))
                    ))
                ));
        };

        std::sort(replacements.begin(), replacements.end(), comp);

        std::unique(replacements.begin(), replacements.end());

        dbc->beginTransaction();

        std::string lastFilePath = "<invalid file path (default value for this variable)>";
        int count = 0;

        //TODO: Replacements are not linked to a transformation
        //TODO: lines has one line origin from the _same_ file
        //TODO: replacements are currently processed in the wrong order, bc column information is not used.
        for (auto it = replacements.begin(); it != replacements.end(); ++it) {
            lastFilePath = it->fileData.filepath;

            if (dbc->isFileInDb(lastFilePath) == false)
                dbc->addFile(lastFilePath);

            int fileOffs = 0;


            unsigned int fileLenght = fileLengts.find(it->fileData.filepath)->second;

            // Register every line.
            for (unsigned int i = 1; i <= fileLenght + fileOffs; ++i)
            if (dbc->isActualLineInTheDb(it->fileData.filepath, i) == false)
                dbc->updateLineOffsetMapping(it->fileData.filepath, i, dbc->getOriginalLineNumber(it->fileData.filepath, i) - fileOffs);

            // Go through a file
            for (; it != replacements.end() && lastFilePath == it->fileData.filepath; ++it, ++count) {
                int change = it->numberOfLinesInReplacementText - it->numberOfLinesReplaced;
                int startline = it->startLine + fileOffs;
                int totalLinesOfFile = fileLenght + fileOffs;

                if (change != 0) {
                    // Offset the lines after

                    if (change > 0) {
                        for (int i = totalLinesOfFile; i >= startline + it->numberOfLinesReplaced; --i) {
                            dbc->updateLineOffsetMapping(it->fileData.filepath, i + change, dbc->getOriginalLineNumber(it->fileData.filepath, i));
                        }
                    }
                    else {
                        for (int i = startline + it->numberOfLinesReplaced; i <= totalLinesOfFile; ++i) {
                            dbc->updateLineOffsetMapping(it->fileData.filepath, i + change, dbc->getOriginalLineNumber(it->fileData.filepath, i));
                        }
                    }
                }

                // Replaced lines
                for (int i = startline; i < startline + it->numberOfLinesInReplacementText; ++i) {
                    dbc->updateLineOffsetMapping(it->fileData.filepath, i, it->from.start_line);
                }

                fileOffs += change;
            }

            if (it == replacements.end())
                break;
        }

        dbc->endTransaction();
    }

    int getOffsetToInsertName(Decl const *D, SourceManager &SM) {
        std::string source;

        int leftOffs = 0;
        int rightOffs = 0;

        source = getSourceCode(SM, CharSourceRange::getCharRange(D->getSourceRange().getBegin().getLocWithOffset(rightOffs), D->getSourceRange().getEnd().getLocWithOffset(leftOffs)));

        int pos = -1; // no match

        for (std::size_t i = 0; i < source.size(); ++i) {
            if (source[i] == '{' || source[i] == ';') {
                break;
            }
            else if (hasEnding(source.substr(0, i), "struct")) {
                pos = i;
                break;
            }
            else if (hasEnding(source.substr(0, i), "class")) {
                pos = i;
                break;
            }
            else if (hasEnding(source.substr(0, i), "union")) {
                pos = i;
                break;
            }
        }

        assert(pos != -1 && "couldn't find where to put the recordname.");

        if (pos == -1)
            abort();

        return pos + rightOffs;
    }

    std::string insertName(NamedDecl const *D, Transform &Owner, SourceManager &SM, std::string name) {
        std::string source = getSourceCode(SM, CharSourceRange::getCharRange(D->getSourceRange().getBegin(), D->getSourceRange().getEnd()));

        int pos = getOffsetToInsertName(D, SM);

        SourceLocation nameLoc = D->getSourceRange().getBegin().getLocWithOffset(pos);

        if (name == "") {
            name = getName(D, Owner, SM);
        }

        Owner.addTracedReplacement(SM, CharSourceRange::getCharRange(nameLoc, nameLoc),
            std::string(" ") + name + " ");

        return name;
    }

    std::string getName(NamedDecl const *D, Transform &Owner, SourceManager &SM) {

        static std::set < std::tuple<unsigned int, unsigned int, unsigned int> > alreadyNamed;

        if (isUnnamed(D)){
            unsigned int fid = SM.getFileID(D->getLocStart()).getHashValue();
            unsigned int lnum = SM.getSpellingLineNumber(D->getLocStart());
            unsigned int cnum = SM.getSpellingColumnNumber(D->getLocStart());
            std::string const name = std::string("_____backport__name_unnamed_cxxrecord___") + "uid_" +
                std::to_string(fid) + "_" +
                std::to_string(lnum) + "_" + std::to_string(cnum);

            auto id = std::make_tuple(fid, lnum, cnum);

            if (alreadyNamed.count(id) == 0) {
                insertName(D, Owner, SM, name);
                alreadyNamed.insert(id);
            }
            return name;
        }
        else {
            return (std::string)D->getName();
        }
    }

    std::string getQualifiedNameAsString(Decl const *D, Transform &Owner, SourceManager &SM) {
        if (dyn_cast_or_null<NamedDecl>(D) != nullptr) {
            return getQualifiedNameAsString(static_cast<NamedDecl const *>(D), Owner, SM);
        }
        else {
            return "";
        }
    }

    std::string getQualifiedNameAsString(NamedDecl const *D, Transform &Owner, SourceManager &SM) {
        

        std::string prefix;

        auto declContext = D->getDeclContext();

        while (declContext != nullptr && declContext->isTransparentContext())
            declContext = declContext->getLexicalParent();

        if (declContext == nullptr)
            return "";

        if (declContext->isNamespace()) {

            auto namesp = dyn_cast<NamespaceDecl>(declContext);

            if (namesp == nullptr)
                return "";

            if (namesp->isInlineNamespace()) {
                prefix = getQualifiedNameAsString(namesp, Owner, SM);

            }
            else if (namesp->isAnonymousNamespace()) {
                prefix = "";
            }
            else {
                prefix = getQualifiedNameAsString(namesp, Owner, SM);
            }

        } else if (!declContext->isFunctionOrMethod()) {
            prefix = getQualifiedNameAsString(dyn_cast_or_null<Decl>(declContext), Owner, SM);
        }

        if (prefix.empty() != true)
            prefix += "::";

        return prefix + getName(D, Owner, SM);
    }

    std::pair<std::shared_ptr<SplittableString>, bool> namer(clang::QualType const &q, Transform &Owner,
        SourceManager &SM, ASTContext *Context) {
        if (hasSubType(q, [](QualType const &t) { return t->isRecordType(); }) == false) {
            return std::make_pair<std::shared_ptr<SplittableString>, bool>(std::make_shared<SplittableString>(print(q)), false);
        }

        else
            return std::make_pair<std::shared_ptr<SplittableString>, bool>(nullptr, false);
    }

    std::string getQualifiedNameAsStringWithTemplateArgs(NamedDecl const *D, Transform &Owner, SourceManager &SM) {
        std::string result = getQualifiedNameAsString(D, Owner, SM);


        if (dyn_cast_or_null<ClassTemplateSpecializationDecl>(D) != nullptr) {
            auto TC = dyn_cast_or_null<ClassTemplateSpecializationDecl>(D);
            bool first = true;

            result += "< ";
            for (auto &cp : TC->getTemplateArgs().asArray()) {
                if (first == false)
                    result += " , ";
                if (cp.getKind() == clang::TemplateArgument::ArgKind::Type) {
                    if (cp.getAsType()->isRecordType() && cp.getAsType()->getAsCXXRecordDecl() != nullptr)
                        result += " " + getQualifiedNameAsStringWithTemplateArgs(cp.getAsType()->getAsCXXRecordDecl(), Owner, SM) + " ";
                    else
                        result += *replaceTypeRecursivly(cp.getAsType(), Owner, SM, &D->getASTContext(), namer);
                }
                else
                    result += printToStringReverse(cp);
                first = false;
            }

            result += " > ";
        }

        return result;
    }

    std::string getFunctionQualifiers(const FunctionType *func) {
        std::string qualifiers;
        if (func->isConst()) {
            qualifiers += " const ";
        }

        if (func->isVolatile()) {
            qualifiers += " volatile ";
        }

        return qualifiers;
    }

} /*namespace helper*/ } /*namespace backport*/
