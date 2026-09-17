// =====================================================================================
//  DrugData.h
//  Static pharmacologic profile for each playable intravenous anesthetic, plus the
//  per-threat effectiveness matrix derived from the source chapter's pharmacodynamic
//  tables (spectrum of effects; CNS/CV/respiratory effects table; clinical-use text).
// =====================================================================================
#pragma once
#include "Common.h"

// Effect magnitude used for both patient side-effects and enemy matchups.
// Mirrors the qualitative language of the source table ("decreased", "unchanged",
// "increased", "unchanged to decreased", etc.) collapsed to a signed float scale.
enum class Effect : int { StrongDecrease = -2, Decrease = -1, Unchanged = 0, Increase = 1, StrongIncrease = 2 };

struct DrugProfile {
    DrugID       id;
    std::string  name;
    std::string  shortName;          // hotbar label
    Color        color;

    // ---- Pharmacokinetics (Table 8.1) ----
    float doseLowMgKg, doseHighMgKg;     // induction dose range, mg/kg IV
    float onsetSeconds;                  // scaled from relative onset speed (all IV agents
                                          // are "rapid", so this drives short in-game windup)
    float durationLowMin, durationHighMin; // duration of action after a single bolus, minutes
    float clearanceMlKgMin;              // plasma clearance -- drives resource regeneration
    float proteinBindingPct;             // fraction protein-bound (affects free-fraction flavor text)
    float contextHalfTimeFactor;         // 0..1, relative growth of context-sensitive half-time
                                          // with prolonged infusion (propofol/etomidate/ketamine
                                          // low & flat; thiopental/diazepam climb steeply; midazolam
                                          // intermediate) -- drives infusion "drift" penalty

    // ---- Pharmacodynamics / clinical flags ----
    bool  analgesic;             // produces true analgesia (ketamine; dexmedetomidine "yes?")
    bool  amnestic;              // strong anterograde amnesia (benzodiazepines)
    bool  anticonvulsant;        // Table 8.2 "Anticonvulsant" row
    bool  proconvulsant;         // methohexital activates epileptic foci (special case)
    bool  raisesICP;             // ketamine: cerebral vasodilator, increases CBF/ICP
    bool  loweredICP;            // propofol/thiopental/etomidate: cerebral vasoconstrictors
    bool  adrenalSuppressant;    // etomidate: 11-beta-hydroxylase inhibition
    bool  antiemetic;            // propofol reduces PONV
    bool  proemetic;             // etomidate associated with more PONV
    bool  hasAntagonist;         // benzodiazepines reversible with flumazenil
    bool  goodForRSI;            // suitable for rapid tracheal access (airway reflex suppression)
    bool  goodForECT;            // favorable seizure-duration profile for ECT
    bool  vagalBradycardiaRisk;  // dexmedetomidine bolus / bradycardia, heart block
    bool  sympatheticStimulant;  // ketamine raises HR/BP/CO centrally

    Effect bloodPressureEffect;
    Effect heartRateEffect;
    Effect respiratoryEffect;

    std::string codexBlurb;      // short paraphrased educational summary for the Codex screen
};

// The full roster, defined in DrugData.cpp
const std::array<DrugProfile, (size_t)DrugID::COUNT>& GetDrugRoster();
const DrugProfile& GetDrug(DrugID id);

// Effectiveness of a drug against a given threat archetype, 0..1 (paraphrased from the
// chapter's discussion of which agent is/is not indicated for which physiologic problem).
float DrugEffectivenessVsThreat(DrugID drug, ThreatKind threat);

// Some drug/threat interactions actively BACKFIRE (e.g. methohexital vs a seizure focus,
// or ketamine vs a raised-ICP target). Returns a multiplier applied to the *enemy*,
// >1 meaning the enemy is empowered rather than harmed.
float DrugBackfireMultiplier(DrugID drug, ThreatKind threat);
