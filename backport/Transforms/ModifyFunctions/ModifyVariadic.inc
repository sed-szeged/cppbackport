#include "TransformBase/TransformHandler.h"
#include "Util/TransformUtility.h"
#include "Util/StringUtility.h"
#include "ModifyVariadicMatchers.h"
#include "ModifyVariadicUtil.h"

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace clang;

typedef ReplacerTemplate<class VariadicTemplateFunctionInstant> VariadicTemplateFunctionModifier;
typedef ReplacerTemplate<class VariadicFuncCall> VariadicFuncCallReplacer;
typedef ReplacerTemplate<class VariadicFuncFinalCall> VariadicFuncFinalCallReplacer;
typedef ReplacerTemplate<class NonVariadicParamNum> NonVariadicParamNumFinder;

// A struct to store information about the function parameter types for later use
struct FunctionParameterTypeInfo {
    std::string type;
    std::string name;
    std::string typeClassName;
    bool isConst = false;
    bool isReference = false;
    bool isDependent = false;
    bool isParameterPack = false;
};

static int changes = 0;

static std::map<std::string, unsigned int> NonVariadicVariableCountMap;

// Try to figure out the number of non variadic function parameters
unsigned int getNonVariadicVariableCount(const clang::FunctionTemplateDecl* VTF) {

    std::string funcName = VTF->getDeclName().isIdentifier() ? VTF->getName() : "";
    unsigned int nonVariadicFunctionArgumentCount = NonVariadicVariableCountMap[funcName];

    return nonVariadicFunctionArgumentCount;
}

