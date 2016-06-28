#ifndef TRANSFORM_UTILITY_H
#define TRANSFORM_UTILITY_H

#include "TransformBase/TransformHandler.h"
#include <functional>
#include "StringUtility.h"
#include <memory>
#include <set>
#include <utility>
#include <list>
#include "Util/ReplacementData.h"
#include <map>
#include <type_traits>

namespace backport { namespace helper {

    /**
    * \brief Returns the given function pointer variable with the given name.
    */
    std::string rebuildFunctionPointerType(QualType const& type, std::string name);

    /**
    * \brief Returns the SourceRange for the file that contains the source location.
    */
    clang::SourceRange getFileSourceRange(clang::SourceLocation const &loc, clang::SourceManager const & SM);

    /**
    * \brief If the given SourceLocation points at the start of a token,
    *        it will return the SourceLocation of that tokens end
    *        else it will result in undefined behaviour.
    */
    clang::SourceLocation getTokenEnd(clang::SourceManager & sm, const clang::SourceLocation& loc);

    /**
    * \brief Returns a zero length CharSourceRange with the given SourceLocation
    */
    clang::CharSourceRange makeInsertionPoint(const clang::SourceLocation& loc);

    /**
    * \brief Returns the CharSourceRange of a given node that includes the last token
    */
    clang::CharSourceRange getFullRange(clang::SourceManager& sm, const clang::SourceRange& sr);

    /**
    * \brief Get the function template decl node if the function is a template specialization
    */
    const FunctionTemplateDecl* getFunctionTemplate(const FunctionDecl* FD, clang::ASTContext& ASTCon);

    /**
    * \brief Returns the endlocation of the given decl.
    */
    clang::SourceLocation functionDeclRangeEnd(const Decl* FD);

    /**
    * \brief Helper function for returning a printing policy for printing AST nodes.
    */
    clang::PrintingPolicy& getPrintingPolicy();

    /**
    * \brief Returns the type of the given ValueDecl as string.
    */
    std::string getType(const ValueDecl* var);

    /**
    * \brief Returns the source at the specified source range as string.
    */
    std::string getSourceCode(const clang::SourceManager& SM, const clang::CharSourceRange& range);

    /**
    * \brief Removes c++11 keywords like constexpr and noexcept from the text.
    */
    std::string removeConstexprNoexcept(std::string text);

    /**
    * \brief Returns the source at the specified source range as string.
    */
    std::string getSourceCode(const clang::SourceManager& SM, const clang::SourceRange& range);

    /**
    * \brief Returns the number of characters in the source until the specified separator from the given location.
    */
    int getLengthFromLocationUntilSeparator(clang::SourceManager const& SM, clang::SourceLocation const& loc, const char sep = ';');

    /**
    * \brief Returns the number of characters in the source until the next opening parentheses is closed
    */
    int getLengthFromLocationUntilClosingParentheses(clang::SourceManager const& SM, clang::SourceLocation const& loc, const char open = '(', const char close = ')');

    /**
    * \brief Returns true if the suffix of the given fullString contains ending string.
    */
    bool hasEnding(std::string const &fullString, std::string const &ending);

    /**
    * \brief Returns true if the fullString starts with the given beginning.
    */
    bool hasBeginning(std::string const &fullString, std::string const &beginning);

    /**
    * \brief Returns true if the given NamedDecl is unnamed.
    */
    bool isUnnamed(NamedDecl const *D);

    /**
    * \brief Attempts to cast the TypePtr to TemplateSpecializationType or ElaboratedType otherwise returns nullptr.
    */
    const TemplateSpecializationType* castToTemplateSpecializationType(QualType const& qualType);

    /**
    * \brief This function searches for nested template type aliases in template arguments.
    */
    bool containsTypeAliasTemplateTypesInType(const TemplateSpecializationType *TST);

    /**
    * \brief This function returns true if the given type contains template type alias type or a function type which contains a template type alias.
    */
    bool isTemplateTypeAliasedTypeOrFunctionType(QualType const& type);
    /**
    * \brief This function checks whether template type aliases used in the given type.
    */
    bool isTemplateTypeAliasedType(QualType const& type);

    /**
    * \brief This function returns the location of the end of function prototypes.
    */
    SourceLocation getAfterPrototypeLocation(SourceManager& SM, const FunctionDecl *fd);

