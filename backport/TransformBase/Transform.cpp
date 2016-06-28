//===-- Core/Transform.cpp - Transform Base Class Def'n -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file provides the definition for the base Transform class from
/// which all transforms must subclass.
///
//===----------------------------------------------------------------------===//

#include "TransformBase/Transform.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/STLExtras.h"

#include "Util/TransformUtility.h"
#include "Util/PathUtility.h"
#include "Util/Log.h"
#include "Util/ReplacementData.h"

std::map< std::string, std::set< backport::helper::Path > > Transform::TransformationSourceMap;

template class llvm::Registry<TransformFactory>;

using namespace clang;

llvm::cl::OptionCategory TransformsOptionsCategory("Transforms' options");

namespace {

using namespace tooling;
using namespace ast_matchers;

/// \brief Custom FrontendActionFactory to produce FrontendActions that simply
/// forward (Begin|End)SourceFileAction calls to a given Transform.
class ActionFactory : public clang::tooling::FrontendActionFactory {
public:
  ActionFactory(MatchFinder &Finder, Transform &Owner)
  : Finder(Finder), Owner(Owner) {}

  FrontendAction *create() override {
    return new FactoryAdaptor(Finder, Owner);
  }

private:
  class FactoryAdaptor : public ASTFrontendAction {
  public:
    FactoryAdaptor(MatchFinder &Finder, Transform &Owner)
        : Finder(Finder), Owner(Owner) {}

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &,
                                                   StringRef) override {
      return Finder.newASTConsumer();
    }

    virtual bool BeginSourceFileAction(CompilerInstance &CI,
                                       StringRef Filename) override {
      if (!ASTFrontendAction::BeginSourceFileAction(CI, Filename))
        return false;

      return Owner.handleBeginSource(CI, Filename);
    }

    void EndSourceFileAction() override {
      Owner.handleEndSource();
      return ASTFrontendAction::EndSourceFileAction();
    }

  private:
    MatchFinder &Finder;
    Transform &Owner;
  };

  MatchFinder &Finder;
  Transform &Owner;
};
} // namespace

Transform::Transform(llvm::StringRef Name, const TransformOptions &Options)
    : Name(Name), GlobalOptions(Options) {
  Reset();
}

Transform::~Transform() {}

bool Transform::isFileModifiable(const SourceManager &SM,
                                 const SourceLocation &Loc) const {
  if (SM.isWrittenInMainFile(Loc))
    return true;

  const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(Loc));
  if (!FE)
    return false;

  return GlobalOptions.ModifiableFiles.isFileIncluded(FE->getName());
}

CompilerVersions Transform::getTargetCompilerVersions() const {
    return GlobalOptions.TargetVersions;
}

bool Transform::handleBeginSource(CompilerInstance &CI, StringRef Filename) {
  CurrentSource = Filename;

  if (Options().EnableTiming) {
    Timings.push_back(std::make_pair(Filename.str(), llvm::TimeRecord()));
    Timings.back().second -= llvm::TimeRecord::getCurrentTime(true);
  }
  return true;
}

void Transform::handleEndSource() {
  CurrentSource.clear();
  if (Options().EnableTiming)
    Timings.back().second += llvm::TimeRecord::getCurrentTime(false);
}

void Transform::addTiming(llvm::StringRef Label, llvm::TimeRecord Duration) {
  Timings.push_back(std::make_pair(Label.str(), Duration));
}

