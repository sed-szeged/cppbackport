#include "TransformBase/TransformHandler.h"
#include "Util/TransformUtility.h"
#include "Util/StringUtility.h"
#include "ModifyAutoMatchers.h"

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace clang;

typedef ReplacerTemplate<class AutoFuncDecl> AutoFuncDeclReplacer;
typedef ReplacerTemplate<class AutoFuncSpecDecl> AutoFuncSpecDeclReplacer;
typedef ReplacerTemplate<class AutoFuncCall> AutoFuncCallReplacer;

// Transforms declarations and definitons of auto return functions where the return type is a dependent type
// Also transforms the definitions of the specializations
template<>
void AutoFuncDeclReplacer::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const FunctionTemplateDecl *TF = Result.Nodes.getNodeAs<clang::FunctionTemplateDecl>(ModAutoFuncDeclId);
    assert(TF && "Bad Callback. No node provided");

    if (!Owner.isFileModifiable(SM, TF->getLocStart())) {
        return;
    }

    auto Policy = getPrintingPolicy();

    // Stream for printing the declaration
    std::string s_decl;
    llvm::raw_string_ostream rso_decl(s_decl);

    // Stream for the transformed declaration
    std::string s_newDecl;
    llvm::raw_string_ostream rso_newDecl(s_newDecl);

    TF->print(rso_decl, Policy);

    // We'll take the template declaration part of the original, but add an extra template parameter for the return type 'ReturnTypeOfAutoFunction'
    rso_newDecl << rso_decl.str().substr(0, rso_decl.str().find("> auto ", 0)) << ", typename ReturnTypeOfAutoFunction>\n" << "ReturnTypeOfAutoFunction " << (TF->isOutOfLine() ? TF->getQualifiedNameAsString() : TF->getNameAsString()) << "(";
    // FIXME: Use code below instead of getNamespaceForDecl it's not prepared for special cases and unsafe.
    // (TF->isOutOfLine() ? TF->getQualifiedNameAsString() : TF->getNameAsString())

    // Construct the parameter list of the function
    bool first = true;
    for (auto param : TF->getAsFunction()->parameters()) {
        if (!first) {
            rso_newDecl << ", ";
        }

        // FIXME : Shouldn't we add defaulted values here?
        rso_newDecl << param->getType().getAsString(Policy) << " " << param->getNameAsString();

        first = false;
    }
    rso_newDecl << ")";

    // We check if this is the definition or a declaration only
    if ((TF->getAsFunction()->getBody() == nullptr) || (!TF->isThisDeclarationADefinition())) {
        // The sourcerange info on declarations is horrible
        auto range = CharSourceRange::getTokenRange(
            SM.getSpellingLoc(TF->getSourceRange().getBegin()),
            SM.getSpellingLoc(functionDeclRangeEnd(TF)));

        // Since this is just a declaration we'll add ';' at the end since we'll alomst surely replace the original one
        Owner.addTracedReplacement(SM, range, rso_newDecl.str() + std::string(";\n"));
    } else {
        // If this is the definition we'll replace just until the body starts
        // We don't need to alter the body of the function at all
        auto range = CharSourceRange::getTokenRange(
            SM.getSpellingLoc(TF->getSourceRange().getBegin()),
            SM.getSpellingLoc(TF->getAsFunction()->getBody()->getLocStart().getLocWithOffset(-1)));

        Owner.addTracedReplacement(SM, range, rso_newDecl.str());
    }

    ++AcceptedChanges;

    // We'll need to deal with explicit specializations of the function
    for (auto TFS : TF->specializations()) {
        if (TFS->getTemplateSpecializationKind() != clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {
            // Stream for the transformed explicit specialization
            std::string s_specDecl;
            llvm::raw_string_ostream rso_specDecl(s_specDecl);
            
            // Get the desugared return type for the declaration
            std::string returnType = TFS->getAsFunction()->getCanonicalDecl()->getReturnType().getCanonicalType().getAsString(Policy);
            rso_specDecl << "template<>\n" << returnType << " ";

            // Stream where we will get declaration of the function with the proper specialization types written
            std::string s_fullName;
            llvm::raw_string_ostream rso_fullName(s_fullName);

            // Get the diagnostic name
            // Example : foo<int, double>
            TFS->getNameForDiagnostic(rso_fullName, Policy, true);

            // At the end of the template specialization types we'll append the return type as the extra template parameter
            rso_specDecl << rso_fullName.str().substr(0, rso_fullName.str().length() - 1) + ", " + returnType + ">(";

            // We'll generate the parameter list of the specialization
            bool first = true;
            for (auto param : TFS->getAsFunction()->parameters()) {
                if (!first) {
                    rso_specDecl << ", ";
                }

                // FIXME : default values of parameters ignored
                rso_specDecl << param->getType().getAsString(Policy) << " " << param->getNameAsString();

                first = false;
            }
            rso_specDecl << ")";

            // Just to be sure we'll check if this is the definition
            if ((TFS->getAsFunction()->getBody() == nullptr) || (!TFS->isThisDeclarationADefinition())) {
                // The method would work properly if it's a declaration only
                // But '->specializations()' returns the definitions only, so this code should never run
                auto range = CharSourceRange::getTokenRange(
                    SM.getSpellingLoc(TF->getSourceRange().getBegin()),
                    SM.getSpellingLoc(functionDeclRangeEnd(TFS)));

                Owner.addTracedReplacement(SM, range, rso_specDecl.str() + std::string(";\n"));
            } else {
                auto range = CharSourceRange::getTokenRange(
                    SM.getSpellingLoc(TFS->getSourceRange().getBegin()),
                    SM.getSpellingLoc(TFS->getAsFunction()->getBody()->getLocStart().getLocWithOffset(-1)));

                Owner.addTracedReplacement(SM, range, rso_specDecl.str());
            }

            ++AcceptedChanges;
        }
    }
}