    /**
    * \brief Returns the given QualType as string.
    */
    std::string printToString(QualType const &D);

    /**
    * \brief Returns the given Decl as string.
    */
    std::string printToString(Decl const *D);

    /**
    * \brief Returns the given Stmt as string.
    */
    std::string printToString(Stmt const* N);

    /**
    * \brief Returns the name of a NamedDecl if it is unnamed it names it.
    */
    std::string getName(NamedDecl const *D, Transform &Owner, SourceManager &SM);

    /**
    * \brief Inserts a name for a NamedDecl. Currently only works for RecordDecls.
    */
    std::string insertName(NamedDecl const *D, Transform &Owner, SourceManager &SM, std::string name = "");

    /**
    * \brief Returns the same name as getName except it includes as much qualification as possible.
    */
    std::string getQualifiedNameAsString(NamedDecl const *D, Transform &Owner, SourceManager &SM);

    /**
    * \brief Returns the same as getQualifiedNameAsString except with template argument if there are any.
    */
    std::string getQualifiedNameAsStringWithTemplateArgs(NamedDecl const *D, Transform &Owner, SourceManager &SM);

    /**
    * \brief Returns true if the given matcher matches(returns true) for the given type or for one of its subtype.
    */
    bool hasSubType(QualType q, std::function<bool(QualType const &)> matcher);

    typedef std::function<std::pair<std::shared_ptr<SplittableString>, bool>(clang::QualType const &q, Transform &Owner,
        SourceManager &SM, ASTContext *Context)> userTypeReplacerFunction;

    /**
    * \brief Returns a handled qualtype as a SplittableString. If you want to name something to this type you should put 
    * the name between the first and the second part of the result. If the userReplacer returns something non nullptr as 'first' it is returned.
    * If the userReplacer's result 'second' is true we stop and return nullptr. If failIfTypeIsNotHandled is true we abort if the type is not expandable,
    * (doesn't have subtypes) and the userReplacer didn't do anything with it and made us continue. If failIfTypeIsNotHandled is false in this case we
    * just print out the type.
    */
    std::shared_ptr<SplittableString> replaceTypeRecursivly(clang::QualType q, Transform &Owner,
        SourceManager &SM, ASTContext *Context, userTypeReplacerFunction userReplacer, bool failIfTypeIsNotHandled = true);

    /**
    * \brief Type replacer helper. Removes the original type which was after the variable name, and replaces it with typeT->second.
    */
    void replaceEnd(std::shared_ptr<SplittableString> typeT, Transform &Owner, SourceManager &SM, SourceLocation const &nameEnd, SourceLocation const &codeEnd);

    /* Comparing the positions SourceLocations.*/
    bool less(SourceManager const &SM, clang::SourceLocation const &a, clang::SourceLocation const &b);

    /* Comparing the positions SourceLocations.*/
    bool equal(SourceManager const &SM, clang::SourceLocation const &a, clang::SourceLocation const &b);

    void updateMappings(std::shared_ptr<DatabaseInterface> dbc, std::set<backport::ReplacementData> const &rlist, std::map<std::string, unsigned > const &fileLengts);

    /**
    * \brief Returns the SourceRange for the file that contains the given node.
    */
    template<typename T>
    static inline clang::SourceRange getFileSourceRange(const T* N, clang::SourceManager const & SM) {
        return getFileSourceRange(N->getSourceRange().getBegin(), SM);
    }

    /**
     * \brief Seeks upwards in the AST until the parent of the current node matches the condition given as a lambda expression.
     */
    template<typename T>
    static inline clang::ast_type_traits::DynTypedNode getFirstContainingDynTypedParent_which(const T *N, clang::ASTContext& ASTCon,
        std::function<bool(clang::ast_type_traits::DynTypedNode const &)> predicate) {
        auto current = ASTCon.getParents(*N)[0];

        while (predicate(current) == false) {
            // FIXME : Handle when it tries to step up from the top node
            current = ASTCon.getParents(current)[0];
        }

        return current;
    }

    /**
     * \brief Seeks upwards in the AST until the parent of the current node matches the condition given as a lambda expression
     *        The found node will be cast to the given type if possible (if not, the result will be nullptr)
     */
    template<typename type, typename T>
    static inline type const *getFirstContainingParent_which_as(const T* N, clang::ASTContext& ASTCon, std::function<bool(clang::ast_type_traits::DynTypedNode const &)> predicate)  {
        return getFirstContainingDynTypedParent_which(N, ASTCon, predicate).template get<type>();
    }
    
