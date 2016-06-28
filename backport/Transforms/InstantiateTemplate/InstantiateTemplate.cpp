#include "TransformBase/TransformHandler.h"
#include "Util/TransformUtility.h"
#include "InstantiateTemplateMatchers.h"

#include <sstream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace clang;

typedef ReplacerTemplate<class TemplateClassInstant> TemplateClassInstantiator;
typedef ReplacerTemplate<class TemplateFunctionInstant> TemplateFunctionInstantiator;

// Instantiate template classes that contain template dependent auto or decltype expression
template<>
void TemplateClassInstantiator::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const ClassTemplateDecl *TC = Result.Nodes.getNodeAs<clang::ClassTemplateDecl>(TemplateClassId);
    assert(TC && "Bad Callback. No node provided");

    if (!Owner.isFileModifiable(SM, TC->getLocStart())) {
        return;
    }

    // Don't do anything with forward declarations
    if (!TC->isThisDeclarationADefinition()) {
        return;
    }

    // Get the printing policy
    auto Policy = getPrintingPolicy();

    // Replacements inserted at the same position will be printed in lexicographic order
    // By inserting a comment with an increasing number at the start, we can maintain the order the specializations are found by clang
    // This is mostly useless since the order clang lists the specializations will usually be invalid in the case of template metaprogramming
    int specCount = 1000000;
    
    int count = 0;
    for (auto TCS : TC->specializations()) {
        if (TCS->getTemplateSpecializationKind() == clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {
            // Stream for diagnostic name (contains template specialization variables i.e.: foo<int, double>)
            std::string s_diagnosticName;
            llvm::raw_string_ostream rso_diagnosticName(s_diagnosticName);
            TCS->getNameForDiagnostic(rso_diagnosticName, Policy, true);
            
            // Get the name of the class
            std::string from = TCS->getName();

            // Stream for the entire class
            // Will not contain the specialization arguments or the "template<>" needed before
            std::string s_class;
            llvm::raw_string_ostream rso_class(s_class);
            TCS->print(rso_class, Policy);

            std::string specialization = rso_class.str();

            // The print will start either "clas foo" or "struct foo"
            // By starting to look for the name from pos 6 we make sure we match the actual class name
            auto start_pos = specialization.find(from, 6);

            // We'll replace the class name with the one that contains the specialization arguments
            specialization.replace(start_pos, from.length(), rso_diagnosticName.str());

            // Specialization count number for ordering the specializations
            std::stringstream ss;
            ss << specCount;
            ++specCount;
            
            // Putting the whole specialization together
            specialization = "\n/*" + ss.str() + "*/\ntemplate <>\n" + specialization + ";\n";

            // Insert the specialization after the generic definition
            Owner.addTracedReplacement(SM, makeInsertionPoint(SM.getSpellingLoc(TC->getSourceRange().getEnd().getLocWithOffset(2))), removeConstexprNoexcept(specialization));

            ++count;
        }
    }

    // Get the partial class specializations
    clang::ClassTemplateDecl TCC = *TC;
    llvm::SmallVector<clang::ClassTemplatePartialSpecializationDecl *, 10> TCPSL;
    TCC.getPartialSpecializations(TCPSL);

    // Partial specializations can be removed since full specializations are generated
    for (auto TCPS : TCPSL) {
        auto range = CharSourceRange::getTokenRange(TCPS->getSourceRange().getBegin(), TCPS->getSourceRange().getEnd());

        Owner.addTracedReplacement(SM, range, "/* Partial Specialization Removed - Full Specializations generated */");
    }

    // Stream for the generic class declaration
    std::string s_classDecl;
    llvm::raw_string_ostream rso_classDecl(s_classDecl);
    TC->print(rso_classDecl, Policy);

    // TODO: static class variables shouldn't be removed from the generic template
    std::string classDeclaration = rso_classDecl.str().substr(0, rso_classDecl.str().find(TC->getNameAsString() + std::string(" {"), 0)) + TC->getNameAsString() + " /* Class Body Removed - Specializations generated */";

    auto range = CharSourceRange::getTokenRange(
        SM.getSpellingLoc(TC->getLocStart()),
        SM.getSpellingLoc(TC->getLocEnd()));

    Owner.addTracedReplacement(SM, range, removeConstexprNoexcept(classDeclaration));

    ++AcceptedChanges;
}

