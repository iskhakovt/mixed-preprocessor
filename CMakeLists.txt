# Copyright (c) Timur Iskhakov.
# Distributed under the terms of the GNU GPL v3 License.


cmake_minimum_required(VERSION 3.5)
project(mixed-preprocessor)

set(LLVM_DIR ../llvm)
set(BUILD_DIR ../build)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -fPIC -fvisibility-inlines-hidden -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcast-qual -Wmissing-field-initializers -pedantic -Wno-long-long -Wcovered-switch-default -Wnon-virtual-dtor -Wdelete-non-virtual-dtor -std=c++11 -g -fno-exceptions -fno-rtti")
set(LINK_SETTINGS "-Wl,-search_paths_first -Wl,-headerpad_max_install_names")
set(LLVM_LIBS LLVMOption LLVMTableGen LLVMX86Disassembler LLVMX86AsmParser LLVMX86CodeGen LLVMSelectionDAG LLVMAsmPrinter LLVMX86Desc LLVMMCDisassembler LLVMX86Info LLVMX86AsmPrinter LLVMX86Utils LLVMipo LLVMVectorize LLVMLinker LLVMIRReader LLVMAsmParser LLVMCodeGen LLVMScalarOpts LLVMInstCombine LLVMInstrumentation LLVMProfileData LLVMBitWriter LLVMTransformUtils LLVMTarget LLVMAnalysis LLVMObject LLVMMCParser LLVMBitReader LLVMMC LLVMCore LLVMSupport curses pthread z m)
set(CLANG_LIBS clangFrontend clangSerialization clangDriver clangParse clangSema clangAnalysis clangAST clangBasic clangEdit clangLex clangTooling)
set(LLVM_DEFINITIONS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)

add_subdirectory(MixedPreprocessor)
add_subdirectory(MixedPreprocessorInvocation)