    /**
     * \brief Returns the containing expression node
     *        In this case an expression is basically a piece of code terminated by a semicolon
     *        If the node is not part of such an expression, then the behaviour is undefined
     */
    template<typename T>
    static inline clang::ast_type_traits::DynTypedNode getContainingExpression_DynTypedNode(const T* N, clang::ASTContext& ASTCon) {
        return getFirstContainingDynTypedParent_which(N, ASTCon,
            [&](clang::ast_type_traits::DynTypedNode const &current) {
            return current.get<TranslationUnitDecl>() != nullptr || ((ASTCon.getParents(current)[0].template get<TranslationUnitDecl>() != nullptr) ||
                (ASTCon.getParents(current)[0].template get<NamespaceDecl>() != nullptr) ||
                (ASTCon.getParents(current)[0].template get<CXXRecordDecl>() != nullptr) ||
                (ASTCon.getParents(current)[0].template get<CompoundStmt>() != nullptr)); });
    }

    /**
     * \brief Returns the range of the expression that contains the given node
     */
    template<typename T>
    static inline clang::SourceRange getContainingExpressionRange(const T* N, clang::ASTContext& ASTCon) {
        if ((ASTCon.getParents(*N)[0].template get<TranslationUnitDecl>() != nullptr) ||
            (ASTCon.getParents(*N)[0].template get<NamespaceDecl>() != nullptr) ||
            (ASTCon.getParents(*N)[0].template get<CXXRecordDecl>() != nullptr) ||
            (ASTCon.getParents(*N)[0].template get<CompoundStmt>() != nullptr)) {
            return N->getSourceRange();
        }

        auto child = getContainingExpression_DynTypedNode(N, ASTCon);

        clang::SourceRange result;
        auto parentStmt = child.template get<Stmt>();
        if (parentStmt != nullptr) {
            result = parentStmt->getSourceRange();
        }

        auto parentDecl = child.template get<Decl>();
        if (parentDecl != nullptr) {
            result = parentDecl->getSourceRange();
        }

        if ((parentStmt == nullptr) && (parentDecl == nullptr)) {
            assert(false);
        }

        return result;
    }

    /**
     * \brief Retrieve the range of the functiondeclaration this node is in, works for any node
     *        If the node is not inside the function, it will simply return the node's range
     *        If the node is inside a template specialization, it will return the general declaration's range
     */
    template<typename T>
    static inline clang::SourceRange getOuterDeclarationRange(const T* N, clang::ASTContext& ASTCon) {
        auto parents = ASTCon.getParents(*N);
        auto parent = parents[0];
        auto child = parent;

        const Stmt* outerStmt = nullptr;

        while (parent.template get<TranslationUnitDecl>() == nullptr) {
            auto stmt = parent.template get<Stmt>();
            if (stmt != nullptr) {
                outerStmt = stmt;
            }
            child = parent;
            parents = ASTCon.getParents(parent);
            parent = parents[0];
        }

        if (outerStmt == nullptr) {
            clang::SourceLocation loc;

            clang::SourceRange range = N->getSourceRange();

            if (range.isInvalid()) {
                assert(false && "Invalid range for outer declaration");
            }

            return range;
        }

        parents = ASTCon.getParents(*outerStmt);
        parent = parents[0];

        auto tmp = outerStmt->getSourceRange().getBegin().printToString(ASTCon.getSourceManager());

        const FunctionDecl* outerFunctionDecl = parent.template get<FunctionDecl>();
        if (outerFunctionDecl == nullptr) {
            clang::SourceRange range = getContainingExpressionRange(outerStmt, ASTCon);

            if (range.isInvalid()) {
                assert(false && "Invalid range for outer declaration");
            }

            return range;
        }

        clang::SourceRange range;

        auto outerFunctionTemplateDecl = getFunctionTemplate(outerFunctionDecl, ASTCon);

        if (outerFunctionTemplateDecl != nullptr) {
            range = outerFunctionTemplateDecl->getSourceRange();
        } else {
            range = outerFunctionDecl->getSourceRange();
        }

        return range;
    }