// Replacer for the variadic function declaration
template<>
void VariadicTemplateFunctionModifier::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const FunctionTemplateDecl *VTF = Result.Nodes.getNodeAs<clang::FunctionTemplateDecl>(VariadicTemplateFunctionId);
    assert(VTF && "Bad Callback. No node provided");

    // Make sure that the file containing the node is allowed to be modified
    if (!Owner.isFileModifiable(SM, VTF->getLocStart())) {
        return;
    }
    
    auto Policy = getPrintingPolicy();

    // If the Variadic function has no body, it's never called, no need to transform
    if (VTF->getAsFunction()->getBody() == nullptr) {
        // We can't remove the function here, as it might be called from another file
        // If the variadic function is still present at the remove auto stage, then it can be safely removed
        Owner.TransformationSourceMap[InstantiateTemplateID].insert(Owner.getCurrentSource());
        return;
    }

    // To simulate variadic functions, we need to use a templated linked list like structure
    // If there's a variadic function in the file, insert the necessary helper classes at the beginning
    auto fileRange = getFileSourceRange(VTF, SM);
    auto fileStartRange = CharSourceRange::getCharRange(fileRange.getBegin(), fileRange.getBegin());
    Owner.addTracedReplacement(SM, fileStartRange, variadicInclude);

    // Find the variadic parameter in the template parameter list
    TemplateTypeParmDecl* parameterPack = nullptr;
    for (auto param : VTF->getTemplateParameters()->asArray()) {
        if (param->isTemplateParameterPack()) {
            parameterPack = dyn_cast<TemplateTypeParmDecl>(param);
            if (parameterPack != nullptr) {
                break;
            }
        }
    }
    assert(parameterPack && "Can't match parameter pack in variadic template");

    // Replace the variadic template parameter with a simple typename 
    auto parameterPackRange = CharSourceRange::getCharRange(parameterPack->getSourceRange().getBegin(), parameterPack->getSourceRange().getEnd().getLocWithOffset(parameterPack->getName().size()));
    Owner.addTracedReplacement(SM, parameterPackRange, "typename VariadicTemplateParameterPackSubstitute");

    unsigned int nonVariadicFunctionArgumentCount = getNonVariadicVariableCount(VTF);

    // Analyse the function parameters for type, name and source location
    std::vector<FunctionParameterTypeInfo> functionParametersInfo;
    clang::SourceLocation parameterUnpackRangeBegin;
    clang::SourceLocation parameterUnpackRangeEnd;
    unsigned int argCount = 0;
    for (auto param : VTF->getAsFunction()->params()) {
        ++argCount;
        FunctionParameterTypeInfo parameterInfo;
        auto paramType = param->getType().getTypePtrOrNull();
        if (paramType != nullptr) {
            if (paramType->isDependentType()) {
                parameterInfo.isDependent = true;
            }

            // If we are at the fist parameter that is part of the variadic parameter pack
            // Set the start of the source range
            if (argCount == nonVariadicFunctionArgumentCount + 1) {
                parameterUnpackRangeBegin = param->getSourceRange().getBegin();
            }

            // Keep pushing back the source end
            // The proper way to get the correct source end is different if a default argument is given
            if (param->hasDefaultArg()) {
                parameterUnpackRangeEnd = getTokenEnd(SM, param->getDefaultArg()->getSourceRange().getEnd());
            } else {
                parameterUnpackRangeEnd = getTokenEnd(SM, param->getSourceRange().getEnd());
            }

            parameterInfo.typeClassName = paramType->getTypeClassName();

            // The correct unqualified type of the parameter is extracted from the source code
            parameterInfo.type = getSourceCode(SM, clang::CharSourceRange::getCharRange(param->getTypeSourceInfo()->getTypeLoc().getSourceRange()));
            if (parameterInfo.type.length() == 0) {
                parameterInfo.type = getSourceCode(SM, getFullRange(SM, param->getTypeSourceInfo()->getTypeLoc().getSourceRange()));
            }

            parameterInfo.name = param->getName();

            // A typeClassName of PackExpansion indicates the variadic function parameter
            if (parameterInfo.typeClassName.compare("PackExpansion") == 0) {
                parameterInfo.isParameterPack = true;
                std::string s_paramPackType;
                llvm::raw_string_ostream rso_paramPackType(s_paramPackType);
                param->getType().print(rso_paramPackType, Policy);
                std::string paramPackTypeStr = rso_paramPackType.str();
                if (paramPackTypeStr.find("const ") == 0) {
                    parameterInfo.isConst = true;
                }
                // The reference check is different because of the trailing ...
                if (paramPackTypeStr.at(paramPackTypeStr.size() - 4) == '&') {
                    parameterInfo.isReference = true;
                }
            } else {
                std::string s_paramType;
                llvm::raw_string_ostream rso_paramType(s_paramType);
                param->getType().print(rso_paramType, Policy);
                std::string paramTypeStr = rso_paramType.str();
                if (paramTypeStr.find("const ") == 0) {
                    parameterInfo.isConst = true;
                }
                if (paramTypeStr.at(paramTypeStr.size() - 1) == '&') {
                    parameterInfo.isReference = true;
                }
            }
        }
        functionParametersInfo.push_back(parameterInfo);
    }

    clang::CharSourceRange parameterUnpackRange = clang::CharSourceRange::getCharRange(parameterUnpackRangeBegin, parameterUnpackRangeEnd);

    // Check if the variadic parameter is const and/or a reference
    bool parameterPackIsConst;
    bool parameterPackIsReference;
    for (const auto& info : functionParametersInfo) {
        if (info.isParameterPack) {
            parameterPackIsConst = info.isConst;
            parameterPackIsReference = info.isReference;
            break;
        }
    }

    // Create a replacement for the parameters that come from the variadic parameter pack
    std::string variadicParameterReplacement = "";
    if (parameterPackIsConst) {
        variadicParameterReplacement = "const ";
    }
    int unpackedParamteres = 0;
    argCount = 0;
    for (const auto& param : functionParametersInfo) {
        ++argCount;
        // Only take into account the parameters that are to be extracted from the variadic parameter pack
        if ((argCount > nonVariadicFunctionArgumentCount) && (!param.isParameterPack)) {
            variadicParameterReplacement += std::string("VariadicTemplateList< ") + param.type + std::string(", ");
            ++unpackedParamteres;
        }
    }
    variadicParameterReplacement += std::string("VariadicTemplateParameterPackSubstitute");
    while (unpackedParamteres > 0) {
        variadicParameterReplacement += std::string(" >");
        --unpackedParamteres;
    }
    if (parameterPackIsReference) {
        variadicParameterReplacement += "&";
    }
    variadicParameterReplacement += " variadicTemplateParameterPackSubstitute";

    Owner.addTracedReplacement(SM, parameterUnpackRange, variadicParameterReplacement);

    if (VTF->isThisDeclarationADefinition()) {
        // Find the start of the function body
        auto functionBodyStartRange = makeInsertionPoint(VTF->getAsFunction()->getBody()->getSourceRange().getBegin().getLocWithOffset(1));

        // Generate aliases for the parameters that we'll substitute, so that they can be reffered to with their original name
        std::string variadicNameSubstitution = "\n";
        std::string substitutionDepth = "";
        argCount = 0;
        for (auto param : functionParametersInfo) {
            ++argCount;
            if (argCount > nonVariadicFunctionArgumentCount && !param.isParameterPack) {
                if (param.name.length() != 0) {
                    variadicNameSubstitution += (param.isConst ? std::string("const ") : std::string("")) + param.type + "& " + param.name + " = variadicTemplateParameterPackSubstitute" + substitutionDepth + ".value;\n";
                }
                substitutionDepth += ".next";
            } else if (param.isParameterPack) {
                if (param.name.length() != 0) {
                    variadicNameSubstitution += (param.isConst ? std::string("const ") : std::string("")) + "VariadicTemplateParameterPackSubstitute& " + param.name + " = variadicTemplateParameterPackSubstitute" + substitutionDepth + ";\n";
                }
            }
        }

        Owner.addTracedReplacement(SM, functionBodyStartRange, variadicNameSubstitution);
    }

    // If the function's return type is defined with auto, append the proper decltype
    auto retType = VTF->getAsFunction()->getReturnType().getAsString();
    if (retType.find("decltype", 0) == 0) {
        std::string s_decltype;
        llvm::raw_string_ostream rso_decltype(s_decltype);
        // TODO : This isn't really possible. The values used in the decltype probably don't exist after the conversion
        // The following is experimental and only works with specific conditions (the variable names must not be a substring of anything else in the text)
        // Also the generated code can't be transformed by Modify Auto
        std::string newRetType = retType;

        argCount = 0;
        std::string substitutionDepth = "";
        for (auto param : functionParametersInfo) {
            ++argCount;
            if (argCount > nonVariadicFunctionArgumentCount) {
                if (param.name.length() != 0) {
                    std::string nameSubstitution = "variadicTemplateParameterPackSubstitute" + substitutionDepth + ".value";
                    ReplaceAll(newRetType, param.name, nameSubstitution);
                }
                substitutionDepth += ".next";
            }
        }

        rso_decltype << ") -> " << newRetType;

        auto decltypeRange = clang::CharSourceRange::getCharRange(parameterUnpackRange.getEnd(), VTF->getAsFunction()->getBody()->getLocStart().getLocWithOffset(-1));

        Owner.addTracedReplacement(SM, decltypeRange, rso_decltype.str());
    }

    //llvm::outs() << "1 - " << ++changes << "\n";
    ++AcceptedChanges;
}

