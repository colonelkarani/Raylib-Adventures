// =====================================================================================
//  Level.h -- eight scripted clinical scenarios, each drawn from a situation discussed
//  in the source chapter (rapid-sequence induction, balanced/TIVA maintenance, ICU
//  sedation, neurosurgical ICP control, ECT, septic-patient induction, status
//  epilepticus, and a closing aortic-dissection "no benefit from barbiturates" boss).
// =====================================================================================
#pragma once
#include "Common.h"
#include "Enemy.h"

struct SpawnEvent {
    float time;          // seconds since level start
    ThreatKind kind;
    int count;
    float hpMul = 1.0f;
    float speedMul = 1.0f;
    bool boss = false;
    float spreadRadius = 40.0f;
};

struct LevelDef {
    std::string title;
    std::string subtitle;
    std::string briefing;         // paraphrased clinical scenario setup
    std::string clinicalNote;     // paraphrased teaching point shown post-level
    std::vector<SpawnEvent> events;
    float duration = 60.0f;       // survive-until time if no boss, or boss-gated
    float startingBudgetMul = 1.0f;
    std::vector<DrugID> caution;      // drugs that are risky/backfire-prone in this scenario
    std::vector<DrugID> recommended;  // drugs the briefing highlights as indicated
    bool rewardsSeizureNotKillsIt = false; // ECT special rule flag
};

const std::vector<LevelDef>& GetCampaign();
