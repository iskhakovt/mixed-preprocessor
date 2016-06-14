// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#ifndef MIXED_PREPROCESSOR_MACRODEPENDENCY_HPP
#define MIXED_PREPROCESSOR_MACRODEPENDENCY_HPP


#include "MixedComputations.hpp"

#include "clang/Lex/MacroInfo.h"

#include <unordered_map>
#include <unordered_set>

using namespace clang;


class MixedComputations;


class MacroDependency {
	MixedComputations &MC;

    std::unordered_map<const MacroInfo *, std::unordered_set<const MacroInfo *>> graph_to;
    std::unordered_map<const MacroInfo *, std::unordered_set<const MacroInfo *>> graph_from;

	void AddDependencies(const MacroInfo *MI, const std::unordered_set<const MacroInfo *> &dependency);
	void DeleteDependencies(const MacroInfo *MI);

	void PreprocessDependent(const MacroInfo *MI, std::unordered_set<const MacroInfo *> &updated);

public:
    MacroDependency(MixedComputations &MC) : MC(MC) {}

	void Update(const MacroInfo *MI);
};


#endif //MIXED_PREPROCESSOR_MACRODEPENDENCY_HPP