// Replacer for calls to variadic functions
template<>
void VariadicFuncCallReplacer::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>(VariadicFuncCallId);
    assert(CE && "Bad Callback. No node provided");

    // Make sure that the file containing the node is allowed to be modified
    if (!Owner.isFileModifiable(SM, CE->getLocStart())) {
        return;
    }

    auto Policy = getPrintingPolicy();

    // Get a pointer to the generic template declaration of the called variadic function
    auto VTF = CE->getCalleeDecl()->getAsFunction()->getPrimaryTemplate();

    // If the called variadic function is in a file that we couldn't transform, don't transform the call either
    // NOTE: If variadic system functions are called, there's not much point in backporting
    if (!Owner.isFileModifiable(SM, VTF->getLocStart())) {
        return;
    }

    unsigned int nonVariadicFunctionArgumentCount = getNonVariadicVariableCount(VTF);

    // Extract the variadic arguments
    unsigned int argCount = 0;
    std::vector<std::string> functionCallVariadicArguments;
    clang::SourceLocation functionCallVariadicArgumentsBegin;
    clang::SourceLocation lastArgBegin;
    for (const auto& arg : CE->arguments()) {
        ++argCount;
        if (argCount > nonVariadicFunctionArgumentCount) {
            auto argStr = getSourceCode(SM, clang::CharSourceRange::getCharRange(arg->getSourceRange().getBegin(), getTokenEnd(SM, arg->getSourceRange().getEnd())));
            if (argCount == nonVariadicFunctionArgumentCount + 1) {
                functionCallVariadicArgumentsBegin = arg->getSourceRange().getBegin();
            }
            lastArgBegin = arg->getSourceRange().getBegin();
            functionCallVariadicArguments.push_back(argStr);
        }
    }
    if (argCount < nonVariadicFunctionArgumentCount + 1) {
        functionCallVariadicArgumentsBegin = CE->getSourceRange().getEnd().getLocWithOffset(-1);
    }

    // Check if the last argument is a parameter pack
    std::string lastArgName = "";
    bool hasVariadicArg = false;
    auto lastArgRange = clang::CharSourceRange::getCharRange(lastArgBegin, CE->getSourceRange().getEnd());
    if (lastArgRange.isValid()) {
        auto lastArgCodeEnd = getSourceCode(SM, lastArgRange);
        std::string lastArgCodeName = "";
        if (lastArgCodeEnd.length() > 3) {
            lastArgCodeName = lastArgCodeEnd.substr(0, lastArgCodeEnd.length() - 3);
            lastArgCodeEnd = lastArgCodeEnd.substr(lastArgCodeEnd.length() - 3);
        }
        if (lastArgCodeEnd.compare("...") == 0) {
            lastArgName = functionCallVariadicArguments[functionCallVariadicArguments.size() - 1];
            hasVariadicArg = true;
            if (lastArgName.compare(lastArgCodeName) == 0) {
                for (int i = functionCallVariadicArguments.size() - 1; i >= 0; --i) {
                    if (functionCallVariadicArguments[i].compare(lastArgName) == 0) {
                        functionCallVariadicArguments.pop_back();
                    }
                }
            } else {
                // TODO : Something is amiss, argument names get concatenated
                return;
            }
        }
    }

    // If there are neither variadic parameters, netiher parameter pack, there is nothing to do
    if ((functionCallVariadicArguments.size() == 0) && (!hasVariadicArg)) {
        return;
    }

    auto functionCallVariadicArgumentsRange = clang::CharSourceRange::getCharRange(functionCallVariadicArgumentsBegin, CE->getSourceRange().getEnd());

    // Create the substitute for the variadic arguments
    std::string argumentList = "";
    if (argCount < nonVariadicFunctionArgumentCount + 1) {
        if (argCount > 0) {
            argumentList = ", ";
        }
    }
    if (functionCallVariadicArguments.size() != 0) {
        argumentList = "variadicTemplateList";
    }
    for (const auto& arg : functionCallVariadicArguments) {
        argumentList += std::string("(") + arg + std::string(")");
    }
    if (hasVariadicArg) {
        if (functionCallVariadicArguments.size() > 0) {
            argumentList += std::string(", ");
        }
        argumentList += lastArgName;
    }

    Owner.addTracedReplacement(SM, functionCallVariadicArgumentsRange, argumentList);

    //llvm::outs() << "2 - " << ++changes << "\n";
    ++AcceptedChanges;
}