    /**
     * \brief Attempt to return the namespace of the given node, works for any node type
     *        Will also return class scopes
     */
    template<typename T>
    static inline std::string getCurrentNamespace(const T* N, clang::ASTContext& ASTCon, bool getClassScope = true) {
        auto parents = ASTCon.getParents(*N);
        auto parent = parents[0];

        std::string scope;

        while (parent.template get<TranslationUnitDecl>() == nullptr) {

            if (parent.template get<NamespaceDecl>() != nullptr) {
                const NamespaceDecl* ND = parent.template get<NamespaceDecl>();
                auto name = ND->getNameAsString();
                scope = name + "::" + scope;
            } else if (getClassScope && (parent.template get<CXXRecordDecl>() != nullptr)) {
                const CXXRecordDecl* RD = parent.template get<CXXRecordDecl>();
                if (!RD->isImplicit()) {
                    auto name = RD->getNameAsString();
                    scope = name + "::" + scope;
                }
            }

            parents = ASTCon.getParents(parent);
            parent = parents[0];
        }

        return scope;
    }

    template<class T>
    static std::string print(T const &D) {
        std::string s;
        llvm::raw_string_ostream oss(s);
        D.print(oss, getPrintingPolicy());
        return oss.str();
    }

    template<class T>
    static std::string printToStringReverse(T const &D) {
        std::string s;
        llvm::raw_string_ostream oss(s);
        D.print(getPrintingPolicy(), oss);
        return oss.str();
    }

    template<class T>
    static inline std::string getNameWithoutTemplateParams(T const *D, Transform &Owner, SourceManager &SM) {
        if (D->getType().getNonReferenceType().getLocalUnqualifiedType().getTypePtr()->isRecordType())
            return getNameWithoutTemplateParams(D->getType().getNonReferenceType().getLocalUnqualifiedType().getTypePtr()->getAsCXXRecordDecl(), Owner, SM);
        else
            return printToString(D->getType().getNonReferenceType().getLocalUnqualifiedType());
    }

    template<>
    inline std::string getNameWithoutTemplateParams(clang::CXXRecordDecl const *D, Transform &Owner, SourceManager &SM) {
        return getName(D, Owner, SM);
    }

    template<class T>
    static inline std::string getNameWithPossiblyTemplateParameters(T const *D, Transform &Owner, SourceManager &SM) {
        if (D->getType().getNonReferenceType().getLocalUnqualifiedType().getTypePtr()->isRecordType())
            return getNameWithPossiblyTemplateParameters(D->getType().getNonReferenceType().getLocalUnqualifiedType().getTypePtr()->getAsCXXRecordDecl(), Owner, SM);
        else
            return printToString(D->getType().getNonReferenceType().getLocalUnqualifiedType());
    }

    template<>
    inline std::string getNameWithPossiblyTemplateParameters(clang::CXXRecordDecl const *D, Transform &Owner, SourceManager &SM) {

        if (D->getDescribedClassTemplate() != nullptr) {
            auto *DT = D->getDescribedClassTemplate();
            std::string name = getName(DT, Owner, SM);
            bool first = true;
            for (const auto& param : DT->getTemplateParameters()->asArray()) {
                if (first) {
                    first = false;
                    name += "<";
                }
                else {
                    name += ", ";
                }
                name += param->getName();
            }
            if (!first) {
                name += ">";
            }

            return name;
        }
        else if (isUnnamed(D) == false) {
            return printToString(D->getTypeForDecl()->getCanonicalTypeInternal());
        }
        else {
            return getName(D, Owner, SM);
        }
    }

    /**
    * \brief Returns a node storage modifiers.
    */
    template <class T>
    static inline std::string getStorageClassAsString(T const *var) {
        std::string result;

        if (dyn_cast<VarDecl>(var) != nullptr) {
            VarDecl const *VD = dyn_cast<VarDecl>(var);

            if (VD->getStorageClass() == clang::StorageClass::SC_Static) {
                result += " static ";
            }

            if (VD->getStorageClass() == clang::StorageClass::SC_Extern) {
                result += " extern ";
            }
        }

        return result;
    }

    /**
    * \brief Returns a node storage modifiers.
    */
    template <>
    inline std::string getStorageClassAsString(Stmt const *var) {
        return "";
    }

    /**
    * \brief Returns the function qualifiers as string.
    */
    std::string getFunctionQualifiers(const FunctionType *func);
} /*namespace helper*/ }/*namespace backport*/

#endif // TRANSFORM_UTILITY_H
