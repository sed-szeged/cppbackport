//===-- Core/Transforms.cpp - class Transforms Impl -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file provides the implementation for class Transforms.
///
//===----------------------------------------------------------------------===//

#include "TransformBase/Transforms.h"
#include "TransformBase/TransformHandler.h"
#include <queue>

namespace cl = llvm::cl;

cl::OptionCategory TransformCategory("Transforms");

struct CompareTransform {
    bool operator()(const TransformHandler* lhs, const TransformHandler* rhs) {
        return lhs->getPriority() < rhs->getPriority();
    }
};

Transforms::~Transforms() {
    for (std::vector<Transform *>::iterator I = ChosenTransforms.begin(),
        E = ChosenTransforms.end();
        I != E; ++I)
        delete *I;

    for (OptionMap::iterator I = Options.begin(), E = Options.end(); I != E; ++I)
        delete I->getValue();
}

void Transforms::registerTransforms() {
    for (TransformFactoryRegistry::iterator I = TransformFactoryRegistry::begin(),
        E = TransformFactoryRegistry::end();
        I != E; ++I)
        Options[I->getName()] = new cl::opt<bool>(
        I->getName(), cl::desc(I->getDesc()), cl::cat(TransformCategory));
}

bool Transforms::hasAnyExplicitOption() const {
    for (OptionMap::const_iterator I = Options.begin(), E = Options.end(); I != E;
        ++I)
    if (*I->second)
        return true;
    return false;
}

void
Transforms::createSelectedTransforms(const TransformOptions &GlobalOptions) {
    // if at least one transform is set explicitly on the command line, do not
    // enable non-explicit ones
    bool EnableAllTransformsByDefault = !hasAnyExplicitOption();
    bool NoRequiredVersions = GlobalOptions.TargetVersions.Clang.isNull() && GlobalOptions.TargetVersions.Gcc.isNull() && GlobalOptions.TargetVersions.Icc.isNull() && GlobalOptions.TargetVersions.Msvc.isNull();
    std::priority_queue<TransformHandler*, std::vector<TransformHandler*>, CompareTransform> TrafoPrio;

    for (TransformFactoryRegistry::iterator I = TransformFactoryRegistry::begin(),
        E = TransformFactoryRegistry::end();
        I != E; ++I) {
        bool ExplicitlyEnabled = *Options[I->getName()];
        bool OptionEnabled = EnableAllTransformsByDefault || ExplicitlyEnabled;

        if (!OptionEnabled)
            continue;

        std::unique_ptr<TransformFactory> Factory(I->instantiate());
        if (NoRequiredVersions || !Factory->supportsCompilers(GlobalOptions.TargetVersions))
            TrafoPrio.push(static_cast<TransformHandler*>(Factory->createTransform(GlobalOptions)));
        else if (ExplicitlyEnabled)
            llvm::errs() << "note: " << '-' << I->getName()
            << ": transform not needed for specified compilers\n";
    }

    while (!TrafoPrio.empty()) {
        ChosenTransforms.push_back(TrafoPrio.top());
        TrafoPrio.pop();
    }
}