// Replacer for the variadic function final call declaration
template<>
void VariadicFuncFinalCallReplacer::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const FunctionTemplateDecl *VTF = Result.Nodes.getNodeAs<clang::FunctionTemplateDecl>(VariadicTemplateWithCallExprID);
    assert(VTF && "Bad Callback. No node provided");

    const Decl *D = Result.Nodes.getNodeAs<clang::Decl>(CalledFuncFromVariadicFuncID);
    assert(D && "Bad Callback. No node provided");

    const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>(VariadicFuncCallerID);
    assert(CE && "Bad Callback. No node provided");

    // Make sure that the file containing the node is allowed to be modified
    if (!Owner.isFileModifiable(SM, VTF->getLocStart())) {
        return;
    }

    auto Policy = getPrintingPolicy();

    auto CF = dyn_cast<clang::FunctionDecl>(D);
    auto CTF = dyn_cast<clang::FunctionTemplateDecl>(D);

    // Get the callee as a function decl
    const FunctionDecl* F = nullptr;
    if (CF != nullptr) {
        F = CF;
    } else if (CTF != nullptr) {
        F = CTF->getAsFunction();
    } else {
        assert(false && "Called expression is not a function");
    }

    // Make really sure that the file containing the called node is allowed to be modified
    if (!Owner.isFileModifiable(SM, F->getLocStart())) {
        return;
    }

    // If the call doesn't contain the variadic parameter, we can ignore it
    std::string varargStr = "";
    bool varargFound = false;
    for (const auto& param : VTF->getAsFunction()->parameters()) {
        if (param->isParameterPack()) {
            varargStr = param->getName();
            varargFound = true;
        }
    }
    if (!varargFound) {
        return;
    }
    bool varargInCall = false;
    int count = 0;
    for (const auto& arg : CE->arguments()) {
        ++count;
        auto argStr = getSourceCode(SM, clang::CharSourceRange::getCharRange(arg->getSourceRange().getBegin(), getTokenEnd(SM, arg->getSourceRange().getEnd())));
        if (argStr.compare(varargStr) == 0) {
            varargInCall = true;
        }
    }
    if (!varargInCall) {
        // If the only argument of the function call is the variadic parameter pack
        // then when it's substituted as an empty pack, the call is considered to have no argument
        // So in this case a regex check is also necessary
        auto CEStr = getSourceCode(SM, getFullRange(SM, CE->getSourceRange()));
        llvm::Regex argRegex(varargStr + "[:space:]*\\.\\.\\.");
        if (!argRegex.match(CEStr)) {
            return;
        }
    }

    // Make sure the call is not to a specialisation of the variadic template
    auto PT = F->getPrimaryTemplate();
    TemplateTypeParmDecl* parameterPack = nullptr;
    if (PT != nullptr) {
        for (auto param : PT->getTemplateParameters()->asArray()) {
            if (param->isTemplateParameterPack()) {
                parameterPack = dyn_cast<TemplateTypeParmDecl>(param);
                if (parameterPack != nullptr) {
                    break;
                }
            }
        }
    }
    if (parameterPack != nullptr) {
        return;
    }
    
    // Create an alternate version of the function with every parameter on a variadic list and an extra parameter representing the variadic list end
    std::string s_calledFuncAlt;
    llvm::raw_string_ostream rso_calledFuncAlt(s_calledFuncAlt);
    rso_calledFuncAlt << "\n";
    clang::SourceLocation paramsBegin;
    clang::SourceLocation paramsEnd;
    std::string paramSubstitute = "";
    unsigned int nonVariadicFunctionArgumentCount = getNonVariadicVariableCount(VTF);
    // Analyse the function parameters for type, name and source location
    std::vector<FunctionParameterTypeInfo> functionParametersInfo;
    unsigned int argCount = 0;
    for (auto param : F->getAsFunction()->params()) {
        ++argCount;
        FunctionParameterTypeInfo parameterInfo;
        auto paramType = param->getType().getTypePtrOrNull();
        if (paramType != nullptr) {
            if (paramType->isDependentType()) {
                parameterInfo.isDependent = true;
            }

            if (argCount == nonVariadicFunctionArgumentCount + 1) {
                paramsBegin = param->getSourceRange().getBegin();
            }

            if (param->hasDefaultArg()) {
                paramsEnd = getTokenEnd(SM, param->getDefaultArg()->getSourceRange().getEnd());
            } else {
                paramsEnd = getTokenEnd(SM, param->getSourceRange().getEnd());
            }

            parameterInfo.typeClassName = paramType->getTypeClassName();

            // The correct unqualified type of the parameter is extracted from the source code
            parameterInfo.type = getSourceCode(SM, clang::CharSourceRange::getCharRange(param->getTypeSourceInfo()->getTypeLoc().getSourceRange()));
            if (parameterInfo.type.length() == 0) {
                parameterInfo.type = getSourceCode(SM, getFullRange(SM, param->getTypeSourceInfo()->getTypeLoc().getSourceRange()));
            }

            parameterInfo.name = param->getName();

            // A typeClassName of PackExpansion indicates the variadic function parameter
            if (parameterInfo.typeClassName.compare("PackExpansion") == 0) {
                parameterInfo.isParameterPack = true;
                std::string s_paramPackType;
                llvm::raw_string_ostream rso_paramPackType(s_paramPackType);
                param->getType().print(rso_paramPackType, Policy);
                std::string paramPackTypeStr = rso_paramPackType.str();
                if (paramPackTypeStr.find("const ") == 0) {
                    parameterInfo.isConst = true;
                }
                // The reference check is different because of the trailing ...
                if (paramPackTypeStr.at(paramPackTypeStr.size() - 4) == '&') {
                    parameterInfo.isReference = true;
                }
            }
            else {
                std::string s_paramType;
                llvm::raw_string_ostream rso_paramType(s_paramType);
                param->getType().print(rso_paramType, Policy);
                std::string paramTypeStr = rso_paramType.str();
                if (paramTypeStr.find("const ") == 0) {
                    parameterInfo.isConst = true;
                }
                if (paramTypeStr.at(paramTypeStr.size() - 1) == '&') {
                    parameterInfo.isReference = true;
                }
            }
        }
        functionParametersInfo.push_back(parameterInfo);
    }
    if (argCount == 0) {
        paramsBegin = F->getLocation().getLocWithOffset(F->getName().size() + 1);
    } else if (argCount < nonVariadicFunctionArgumentCount + 1) {
        paramsBegin = paramsEnd;
    }
    if (F->getPrimaryTemplate() != nullptr) {
        rso_calledFuncAlt << getSourceCode(SM, clang::CharSourceRange::getCharRange(F->getPrimaryTemplate()->getSourceRange().getBegin(), paramsBegin));
    } else {
        rso_calledFuncAlt << getSourceCode(SM, clang::CharSourceRange::getCharRange(F->getSourceRange().getBegin(), paramsBegin));
    }
    if ((argCount != 0) && (argCount < nonVariadicFunctionArgumentCount + 1)) {
        rso_calledFuncAlt << ", ";
    }

    bool parameterPackIsConst = false;
    bool parameterPackIsReference = false;
    for (const auto& param : VTF->getAsFunction()->parameters()) {
        std::string paramTypeClassName = param->getType()->getTypeClassName();
        if (paramTypeClassName.compare("PackExpansion") == 0) {
            std::string s_paramPackType;
            llvm::raw_string_ostream rso_paramPackType(s_paramPackType);
            param->getType().print(rso_paramPackType, Policy);
            std::string paramPackTypeStr = rso_paramPackType.str();
            if (paramPackTypeStr.find("const ") == 0) {
                parameterPackIsConst = true;
            }
            // The reference check is different because of the trailing ...
            if (paramPackTypeStr.at(paramPackTypeStr.size() - 4) == '&') {
                parameterPackIsReference = true;
            }
        }
    }
    std::string variadicParameterReplacement = "";
    if (parameterPackIsConst) {
        variadicParameterReplacement = "const ";
    }
    int unpackedParamteres = 0;

    if (argCount != 0) {
        argCount = 0;
        for (const auto& param : functionParametersInfo) {
            ++argCount;
            // Only take into account the parameters that are to be extracted from the variadic parameter pack
            if (argCount > nonVariadicFunctionArgumentCount) {
                variadicParameterReplacement += std::string("VariadicTemplateList< ") + param.type + std::string(", ");
                ++unpackedParamteres;
            }
        }
        variadicParameterReplacement += std::string("VariadicTemplateListEmpty");
        while (unpackedParamteres > 0) {
            variadicParameterReplacement += std::string(" >");
            --unpackedParamteres;
        }
    } else {
        variadicParameterReplacement += "VariadicTemplateListEmpty";
    }

    if (parameterPackIsReference) {
        variadicParameterReplacement += "&";
    }
    variadicParameterReplacement += " variadicTemplateParameterPackSubstitute";

    rso_calledFuncAlt << variadicParameterReplacement << ")";

    // If the function's return type is defined with auto, append the proper decltype
    auto retType = F->getReturnType().getAsString();
    if (retType.find("decltype", 0) == 0) {
        // TODO : This isn't really possible. The values used in the decltype probably don't exist after the conversion
        // The following is experimental and only works with specific conditions (the variable names must not be a substring of anything else in the text)
        // Also the generated code can't be transformed by Modify Auto
        std::string newRetType = retType;

        argCount = 0;
        std::string substitutionDepth = "";
        for (auto param : functionParametersInfo) {
            ++argCount;
            if (argCount > nonVariadicFunctionArgumentCount) {
                if (param.name.length() != 0) {
                    std::string nameSubstitution = "variadicTemplateParameterPackSubstitute" + substitutionDepth + ".value";
                    ReplaceAll(newRetType, param.name, nameSubstitution);
                }
                substitutionDepth += ".next";
            }
        }

        rso_calledFuncAlt << " -> " << newRetType;
    }

    if (D->getAsFunction()->isThisDeclarationADefinition()) {
        // Generate aliases for the parameters that we'll substitute, so that they can be reffered to with their original name
        std::string variadicNameSubstitution = "\n";
        std::string substitutionDepth = "";
        argCount = 0;
        for (auto param : functionParametersInfo) {
            ++argCount;
            if (argCount > nonVariadicFunctionArgumentCount) {
                if (param.name.length() != 0) {
                    variadicNameSubstitution += (param.isConst ? std::string("const ") : std::string("")) + param.type + "& " + param.name + " = variadicTemplateParameterPackSubstitute" + substitutionDepth + ".value;\n";
                }
                substitutionDepth += ".next";
            }
        }

        rso_calledFuncAlt << " {\n" << variadicNameSubstitution << "\n";

        rso_calledFuncAlt << getSourceCode(SM, clang::CharSourceRange::getCharRange(F->getBody()->getSourceRange().getBegin().getLocWithOffset(1), getTokenEnd(SM, F->getBody()->getSourceRange().getEnd())));
        rso_calledFuncAlt << "\n";
    } else {
        rso_calledFuncAlt << ";\n";
    }

    auto calledFuncAltRange = makeInsertionPoint(F->getSourceRange().getEnd().getLocWithOffset(1));

    Owner.addTracedReplacement(SM, calledFuncAltRange, rso_calledFuncAlt.str());

    //llvm::outs() << "3 - " << ++changes << "\n";
    ++AcceptedChanges;
}

