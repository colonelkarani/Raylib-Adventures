// =====================================================================================
//  Vitals.h -- the "patient" the player is protecting. Tracked as a small set of
//  monitor readouts, each nudged by drug side-effects (Table 8.2) and by threats that
//  reach the center of the field. This is a deliberately simplified physiologic model
//  for gameplay purposes; the *direction* every drug pushes each vital is accurate to
//  the source chapter, even though the numeric monitor is a game abstraction.
// =====================================================================================
#pragma once
#include "Common.h"
#include "DrugData.h"

struct Vitals {
    float map        = 85.0f;   // mean arterial pressure, mmHg (normal ~70-100)
    float heartRate   = 75.0f;  // bpm
    float respRate    = 14.0f;  // breaths/min (0 = apneic)
    float icp         = 10.0f;  // mmHg (normal <15, danger >20)
    float awareness    = 0.0f;  // 0..100, risk of explicit recall; 100 = awareness event
    float painLoad     = 0.0f;  // 0..100, unmanaged nociception; 100 = surgical stimulus response failure
    float cortisolSuppression = 0.0f; // 0..100, etomidate-driven adrenal suppression stack
    float delirium      = 0.0f; // 0..100, prolonged benzodiazepine-infusion ICU delirium risk
    bool  alive = true;

    void ApplyDrugSideEffects(const DrugProfile& d, float intensity);
    void ApplyThreatDamage(ThreatKind k, float amount);
    void Update(float dt);
    bool InDanger() const; // true if any vital is in a critical band (drives warning UI)
    std::string WorstVitalLabel() const;
};
