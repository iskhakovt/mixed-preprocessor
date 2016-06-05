# Copyright (c) Timur Iskhakov.
# Distributed under the terms of the GNU GPL v3 License.


cmake_minimum_required(VERSION 3.5)

include_directories(../${LLVM_DIR}/include)
include_directories(../${LLVM_DIR}/tools/clang/include)
include_directories(../${BUILD_DIR}/include)
include_directories(../${BUILD_DIR}/tools/clang/include)

link_directories(../${BUILD_DIR}/lib)
link_directories(../${BUILD_DIR}/tools/clang/lib)

add_definitions(${LLVM_DEFINITIONS})

add_library(mixed-preprocessor-core STATIC
       MixedComputations.cpp MixedComputationsPPCallbacks.cpp MacroDependency.cpp MacroExpansion.cpp)

target_link_libraries(mixed-preprocessor-core
        ${LINK_SETTINGS} ${LLVM_LIBS} ${CLANG_LIBS}
)
