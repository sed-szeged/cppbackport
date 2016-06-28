#include "DelegatingConstructorMatchers.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "Util/Log.h"

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;

using DelegatingConstructorReplacer = ReplacerTemplate<class DelegatingConstructorReplace>;

namespace {
    /**
    * \brief Stores information about delegating constructors and it's call.
    */
    struct DelegationMetaData {
        DelegationMetaData(const CXXConstructorDecl* ctor, CompoundStmt* body) : delegateCtor(ctor), body(body) { }
        const CXXConstructorDecl* delegateCtor;
        CompoundStmt* body;
        std::map<const ParmVarDecl*, Expr*> args; // params, arguments pairs
    };

    /**
    * \brief clang::RecursiveASTVisitor that searches for DeclRefExpr nodes for identifying params usage.
    */
    class DeclRefExprVisitor
        : public RecursiveASTVisitor<DeclRefExprVisitor> {
        friend class RecursiveASTVisitor<DeclRefExprVisitor>;

    public:
        DeclRefExprVisitor() {}

        /**
        * \brief Returns containing decl ref expressions.
        */
        std::vector<DeclRefExpr*> collectDeclRefExpressions(Stmt * const stmt) {
            TraverseStmt(stmt);
            return containingDeclRefs;
        }

    private:
        /**
        * \brief Collects DeclRefExpr expressions.
        */
        bool VisitDeclRefExpr(DeclRefExpr *D) {
            containingDeclRefs.push_back(D);
            return true;
        }

        std::vector<DeclRefExpr*> containingDeclRefs;
    };

    /**
    * \brief clang::RecursiveASTVisitor that searches for CXXConstructExpr node which satisfies the given requirements.
    */
    class ConditionalCXXConstructExprVisitor
        : public RecursiveASTVisitor<ConditionalCXXConstructExprVisitor> {
        friend class RecursiveASTVisitor<ConditionalCXXConstructExprVisitor>;

    public:
        ConditionalCXXConstructExprVisitor(std::function<bool(CXXConstructExpr*)> cond) : condition(cond) {}

        /**
        * \brief Returns the first CXXConstructExpr which fulfills the given condition.
        */
        CXXConstructExpr* getConstructExpression(Stmt * const stmt) {
            TraverseStmt(stmt);
            return delegatingConstructExpr;
        }

    private:
        /**
        * \brief Visits ConstructExpr nodes until the given condition is fulfilled.
        */
        bool VisitCXXConstructExpr(CXXConstructExpr *D) {
            if (condition(D)) {
                delegatingConstructExpr = D;
                return false;
            }
            return true;
        }

        CXXConstructExpr *delegatingConstructExpr;
        std::function<bool(CXXConstructExpr*)> condition;
    };

    /**
    * \brief Returns the original argument from the delegation chain.
    */
    std::string traceDelegationParameter(std::vector<DelegationMetaData> const& delegationChain, std::vector<DelegationMetaData>::reverse_iterator& currIt, std::string lookupParam) {
        // identify init expression with a constructor parameter
        // if the argument is not a constant initializer we will move up in the delegation chain until we identify one
        // FIXME: Use DeclRefExpr if exists to identify the corresponding ParmVarDecl
        for (auto lookupIt = currIt; lookupIt != delegationChain.rend(); ++lookupIt) {
            for (auto elem : lookupIt->args) {
                if (elem.first->getNameAsString() == lookupParam) { // weather the argument and the parmvardecl are equal
                    // we are looking for the next parameter in an upper delegation
                    lookupParam = printToString(elem.second);
                    break;
                }
            }
        }
        return lookupParam;
    }

    /**
    * \brief Search for a method definition in outer contexts.
    */
    CXXConstructorDecl const* lookupFunctionDefinitionInOuterContexts(CXXConstructorDecl const* delegateCtor) {
        auto ret = delegateCtor;
        auto parentCon = delegateCtor->getDeclContext();
        while ((parentCon = parentCon->getLexicalParent()) != nullptr) { // moving up in between the contexts
            for (auto elem : parentCon->noload_decls())
            if (elem->getPreviousDecl() == delegateCtor) { // previousdecl is pointing to a redeclaration
                if (auto def = dyn_cast<CXXConstructorDecl>(elem)) {
                    if (!def->doesThisDeclarationHaveABody()) { // still a redeclaration
                        continue;
                    }
                    ret = def;
                    break;
                }
            }
        }
        return ret;
    }
}