// Find the number of non variadic parameters
template<>
void NonVariadicParamNumFinder::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const FunctionTemplateDecl *VTF = Result.Nodes.getNodeAs<clang::FunctionTemplateDecl>(NumCountVariadicTemplateWithCallExprID);
    assert(VTF && "Bad Callback. No node provided");

    const Decl *D = Result.Nodes.getNodeAs<clang::Decl>(NumCountCalledFuncFromVariadicFuncID);
    assert(D && "Bad Callback. No node provided");

    const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>(NumCountFuncCallerID);
    assert(CE && "Bad Callback. No node provided");

    // Make sure that the file containing the node is allowed to be modified
    if (!Owner.isFileModifiable(SM, VTF->getLocStart())) {
        return;
    }

    auto Policy = getPrintingPolicy();

    auto CF = dyn_cast<clang::FunctionDecl>(D);
    auto CTF = dyn_cast<clang::FunctionTemplateDecl>(D);

    // Get the callee as a function decl
    const FunctionDecl* F = nullptr;
    if (CF != nullptr) {
        F = CF;
    }
    else if (CTF != nullptr) {
        F = CTF->getAsFunction();
    }
    else {
        assert(false && "Called expression is not a function");
    }

    // Store the minimum number of variables as the number for the non variadic variables
    // This only works if this match happens before the one that modifies the functions, and that should be the case
    std::string varargStr = "";
    bool varargFound = false;
    for (const auto& param : VTF->getAsFunction()->parameters()) {
        if (param->isParameterPack()) {
            varargStr = param->getName();
            varargFound = true;
        }
    }
    unsigned int varCount = 0;
    for (const auto& arg : CE->arguments()) {
        auto argStr = getSourceCode(SM, clang::CharSourceRange::getCharRange(arg->getSourceRange().getBegin(), getTokenEnd(SM, arg->getSourceRange().getEnd())));
        if ((!varargFound) || (argStr.compare(varargStr) != 0)) {
            ++varCount;
        }
    }
    std::string funcName = F->getDeclName().getAsString();
    if (NonVariadicVariableCountMap.count(funcName) == 0) {
        NonVariadicVariableCountMap[funcName] = varCount;
    }
    else {
        if (NonVariadicVariableCountMap[funcName] > varCount) {
            NonVariadicVariableCountMap[funcName] = varCount;
        }
    }

    //llvm::outs() << "4 - " << ++changes << "\n";
    ++AcceptedChanges;
}

