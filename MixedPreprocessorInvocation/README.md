# Invocation

Unfortunately, there is no graceful way to implement a different Preprocessor or TokenLexer class in clang
(only using PPCallbacks).

Thus, this implementation uses MixedComputations, which is mostly like TokenLexer, but not integrated in Preprocessor.
