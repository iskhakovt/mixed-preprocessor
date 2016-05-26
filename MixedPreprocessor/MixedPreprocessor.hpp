// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#ifndef MIXED_PREPROCESSOR_MIXEDPREPROCESSOR_HPP
#define MIXED_PREPROCESSOR_MIXEDPREPROCESSOR_HPP


#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PreprocessorOptions.h"

using namespace clang;


class MixedPreprocessor : public Preprocessor {
public:
    MixedPreprocessor(IntrusiveRefCntPtr<PreprocessorOptions> PPOpts,
                      DiagnosticsEngine &diags, LangOptions &opts,
                      SourceManager &SM, HeaderSearch &Headers,
                      ModuleLoader &TheModuleLoader,
                      IdentifierInfoLookup *IILookup, bool OwnsHeaders,
                      TranslationUnitKind TUKind)
            : Preprocessor(PPOpts, diags, opts, SM, Headers, TheModuleLoader, IILookup, OwnsHeaders, TUKind) {}
};


#endif //MIXED_PREPROCESSOR_MIXEDPREPROCESSOR_HPP
