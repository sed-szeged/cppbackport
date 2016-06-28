#include "RemoveAttributesMatchers.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "Util/Log.h"

using namespace backport::helper;
using namespace clang::ast_matchers;
using namespace clang::tooling;

using AttributesReplacer = ReplacerTemplate<class AttributesReplace>;

namespace {
}

/**
* \brief This function removes override and final specifiers.
*       TODO: Implement attribute parsing for unknown attributes as a special attribute type like Unknown.
*/
template<>
void AttributesReplacer::run(const MatchFinder::MatchResult &Result) {
    const Decl *D{ Result.Nodes.getNodeAs<Decl>(RemoveAttributesMatcherId) };
    assert(D && "Bad Callback. No node provided");

    SourceManager &SM = *Result.SourceManager;
    if (!Owner.isFileModifiable(SM, D->getLocStart())) {
        return;
    }

    for (const auto& attr : D->attrs()) {
        auto attrKind = attr->getKind();
        bool replacementReady = false;
        bool error = false;
        clang::CharSourceRange replacementRange;
        std::string replacementText = "";
        if (!attr->isImplicit() && !attr->isInherited()) {
            if (attrKind == clang::attr::Kind::Override) {
                replacementRange = getFullRange(SM, attr->getRange());
                replacementText = "/* override */";
                replacementReady = true;
            }
            else if (attrKind == clang::attr::Kind::Final) {
                replacementRange = getFullRange(SM, attr->getRange());
                replacementText = "/* final */";
                replacementReady = true;
            }
            else if ((attrKind == clang::attr::Kind::Unused) || (attrKind == clang::attr::Kind::CXX11NoReturn) || (attrKind == clang::attr::Kind::CarriesDependency)) {
                // The attribute range does not include the enclosing "[[" and "]]" so we'll search for those in the code
                // NOTE : While the standard would allow for whitespace and comments to be between the two "[" or "]", we'll only look for "[[" and "]]" specifically
                error = false;
                auto range = attr->getRange();

                auto begin = range.getBegin();
                auto end = range.getEnd();

                if (backport::helper::equal(SM, begin, end) && (getTokenEnd(SM, end).isInvalid() || backport::helper::equal(SM, begin, getTokenEnd(SM, end)))) {
                    LOG(logDEBUG) << "Attribute with zero length.";
                    continue;
                }

                if (range.isInvalid()) {
                    LOG(logWARNING) << "Attribute range invalid in " << Owner.getCurrentSource();
                    continue;
                }

                const auto currentAttrFileID = SM.getFileID(begin);

                bool stop = false;
                begin = begin.getLocWithOffset(1);
                while (!stop) {
                    begin = begin.getLocWithOffset(-1);
                    if (SM.getFileID(begin) != currentAttrFileID) {
                        LOG(logWARNING) << "Attribute begin [[ can't be found in " << SM.getFilename(range.getBegin());
                        error = true;
                        break;
                    }
                    if (SM.getCharacterData(begin)[0] == '[') {
                        begin = begin.getLocWithOffset(-1);
                        if (SM.getCharacterData(begin)[0] == '[') {
                            stop = true;
                        }
                    }
                }

                if (error) {
                    continue;
                }

                stop = false;
                end = end.getLocWithOffset(-1);
                while (!stop) {
                    end = end.getLocWithOffset(1);
                    if (SM.getFileID(end) != currentAttrFileID) {
                        LOG(logWARNING) << "Attribute end ]] can't be found in " << SM.getFilename(range.getBegin());
                        error = true;
                        break;
                    }
                    if (SM.getCharacterData(end)[0] == ']') {
                        end = end.getLocWithOffset(1);
                        if (SM.getCharacterData(end)[0] == ']') {
                            stop = true;
                        }
                    }
                }
                end = end.getLocWithOffset(1);

                if (error) {
                    continue;
                }

                auto newRange = CharSourceRange::getCharRange(begin, end);
                auto attrCode = getSourceCode(SM, newRange);
                ReplaceAll(attrCode, std::string("/*"), std::string("//"));
                ReplaceAll(attrCode, std::string("*/"), std::string("//"));
                attrCode = std::string("/*") + attrCode + std::string("*/");

                replacementRange = newRange;
                std::string replacementText = attrCode;
                replacementReady = true;
            }
        }
        if (replacementReady) {
            if (replacementRange.isValid()) {
                if (Owner.isFileModifiable(SM, replacementRange.getBegin()) &&
                    Owner.isFileModifiable(SM, replacementRange.getEnd()) &&
                    SM.getFileID(replacementRange.getBegin()) == SM.getFileID(replacementRange.getEnd()))
                {
                    Owner.addTracedReplacement(SM, replacementRange, replacementText);
                    ++AcceptedChanges;
                } else {
                    LOG(logDEBUG) << "Attributes should be in modifiable files. (This is not an error)";
                    LOG(logDEBUG) << "replacementText: " << replacementText;
                    LOG(logDEBUG) << "in file: ";
                    LOG(logDEBUG) << "from: " << SM.getFilename(replacementRange.getBegin()) << ":" << SM.getSpellingLineNumber(replacementRange.getBegin()) << ":" << SM.getSpellingColumnNumber(replacementRange.getBegin());
                    LOG(logDEBUG) << "to: " << SM.getFilename(replacementRange.getEnd()) << ":" << SM.getSpellingLineNumber(replacementRange.getEnd()) << ":" << SM.getSpellingColumnNumber(replacementRange.getEnd());
                    LOG(logDEBUG) << "the replacementRange starts and ends in the same file: " <<
                        ((SM.getFileID(replacementRange.getBegin()) == SM.getFileID(replacementRange.getEnd())) ? "true" : "false");
                }
            } else {
                LOG(logWARNING) << "Attribute replacement | " << replacementText << " | generated with invalid range in " << Owner.getCurrentSource();
            }
        }
    }
}