/*

const std::string ModifyVariadicID = "ModifyVariadic";

struct ModifyVariadicFactory : TransformFactory {
    ModifyVariadicFactory() {
        Since.Clang = Version(2, 9);
        Since.Gcc = Version(4, 4);
        Since.Icc = Version(12);
        Since.Msvc = Version(10);
    }

    Transform *createTransform(const TransformOptions &Opts) override {
        TransformHandler *handler = new TransformHandler(ModifyVariadicID, Opts, TransformPriorities::MODIFY_VARIADIC);
        auto &acc = handler->getAcceptedChanges();
        handler->initReplacers(
            new NonVariadicParamNumFinder(handler->getAcceptedChanges(), *handler), makeVariadicFinalFunctionVarCountMatcher,
            new VariadicFuncCallReplacer(handler->getAcceptedChanges(), *handler), makeVariadicFuncCallMatcher,
            new VariadicFuncFinalCallReplacer(handler->getAcceptedChanges(), *handler), makeVariadicFinalFunctionMatcher,
            new VariadicTemplateFunctionModifier(handler->getAcceptedChanges(), *handler), makeVariadicTemplateFunctionMatcher
            );
        return handler;
    }
};

// Register the factory using this statically initialized variable.
static TransformFactoryRegistry::Add<ModifyVariadicFactory>
X("modify-variadic", "Modifies variadic functions");

*/