void Transform::addTracedReplacement(const clang::SourceManager& SM, const clang::CharSourceRange& range, const std::string& replacement, int originalLineNumber) {
    auto printRange = [&]() {
        if (range.getBegin().isValid()) {
            LOG(logERROR) << "Startlocation: " << range.getBegin().printToString(SM);
        }
        if (range.getEnd().isValid()) {
            LOG(logERROR) << "Endlocation: " << range.getEnd().printToString(SM);
        }
        LOG(logERROR) << "REPLACEMENT: " << replacement;
    };

    if (range.isInvalid()) {
        LOG(logERROR) << "Invalid replacement during " << getName() << ". Replacement text: " << replacement;
        printRange();
        exit(1);
    }

    assert(::backport::helper::less(SM, range.getBegin(), range.getEnd()) || ::backport::helper::equal(SM, range.getBegin(), range.getEnd()));
    if (SM.getFileID(range.getBegin()) != SM.getFileID(range.getEnd())) {
        LOG(logERROR) << "Transfromtaion starts in one file (" << SM.getFilename(range.getBegin()) << ") but ends in another (" << SM.getFilename(range.getEnd()) << ") during " << getName();
        printRange();
        exit(1);
    }
    else if (!isFileModifiable(SM, range.getBegin())) {
        LOG(logERROR) << "Restricted file (" << SM.getFilename(range.getBegin()) << ") modification attempted during " << getName();
        printRange();
        exit(1);
    }

    int const startLine = SM.getSpellingLineNumber(range.getBegin());
    int const origLines = SM.getSpellingLineNumber(range.getEnd()) - startLine;
    int const repLines = std::count(replacement.begin(), replacement.end(), '\n');
    int const lineChange = repLines - origLines;

    auto backportMgr = Options().BackportMgr;

    if (backportMgr->useDB()) {
        // Get the filePath.
        std::string filePath;
        {
            auto fileE = SM.getFileEntryForID(SM.getFileID(range.getBegin()));
            filePath = fileE->getName();
        }

        std::string fromPath = filePath;

        int comingFrom = -1;
        if (originalLineNumber == -1) {
            comingFrom = startLine;
        } else {
            comingFrom = originalLineNumber;
        }

        this->replacements.insert(backport::ReplacementData(-1, filePath, -1 /*We don't actually know this yet*/, 
            -1 /*not yet registered in the database*/, startLine, origLines, repLines, -1, filePath, comingFrom, comingFrom, true, replacement));

        this->modifiedFilesCurrentLength[filePath] = SM.getSpellingLineNumber(SM.getLocForEndOfFile(SM.getFileID(range.getBegin())));
    }

    addReplacementForCurrentTU(Replacement(SM, range, replacement));
}

void Transform::addTracedReplacement(const clang::SourceManager& SM, const clang::SourceRange& range, const std::string& replacement) {
    addTracedReplacement(SM, clang::CharSourceRange::getCharRange(range.getBegin(), range.getEnd()), replacement);
}

bool
Transform::addReplacementForCurrentTU(const clang::tooling::Replacement &R) {
  if (CurrentSource.empty())
    return false;

  TranslationUnitReplacements &TU = Replacements[CurrentSource];
  if (TU.MainSourceFile.empty())
    TU.MainSourceFile = CurrentSource;
  TU.Replacements.push_back(R);

  return true;
}

std::unique_ptr<FrontendActionFactory>
Transform::createActionFactory(MatchFinder &Finder) {
  return llvm::make_unique<ActionFactory>(Finder, /*Owner=*/*this);
}

Version Version::getFromString(llvm::StringRef VersionStr) {
  llvm::StringRef MajorStr, MinorStr;
  Version V;

  std::tie(MajorStr, MinorStr) = VersionStr.split('.');
  if (!MinorStr.empty()) {
    llvm::StringRef Ignore;
    std::tie(MinorStr, Ignore) = MinorStr.split('.');
    if (MinorStr.getAsInteger(10, V.Minor))
      return Version();
  }
  if (MajorStr.getAsInteger(10, V.Major))
    return Version();
  return V;
}

TransformFactory::~TransformFactory() {}

namespace {
bool versionSupported(Version Required, Version AvailableSince, bool isNullTrue) {
  // null version, means no requirements, means supported
  if (Required.isNull())
    return isNullTrue ? true : false;
  return Required >= AvailableSince;
}
} // end anonymous namespace

bool TransformFactory::supportsCompilers(CompilerVersions Required) const {
    return supportsCompilers(Required, Since);
}

bool TransformFactory::supportsCompilers(CompilerVersions Target, CompilerVersions Available, bool isNullTrue) const {
    return versionSupported(Target.Clang, Available.Clang, isNullTrue) &&
        versionSupported(Target.Gcc, Available.Gcc, isNullTrue) &&
        versionSupported(Target.Icc, Available.Icc, isNullTrue) &&
        versionSupported(Target.Msvc, Available.Msvc, isNullTrue);
}
