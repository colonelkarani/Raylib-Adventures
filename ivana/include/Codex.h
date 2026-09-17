// =====================================================================================
//  Codex.h -- general pharmacology concepts (not tied to one drug) shown in the
//  in-game Codex, paraphrased from the chapter's introductory and mechanism sections.
// =====================================================================================
#pragma once
#include "Common.h"

struct CodexEntry {
    std::string title;
    std::string body;
};

const std::vector<CodexEntry>& GetConceptEntries();