/**
* \brief This function transforms delegating constructors.
*/
template<>
void DelegatingConstructorReplacer::run(const MatchFinder::MatchResult &Result) {
    const CXXConstructorDecl *Ctor{ Result.Nodes.getNodeAs<CXXConstructorDecl>(DelegatingConstructorMatcherId) };
    assert(Ctor && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, Ctor->getLocStart())) {
        return;
    }

    // gathering delegation data: constructor bodies, call parameters and arguments
    auto delegateCtor = Ctor;
    std::vector<DelegationMetaData> delegationChain;
    while (delegateCtor->isDelegatingConstructor()) {
        auto prev = delegateCtor;
        delegateCtor = delegateCtor->getTargetConstructor();
        if (!delegateCtor->doesThisDeclarationHaveABody()) { // search for the definition in outer scope
            delegateCtor = lookupFunctionDefinitionInOuterContexts(delegateCtor);
        }
        // extracting body
        DelegationMetaData data{ delegateCtor, dyn_cast<CompoundStmt>(delegateCtor->getBody()) };
        if (prev->isDelegatingConstructor()) {
            int i = 0;
            ConditionalCXXConstructExprVisitor Visitor([](CXXConstructExpr* D) { return D->getConstructionKind() == CXXConstructExpr::CK_Delegating; });// finds the proper construct expression node
            auto delegateExpr = Visitor.getConstructExpression((*prev->init_begin())->getInit());
            // delegation call parameters
            for (auto param : delegateExpr->children()) {
                auto expr = dyn_cast<Expr>(param);
                data.args[delegateCtor->getParamDecl(i++)] = expr;
            }
        }
        delegationChain.push_back(data);
    }

    // we are generating bodies of the delegated constructors to the body of the current constructor
    // also we generate the constructor initializer list of the last delegating constructor
    std::string body;
    std::string ctorInitList;
    for (auto it = delegationChain.rbegin(); it != delegationChain.rend(); ++it) { // gather constructor bodies
        // get the initialization list of the last delegate
        if (it == delegationChain.rbegin()) {
            // init is a constructor initializer
            for (auto init : it->delegateCtor->inits()) {
                // skip implicit initializations
                if (!init->isWritten()) {
                    continue;
                }
                if (!ctorInitList.empty()) {
                    ctorInitList += ", ";
                }

                auto initExpr = init->getInit();

                std::string initName;
                if (init->isAnyMemberInitializer()) {
                    initName = init->getMember()->getNameAsString();
                }
                // special handling for base class initialization
                else if (init->isBaseInitializer()) {
                    initName = init->getBaseClass()->getAsCXXRecordDecl()->getQualifiedNameAsString();
                    // returns the first CXXConstructExpr
                    ConditionalCXXConstructExprVisitor Visitor([](CXXConstructExpr*) { return true; });
                    initExpr = Visitor.getConstructExpression(initExpr);
                }
                auto initStr = printToString(initExpr);
                bool first = true;
                unsigned int startOffset;
                // replacement of initializer list elements which contains call parameters with proper values
                for (auto substmt : initExpr->children()) {
                    if (first) {
                        startOffset = substmt->getLocStart().getRawEncoding(); // we have to substract this from the DeclRefExpressions location.
                        first = false;
                    }
                    DeclRefExprVisitor Visitor;
                    auto containingDeclRefs = Visitor.collectDeclRefExpressions(substmt); // collects the contained parameter uses
                    // matching locations and replacing declrefexpressions
                    for (auto elem : containingDeclRefs) {
                        auto rng = getFullRange(SM, elem->getSourceRange()); // range of the argument
                        auto declRefSrc = getSourceCode(SM, rng); // argument src
                        auto replace = traceDelegationParameter(delegationChain, it, declRefSrc); // traced value
                        // normalize the range of the parameter location to the string
                        auto repStart = rng.getBegin().getRawEncoding() - startOffset; // start of the replacement index
                        auto repLength = rng.getEnd().getRawEncoding() - startOffset - repStart; // length of the decl ref expression (parameter)
                        initStr.replace(repStart, repLength, replace);
                        startOffset -= replace.length() - repLength; // modify offset based on the replaced lengths
                    }
                }

                // base/member( arguments )
                ctorInitList += initName + "(" + initStr + ")";
            }
        }

        auto const compoundStmt = it->body;
        if (compoundStmt->body_empty())
            continue;

        std::string decl = "{";
        auto delegateBody = backport::helper::printToString(compoundStmt);
        // generate shadow declarations in the body of the constructor based on the call arguments
        for (auto elem : it->args) {
            decl += printToString(elem.first->getType()) + " ";
            auto arg = traceDelegationParameter(delegationChain, it, elem.first->getNameAsString());
            // variable name = value
            decl += elem.first->getNameAsString() + "=" + arg + ";";
        }
        delegateBody.replace(0, 1, decl);
        body += delegateBody;
    }

    // if the last delegate has constructor initializer then we have to add : character
    if (!ctorInitList.empty()) {
        ctorInitList = " : " + ctorInitList + " ";
    }
    auto initrng = CharSourceRange::getCharRange(getAfterPrototypeLocation(SM, Ctor), Ctor->getBody()->getLocStart());
    Owner.addTracedReplacement(SM, initrng, ctorInitList);

    // replaces the constructors body with the content of delegating constructors
    auto rng = CharSourceRange::getCharRange(Ctor->getBody()->getLocStart().getLocWithOffset(1), Ctor->getBody()->getLocStart().getLocWithOffset(1));
    Owner.addTracedReplacement(SM, rng, body);
    ++AcceptedChanges;
}
