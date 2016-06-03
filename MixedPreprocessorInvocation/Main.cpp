// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "FrontendActions.hpp"

#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"


static llvm::cl::OptionCategory MixedToolCategory("Preprocessor options");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

int main(int argc, const char **argv) {
    clang::tooling::CommonOptionsParser op(argc, argv, MixedToolCategory);
    clang::tooling::ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    int result = Tool.run(clang::tooling::newFrontendActionFactory<MixedPrintPreprocessedAction>().get());

    return result;
}