// Transform declarations of specializations of auto return functions where the return type is a dependent type
// NOTE : The definitions of specializations are taken care of in the AutoFuncDeclReplacer function
template<>
void AutoFuncSpecDeclReplacer::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const FunctionDecl *FD = Result.Nodes.getNodeAs<clang::FunctionDecl>(ModAutoFuncSpecDeclId);
    assert(FD && "Bad Callback. No node provided");

    if (!Owner.isFileModifiable(SM, FD->getLocStart())) {
        return;
    }

    // We are only interested in explicit declarations
    if ((FD->isFunctionTemplateSpecialization()) && (FD->getTemplateSpecializationKind() != clang::TemplateSpecializationKind::TSK_ImplicitInstantiation)) {
        auto Policy = getPrintingPolicy();

        // Stream for the transformed explicit specialization
        std::string s_specDecl;
        llvm::raw_string_ostream rso_specDecl(s_specDecl);

        // Stream where we will get declaration of the function with the proper specialization types written
        std::string s_fullName;
        llvm::raw_string_ostream rso_fullName(s_fullName);

        // Get the desugared return type for the declaration
        std::string returnType = FD->getAsFunction()->getCanonicalDecl()->getReturnType().getCanonicalType().getAsString(Policy);
        rso_specDecl << "template<>\n" << returnType << " ";

        // Get the diagnostic name
        // Example : foo<int, double>
        FD->getNameForDiagnostic(rso_fullName, Policy, true);

        // At the end of the template specialization types we'll append the return type as the extra template parameter
        rso_specDecl << rso_fullName.str().substr(0, rso_fullName.str().length() - 1) + ", " + returnType + ">(";

        // We'll generate the parameter list of the specialization
        bool first = true;
        for (auto param : FD->getAsFunction()->parameters()) {
            if (!first) {
                rso_specDecl << ", ";
            }
            
            // FIXME : default values of parameters ignored, but is it necessary if it's a declaration only?
            rso_specDecl << param->getType().getAsString(Policy) << " " << param->getNameAsString();

            first = false;
        }
        rso_specDecl << ")";

        // We know that this is a declaration, so we can't rely on the sourcerange info
        auto range = CharSourceRange::getTokenRange(
            SM.getSpellingLoc(FD->getSourceRange().getBegin()),
            SM.getSpellingLoc(functionDeclRangeEnd(FD)));
        
        Owner.addTracedReplacement(SM, range, rso_specDecl.str() + std::string(";\n"));

        ++AcceptedChanges;
    }
}

// Transform calls to auto return type functions we convert
// Since we'll add a template parameter that isn't one of the function call arguments, each call must fully specify the template arguments
template<>
void AutoFuncCallReplacer::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>(ModAutoFuncCallId);
    assert(CE && "Bad Callback. No node provided");

    if (!Owner.isFileModifiable(SM, CE->getLocStart())) {
        return;
    }

    auto Policy = getPrintingPolicy();

    // Stream for the function call with the template argument specializations written
    std::string s_callWithTemplArgs;
    llvm::raw_string_ostream rso_callWithTemplArgs(s_callWithTemplArgs);

    // Start the call with the function name
    rso_callWithTemplArgs << CE->getCalleeDecl()->getAsFunction()->getName() << "<";

    // List all the original template parameters
    bool first = true;
    for (auto param : CE->getCalleeDecl()->getAsFunction()->getTemplateSpecializationArgs()->asArray()) {
        if (!first) {
            rso_callWithTemplArgs << ", ";
        }

        rso_callWithTemplArgs << param.getAsType().getAsString(Policy);

        first = false;
    }

    // Add the return type as the final template parameter
    rso_callWithTemplArgs << ", " << CE->getCallReturnType().getCanonicalType().getAsString(Policy) << ">(";

    // List the call arguments
    first = true;
    for (auto param : CE->arguments()) {
        if (!first) {
            rso_callWithTemplArgs << ", ";
        }

        param->printPretty(rso_callWithTemplArgs, nullptr, Policy);

        first = false;
    }
    rso_callWithTemplArgs << ")";
    
    // Try to figure out if any namespace is necessary for the function call
    auto& ASTCont = CE->getCalleeDecl()->getASTContext();
    auto functionNamespace = getCurrentNamespace(CE->getCalleeDecl(), ASTCont);
    auto callNamespace = getCurrentNamespace(CE, ASTCont);

    std::string replacement;
    
    // FIXME : the namespaces are wrong if the function is a class method
    std::string missingNamespace;
    if (functionNamespace.find(callNamespace) == 0) {
        missingNamespace = functionNamespace.substr(callNamespace.size());
    } else {
        missingNamespace = functionNamespace;
    }
    replacement = missingNamespace + rso_callWithTemplArgs.str();

    auto range = CharSourceRange::getTokenRange(
        SM.getSpellingLoc(CE->getSourceRange().getBegin()),
        SM.getSpellingLoc(CE->getSourceRange().getEnd()));

    Owner.addTracedReplacement(SM, range, replacement);

    ++AcceptedChanges;
}
