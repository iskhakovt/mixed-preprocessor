// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#ifndef MIXED_PREPROCESSOR_FRONTENDACTIONS_HPP
#define MIXED_PREPROCESSOR_FRONTENDACTIONS_HPP


#include "clang/Frontend/FrontendAction.h"


class MixedPrintPreprocessedAction : public clang::PreprocessorFrontendAction {
protected:
  void ExecuteAction() override;

  bool hasPCHSupport() const override { return true; }
};

#endif //MIXED_PREPROCESSOR_FRONTENDACTIONS_HPP
