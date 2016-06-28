#include "TransformBase/TransformHandler.h"
#include "Util/TransformUtility.h"
#include "Util/StringUtility.h"
#include "ReplaceForRangeMatchers.h"

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;

typedef ReplacerTemplate<class ForRange> ForRangeReplacer;

// Necessary to differentiate replacements because nested for-range statements without body {} end at the same location
static unsigned int counter = 1;

// Expand for-range statements
template<>
void ForRangeReplacer::run(const MatchFinder::MatchResult &Result) {
    SourceManager &SM = *Result.SourceManager;

    const CXXForRangeStmt *S = Result.Nodes.getNodeAs<CXXForRangeStmt>(ForRangeStmtID);
    assert(S && "Bad Callback. No node provided");

    if (!Owner.isFileModifiable(SM, S->getLocStart())) {
        return;
    }

    // If the container is an unknown template type, clang can't determine the proper initialization statement
    // I.E.: It could be either t.begin() or begin(t), but it can't be determined
    if (S->getBeginEndStmt() == nullptr) {
        return;
    }

    auto Policy = getPrintingPolicy();

    // Stream for the new for-range statement
    std::string s_forRange;
    llvm::raw_string_ostream rso_forRange(s_forRange);

    // Stream where the __begin and __end variable initialization will constructed
    std::string s_beginEnd;
    llvm::raw_string_ostream rso_beginEnd(s_beginEnd);

    // Stream for the range initialization expression
    std::string s_rangeInit;
    llvm::raw_string_ostream rso_rangeInit(s_rangeInit);

    // We add an extra block around the range-for statement because we'll be defining a few variables before
    // This helps to further avoid any problems with redefinitions
    rso_forRange << "{\n";

    // Get the range initialization expression
    S->getRangeInit()->printPretty(rso_rangeInit, 0, Policy);
    
    // The DeclStmt returned by getBeginEndStmt should contain exactly two declarations
    // One for the __begin and one for the __end iterator
    bool first = true;
    for (const auto& decl : S->getBeginEndStmt()->decls()) {
        auto varDecl = dyn_cast<VarDecl>(decl);
        if (varDecl != nullptr) {
            // Instead of printing the declarations as-is, we'll instead do it ourselves and deduce the proper type of the iterators
            rso_beginEnd << varDecl->getType().getAsString(Policy) << ((first) ? std::string(" __begin = ") : std::string(" __end = "));
            first = false;

            // We'll need the right hand side of the declaration
            std::string s_init;
            llvm::raw_string_ostream rso_init(s_init);
            varDecl->getInit()->printPretty(rso_init, 0, Policy);

            // The declaration clang generates are in one of the following forms :
            //   auto && __begin = __range.begin();
            //   auto && __begin = begin(__range);
            // We'll take the right hand side, and replace the '__range' with the expression clang would use to initialize it
            std::string tmp = rso_init.str();
            ReplaceAll(tmp, std::string("__range"), std::string("(") + rso_rangeInit.str() + std::string(")"));
            rso_beginEnd << tmp << ";\n";
        }
    }

    // Instead of the rage-for statement we'll have a simple for statement
    // Since we'll do the initialization of the iterators beforehand, we'll skip the initialization part of the for statement
    rso_forRange << rso_beginEnd.str() << "for(;";

    // The condition will be the same clang would use internally for the range-for statement : '__begin != __end'
    S->getCond()->printPretty(rso_forRange, 0, Policy);
    rso_forRange << "; ";

    // The loop expression is again taken from the internal representation : '++ __begin'
    S->getInc()->printPretty(rso_forRange, 0, Policy);
    rso_forRange << ") {\n";

    // We'll initialize the loop variable used in the original range-for statement with the value of '*__begin'
    // When getting the type of the loop variable, we'll also get the proper qualifiers if there were any
    QualType loopVarType;
    for (const auto& decl : S->getLoopVarStmt()->decls()) {
        auto varDecl = dyn_cast<VarDecl>(decl);
        if (varDecl != nullptr) {
            loopVarType = varDecl->getType();
        }
    }
    if (loopVarType->isFunctionPointerType()) {
        rso_forRange << backport::helper::rebuildFunctionPointerType(loopVarType, S->getLoopVariable()->getName());
    } else {
        rso_forRange << loopVarType.getAsString(Policy) << std::string(" ") << std::string(S->getLoopVariable()->getNameAsString());
    }
    rso_forRange << std::string(" = *__begin;") << "\n";

    // The generated for statement will replace the original range-for statement, but we'll leave the body alone
    CharSourceRange statementRange = CharSourceRange::getCharRange(S->getSourceRange().getBegin(), S->getBody()->getSourceRange().getBegin());

    // In total we added two new blocks that we'll need to close after the range-for statement
    // This won't affect visibility, since we'll only put extra blocks around the range-for statement
    CharSourceRange endRange = makeInsertionPoint(getTokenEnd(SM, S->getSourceRange().getEnd()));

    // Hack for clang bug
    if ( endRange.isInvalid() ||
         statementRange.isInvalid() ||
         (SM.getFileID(endRange.getEnd()) != SM.getFileID(S->getSourceRange().getBegin())) ||
         (SM.getFileID(statementRange.getEnd()) != SM.getFileID(S->getSourceRange().getBegin())) ) {
        LOG(logWARNING) << "Invalid range for statement range, probably due to clang bug when no {} are given for for-range statements";

        unsigned int offset = getLengthFromLocationUntilSeparator(SM, S->getSourceRange().getBegin());
        endRange = makeInsertionPoint(S->getSourceRange().getBegin().getLocWithOffset(offset + 1));

        // hope that the part after the : does not contain ')' or else we are out of luck
        offset = getLengthFromLocationUntilClosingParentheses(SM, S->getSourceRange().getBegin());
        statementRange = CharSourceRange::getCharRange(statementRange.getBegin(), S->getSourceRange().getBegin().getLocWithOffset(offset + 1));
    }

    // Hack for clang bug, a different case
    CharSourceRange extendedEndRange = CharSourceRange::getCharRange(endRange.getBegin(), endRange.getBegin().getLocWithOffset(1));
    if (extendedEndRange.isValid()) {
        std::string lastChar = getSourceCode(SM, extendedEndRange);
        if (lastChar.compare(std::string(";")) == 0) {
            endRange = makeInsertionPoint(endRange.getBegin().getLocWithOffset(1));
        }
    }

    Owner.addTracedReplacement(SM, statementRange, rso_forRange.str());
    
    Owner.addTracedReplacement(SM, endRange, std::string("\n}\n}\n/* ") + std::to_string(counter) + std::string(" */"));
    ++counter;

    ++AcceptedChanges;
}
