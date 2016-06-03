// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "FrontendActions.hpp"
#include "MixedComputations.hpp"

#include "clang/Frontend/CompilerInstance.h"

using namespace clang;


void DoMixedPrintPreprocessedInput(Preprocessor &PP, raw_ostream *OS,
                                   const PreprocessorOutputOptions &Opts) {
    MixedComputations MC(PP);
    Token Tok;

    do {
        PP.LexUnexpandedNonComment(Tok);
        ArrayRef<Token> expandedTokens = MC.expandToken(Tok);

        for (auto tokenIt = expandedTokens.begin(); tokenIt != expandedTokens.end(); ++tokenIt) {
            *OS << tok::getTokenName(Tok.getKind()) << " '" << PP.getSpelling(Tok) << "'\n";
        }
    } while (Tok.isNot(tok::eof));
}

void MixedPrintPreprocessedAction::ExecuteAction() {
    CompilerInstance &CI = getCompilerInstance();
    // Output file may need to be set to 'Binary', to avoid converting Unix style
    // line feeds (<LF>) to Microsoft style line feeds (<CR><LF>).
    //
    // Look to see what type of line endings the file uses. If there's a
    // CRLF, then we won't open the file up in binary mode. If there is
    // just an LF or CR, then we will open the file up in binary mode.
    // In this fashion, the output format should match the input format, unless
    // the input format has inconsistent line endings.
    //
    // This should be a relatively fast operation since most files won't have
    // all of their source code on a single line. However, that is still a
    // concern, so if we scan for too long, we'll just assume the file should
    // be opened in binary mode.
    bool BinaryMode = true;
    bool InvalidFile = false;
    const SourceManager& SM = CI.getSourceManager();
    const llvm::MemoryBuffer *Buffer = SM.getBuffer(SM.getMainFileID(),
                                                    &InvalidFile);
    if (!InvalidFile) {
        const char *cur = Buffer->getBufferStart();
        const char *end = Buffer->getBufferEnd();
        const char *next = (cur != end) ? cur + 1 : end;

        // Limit ourselves to only scanning 256 characters into the source
        // file.  This is mostly a sanity check in case the file has no
        // newlines whatsoever.
        if (end - cur > 256) end = cur + 256;

        while (next < end) {
            if (*cur == 0x0D) {  // CR
                if (*next == 0x0A)  // CRLF
                    BinaryMode = false;

                break;
            } else if (*cur == 0x0A)  // LF
                break;

            ++cur;
            ++next;
        }
    }

    raw_ostream *OS = CI.createDefaultOutputFile(BinaryMode, getCurrentFile());
    if (!OS) return;

    DoMixedPrintPreprocessedInput(CI.getPreprocessor(), OS,
                                  CI.getPreprocessorOutputOpts());
}
