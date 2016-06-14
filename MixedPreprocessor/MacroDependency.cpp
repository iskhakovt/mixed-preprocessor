// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MacroDependency.hpp"


void MacroDependency::AddDependencies(const MacroInfo *MI, const std::unordered_set<const MacroInfo *> &dependency) {
    if (graph_to.find(MI) != graph_to.end()) {
        graph_to[MI].insert(dependency.begin(), dependency.end());
    } else {
        graph_to[MI] = dependency;
    }

	for (const auto &to : graph_to[MI]) {
		graph_from[to].insert(MI);
	}
}

void MacroDependency::DeleteDependencies(const MacroInfo *MI){
    if (graph_to.find(MI) != graph_to.end()) {
        for (const auto &to : graph_to[MI]) {
            graph_from[to].erase(MI);
        }
    }
}

void MacroDependency::PreprocessDependent(const MacroInfo *MI, std::unordered_set<const MacroInfo *> &updated) {
	updated.insert(MI);

    /*
    DeleteDependencies(MI);
    if (MC.isDefined(MI)) {
        AddDependencies(MI, MC.Preprocess(MI));
    }
     */

	for (auto const &from : graph_from[MI]) {
		if (updated.find(from) == updated.end()) {
            PreprocessDependent(from, updated);
		}
	}
}

void MacroDependency::Update(const MacroInfo *MI) {
    std::unordered_set<const MacroInfo *> updated;
    PreprocessDependent(MI, updated);
    updated.clear();
    //ComputeDependent(MI, updated);
}
