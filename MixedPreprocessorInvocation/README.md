# Invocation

Unfortunately, there is no graceful way to implement a different Preprocessor class in clang
(only using PPCallbacks or outside the preprocessor).

Thus, this implementation needs to reimplement some classes to have a way to use the MixedPreprocessor.
They behave in the very same way as original, but use MixedPreprocessor instead of Preprocessor.