// Instantiate template functions that contain template dependent auto or decltype expression
template<>
void TemplateFunctionInstantiator::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const FunctionTemplateDecl *TF = Result.Nodes.getNodeAs<clang::FunctionTemplateDecl>(TemplateFunctionId);
    assert(TF && "Bad Callback. No node provided");

    if (!Owner.isFileModifiable(SM, TF->getLocStart())) {
        return;
    }

    // Get the printing policy
    auto Policy = getPrintingPolicy();

    // Replacements inserted at the same position will be printed in lexicographic order
    // By inserting a comment with an increasing number at the start, we can maintain the order the specializations are found by clang
    // This is mostly useless since the order clang lists the specializations will usually be invalid in the case of template metaprogramming
    int specCount = 1000000;

    int count = 0;
    for (auto TFS : TF->specializations()) {
        if (TFS->getTemplateSpecializationKind() == clang::TemplateSpecializationKind::TSK_ImplicitInstantiation) {

            // Stream for the function specialization
            std::string s_funcSpec;
            llvm::raw_string_ostream rso_funcSpec(s_funcSpec);

            // Get the function return type
            TFS->getReturnType().print(rso_funcSpec, Policy);
            rso_funcSpec << "  ";

            // Get the diagnostic name containing the specialization arguments
            TFS->getNameForDiagnostic(rso_funcSpec, Policy, true);
            rso_funcSpec << " ( ";

            bool firstParam = true;
            for (std::size_t i = 0; i < TFS->getNumParams(); ++i)
            {   
                if (firstParam == false) {
                    rso_funcSpec << " , ";
                }

                firstParam = false;

                // Get the function parameter type
                TFS->getParamDecl(i)->getType()->getCanonicalTypeInternal().print(rso_funcSpec, Policy);

                rso_funcSpec << " ";

                // Get the function parameter name
                rso_funcSpec << TFS->getParamDecl(i)->getName();

                // TODO : Can there be default values in implicit specializations? If yes then we need to add those
            }

            rso_funcSpec << " ) ";

            // If the body is available insert that, if not, insert a semicolon to close the declaration
            // NOTE : The body should always be available
            if (TFS->getBody() != nullptr) {
                TFS->getBody()->printPretty(rso_funcSpec, nullptr, Policy, 0);
            } else {
                rso_funcSpec << ";";
            }

            // TODO : Check if member function

            // Specialization count number for ordering the specializations
            std::stringstream ss;
            ss << specCount;
            ++specCount;

            // Put the specialization together
            std::string specialization = "\n/*" + ss.str() + "*/\ntemplate <>\n" + rso_funcSpec.str();

            // Insert the specialization after the generic definition
            Owner.addTracedReplacement(SM, makeInsertionPoint(TF->getSourceRange().getEnd().getLocWithOffset(2)), removeConstexprNoexcept(specialization));

            ++count;
        }
    }

    // Remove the generic function body after the specializations are generated
    if ((count != 0) && (TF->isThisDeclarationADefinition()) && (TF->getAsFunction()->getBody() != nullptr)) {
        auto range = CharSourceRange::getTokenRange(
            SM.getSpellingLoc(TF->getAsFunction()->getBody()->getSourceRange().getBegin()),
            SM.getSpellingLoc(TF->getAsFunction()->getBody()->getSourceRange().getEnd()));

        Owner.addTracedReplacement(SM, range, "; /* Function Body Removed - Specialization generated */");

        ++AcceptedChanges;
    }
}

const char InstantiateTemplateID[] = "InstantiateTemplate";

struct InstantiateTemplateFactory : TransformFactory {
    InstantiateTemplateFactory() {
        Since.Clang = Version(2, 9);
        Since.Gcc = Version(4, 4);
        Since.Icc = Version(12);
        Since.Msvc = Version(10);
    }

    Transform *createTransform(const TransformOptions &Opts) override {
        TransformHandler *handler = new TransformHandler(InstantiateTemplateID, Opts, TransformPriorities::INSTANTIATE_TEMPLATE);
        auto &acc = handler->getAcceptedChanges();

        handler->initReplacers(
            new TemplateFunctionInstantiator(acc, *handler), makeTemplateFunctionMatcher,
            new TemplateClassInstantiator(acc, *handler), makeTemplateClassMatcher
            );

        return handler;
    }
};

// Register the factory using this statically initialized variable.
static TransformFactoryRegistry::Add<InstantiateTemplateFactory>
X("instantiate-template", "Instantiates templates for the used cases");
