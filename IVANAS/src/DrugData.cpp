// =====================================================================================
//  DrugData.cpp -- see DrugData.h for the accuracy note. All doses, kinetics, and
//  qualitative CV/CNS/respiratory effect directions below are transcribed from the
//  chapter's Box 8.1, Table 8.1 (Pharmacokinetic Data), and Table 8.2 (Summary of
//  Pharmacodynamic Effects). Where the source table does not list a value for a given
//  drug (methohexital, diazepam, and lorazepam are not columns in Table 8.2), the value
//  is extrapolated from the same drug class as described in the surrounding prose
//  (e.g. methohexital tracks thiopental as a barbiturate; diazepam/lorazepam track
//  midazolam as benzodiazepines) and is commented as such below.
// =====================================================================================
#include "DrugData.h"

static std::array<DrugProfile, (size_t)DrugID::COUNT> g_roster;
static bool g_built = false;

static void Build() {
    DrugProfile p;

    // ---------------------------------------------------------------- PROPOFOL
    p = {};
    p.id = DrugID::Propofol; p.name = "Propofol"; p.shortName = "PRO";
    p.color = { 235, 235, 240, 255 }; // milky-white emulsion
    p.doseLowMgKg = 1.0f; p.doseHighMgKg = 2.5f;
    p.onsetSeconds = 0.35f;                 // fastest effect-site equilibration of the roster
    p.durationLowMin = 3; p.durationHighMin = 8;
    p.clearanceMlKgMin = 25.0f;             // 20-30, exceeds hepatic blood flow (extrahepatic met.)
    p.proteinBindingPct = 97.0f;
    p.contextHalfTimeFactor = 0.12f;        // stays brief even after prolonged infusion
    p.analgesic = false; p.amnestic = false; p.anticonvulsant = true; p.proconvulsant = false;
    p.raisesICP = false; p.loweredICP = true; p.adrenalSuppressant = false;
    p.antiemetic = true; p.proemetic = false; p.hasAntagonist = false;
    p.goodForRSI = true; p.goodForECT = false; p.vagalBradycardiaRisk = true; // profound baroreflex blunting
    p.sympatheticStimulant = false;
    p.bloodPressureEffect = Effect::StrongDecrease;
    p.heartRateEffect = Effect::Unchanged;   // "unchanged to decreased" -- baroreflex blunted, no reflex tachycardia
    p.respiratoryEffect = Effect::StrongDecrease;
    p.codexBlurb =
        "Alkylphenol hypnotic emulsified in soybean oil/egg lecithin. Fastest, cleanest "
        "recovery of the induction agents thanks to rapid hepatic AND extrahepatic "
        "clearance. No analgesia. Excellent for blunting airway reflexes and as an "
        "antiemetic. Its biggest cost is dose-dependent vasodilation -- it drops blood "
        "pressure more than any other induction drug, worse with rapid injection, age, "
        "and volume depletion.";
    g_roster[(size_t)p.id] = p;

    // ---------------------------------------------------------------- THIOPENTAL
    p = {};
    p.id = DrugID::Thiopental; p.name = "Thiopental"; p.shortName = "THIO";
    p.color = { 250, 225, 120, 255 };
    p.doseLowMgKg = 3.0f; p.doseHighMgKg = 5.0f;
    p.onsetSeconds = 0.45f;
    p.durationLowMin = 5; p.durationHighMin = 10;
    p.clearanceMlKgMin = 3.4f;              // slow hepatic clearance -- long elimination half-time
    p.proteinBindingPct = 83.0f;
    p.contextHalfTimeFactor = 0.95f;        // climbs sharply with repeated dosing/infusion
    p.analgesic = false; p.amnestic = false; p.anticonvulsant = true; p.proconvulsant = false;
    p.raisesICP = false; p.loweredICP = true; p.adrenalSuppressant = false;
    p.antiemetic = false; p.proemetic = false; p.hasAntagonist = false;
    p.goodForRSI = true; p.goodForECT = false; p.vagalBradycardiaRisk = false;
    p.sympatheticStimulant = false;
    p.bloodPressureEffect = Effect::Decrease;
    p.heartRateEffect = Effect::Increase;    // reflex tachycardia as baroreflex partially preserved
    p.respiratoryEffect = Effect::StrongDecrease;
    p.codexBlurb =
        "The classic thiobarbiturate; single-bolus recovery is quick because it relies "
        "on redistribution, but its true elimination is slow. Repeated boluses or an "
        "infusion cause the effect to stack badly (long context-sensitive half-time), so "
        "treat it as strictly single-shot. Potent cerebral vasoconstrictor -- useful for "
        "lowering ICP -- but a long context-sensitive half-time makes it a poor choice "
        "for maintenance sedation.";
    g_roster[(size_t)p.id] = p;

    // ---------------------------------------------------------------- METHOHEXITAL
    p = {};
    p.id = DrugID::Methohexital; p.name = "Methohexital"; p.shortName = "METHX";
    p.color = { 250, 205, 90, 255 };
    p.doseLowMgKg = 1.0f; p.doseHighMgKg = 1.5f;
    p.onsetSeconds = 0.4f;
    p.durationLowMin = 4; p.durationHighMin = 7;
    p.clearanceMlKgMin = 11.0f;             // cleared faster than thiopental -> shorter elimination half-time
    p.proteinBindingPct = 73.0f;
    p.contextHalfTimeFactor = 0.55f;        // barbiturate class, but clears faster than thiopental
    p.analgesic = false; p.amnestic = false; p.anticonvulsant = false; p.proconvulsant = true; // activates epileptic foci
    p.raisesICP = false; p.loweredICP = true; p.adrenalSuppressant = false;   // barbiturate-class cerebral vasoconstriction (extrapolated)
    p.antiemetic = false; p.proemetic = false; p.hasAntagonist = false;
    p.goodForRSI = false; p.goodForECT = true;  // longer seizure duration than propofol -- preferred for ECT
    p.vagalBradycardiaRisk = false; p.sympatheticStimulant = false;
    p.bloodPressureEffect = Effect::Decrease;      // extrapolated barbiturate-class effect
    p.heartRateEffect = Effect::Increase;          // extrapolated barbiturate-class effect
    p.respiratoryEffect = Effect::StrongDecrease;
    p.codexBlurb =
        "An oxybarbiturate cleared far more efficiently by the liver than thiopental, "
        "giving faster, more complete recovery. Unusual among anticonvulsant-leaning "
        "agents: methohexital instead ACTIVATES epileptic foci, which is exactly why it "
        "is prized for electroconvulsive therapy (longer seizure duration than propofol) "
        "and for intraoperative seizure-focus mapping. Do not deploy it against an "
        "established seizure -- it will make one worse.";
    g_roster[(size_t)p.id] = p;

    // ---------------------------------------------------------------- MIDAZOLAM
    p = {};
    p.id = DrugID::Midazolam; p.name = "Midazolam"; p.shortName = "MIDAZ";
    p.color = { 150, 190, 255, 255 };
    p.doseLowMgKg = 0.1f; p.doseHighMgKg = 0.3f;
    p.onsetSeconds = 0.6f;                  // slower effect-site equilibration than propofol/thiopental
    p.durationLowMin = 15; p.durationHighMin = 20;
    p.clearanceMlKgMin = 8.7f;              // 6.4-11
    p.proteinBindingPct = 94.0f;
    p.contextHalfTimeFactor = 0.30f;        // shortest context-sensitive half-time of the three benzodiazepines
    p.analgesic = false; p.amnestic = true; p.anticonvulsant = true; p.proconvulsant = false;
    p.raisesICP = false; p.loweredICP = false; p.adrenalSuppressant = false;
    p.antiemetic = true; p.proemetic = false; p.hasAntagonist = true;
    p.goodForRSI = false; p.goodForECT = false; p.vagalBradycardiaRisk = false;
    p.sympatheticStimulant = false;
    p.bloodPressureEffect = Effect::Unchanged;   // "unchanged to decreased"
    p.heartRateEffect = Effect::Unchanged;
    p.respiratoryEffect = Effect::Unchanged;     // "unchanged" alone; combined w/ opioids -> synergistic depression
    p.codexBlurb =
        "The go-to benzodiazepine for premedication and procedural sedation: potent "
        "anterograde amnesia, anxiolysis, and anticonvulsant action with only minor "
        "cardiorespiratory depression on its own (danger rises sharply combined with "
        "opioids). Best of the three benzodiazepines for continuous infusion, but "
        "prolonged ICU infusions are linked to longer ICU stay and more delirium than "
        "propofol or dexmedetomidine. Reversible with flumazenil.";
    g_roster[(size_t)p.id] = p;

    // ---------------------------------------------------------------- DIAZEPAM
    p = {};
    p.id = DrugID::Diazepam; p.name = "Diazepam"; p.shortName = "DIAZ";
    p.color = { 120, 165, 245, 255 };
    p.doseLowMgKg = 0.3f; p.doseHighMgKg = 0.6f;
    p.onsetSeconds = 0.55f;
    p.durationLowMin = 15; p.durationHighMin = 30;
    p.clearanceMlKgMin = 0.35f;             // 0.2-0.5, very slow -- active metabolites prolong effect
    p.proteinBindingPct = 98.0f;
    p.contextHalfTimeFactor = 0.85f;        // long elimination half-time, active metabolites accumulate
    p.analgesic = false; p.amnestic = true; p.anticonvulsant = true; p.proconvulsant = false;
    p.raisesICP = false; p.loweredICP = false; p.adrenalSuppressant = false;
    p.antiemetic = false; p.proemetic = false; p.hasAntagonist = true;
    p.goodForRSI = false; p.goodForECT = false; p.vagalBradycardiaRisk = false;
    p.sympatheticStimulant = false;
    p.bloodPressureEffect = Effect::Unchanged;  // benzodiazepine-class (extrapolated from midazolam)
    p.heartRateEffect = Effect::Unchanged;
    p.respiratoryEffect = Effect::Unchanged;
    p.codexBlurb =
        "Formulated in propylene glycol because it is poorly water soluble -- it is the "
        "solvent, not the drug, that causes the pain and thrombophlebitis diazepam is "
        "known for on injection. Active metabolites (desmethyldiazepam, oxazepam) give it "
        "the longest true duration of the benzodiazepines, especially in older patients. "
        "Effective for seizures from local-anesthetic toxicity or alcohol withdrawal.";
    g_roster[(size_t)p.id] = p;

    // ---------------------------------------------------------------- LORAZEPAM
    p = {};
    p.id = DrugID::Lorazepam; p.name = "Lorazepam"; p.shortName = "LORAZ";
    p.color = { 95, 145, 235, 255 };
    p.doseLowMgKg = 0.03f; p.doseHighMgKg = 0.1f;
    p.onsetSeconds = 1.1f;                  // slow onset -- explicitly a poor induction choice
    p.durationLowMin = 60; p.durationHighMin = 120;
    p.clearanceMlKgMin = 1.3f;              // 0.8-1.8
    p.proteinBindingPct = 98.0f;
    p.contextHalfTimeFactor = 0.6f;
    p.analgesic = false; p.amnestic = true; p.anticonvulsant = true; p.proconvulsant = false;
    p.raisesICP = false; p.loweredICP = false; p.adrenalSuppressant = false;
    p.antiemetic = false; p.proemetic = false; p.hasAntagonist = true;
    p.goodForRSI = false; p.goodForECT = false; p.vagalBradycardiaRisk = false;
    p.sympatheticStimulant = false;
    p.bloodPressureEffect = Effect::Unchanged;
    p.heartRateEffect = Effect::Unchanged;
    p.respiratoryEffect = Effect::Unchanged;
    p.codexBlurb =
        "Uniquely cleared by direct glucuronide conjugation rather than oxidation, so its "
        "kinetics barely change with age or liver disease. Slow onset and a very long "
        "duration make it unsuitable for induction -- but that same durability is exactly "
        "why it is the first-choice intravenous benzodiazepine for status epilepticus.";
    g_roster[(size_t)p.id] = p;

    // ---------------------------------------------------------------- KETAMINE
    p = {};
    p.id = DrugID::Ketamine; p.name = "Ketamine"; p.shortName = "KET";
    p.color = { 230, 130, 210, 255 };
    p.doseLowMgKg = 1.0f; p.doseHighMgKg = 2.0f;
    p.onsetSeconds = 0.5f;
    p.durationLowMin = 5; p.durationHighMin = 10;
    p.clearanceMlKgMin = 14.5f;             // 12-17
    p.proteinBindingPct = 12.0f;            // lowest of the roster
    p.contextHalfTimeFactor = 0.18f;        // short csht -- viable for infusion despite everything else
    p.analgesic = true; p.amnestic = false; p.anticonvulsant = true; p.proconvulsant = false; // "yes?" in source table
    p.raisesICP = true; p.loweredICP = false; p.adrenalSuppressant = false;
    p.antiemetic = false; p.proemetic = false; p.hasAntagonist = false;
    p.goodForRSI = false; p.goodForECT = false; p.vagalBradycardiaRisk = false;
    p.sympatheticStimulant = true;          // centrally mediated sympathetic stimulation
    p.bloodPressureEffect = Effect::Increase;
    p.heartRateEffect = Effect::Increase;
    p.respiratoryEffect = Effect::Unchanged;
    p.codexBlurb =
        "NMDA-receptor antagonist producing true dissociative anesthesia: profound "
        "analgesia with the eyes open and airway reflexes present (though not "
        "necessarily protective). The only induction agent that raises blood pressure, "
        "heart rate and cardiac output via central sympathetic stimulation -- valuable in "
        "hypovolemic or bronchospastic patients, dangerous in anyone with a fixed cardiac "
        "output or raised ICP, since it is a cerebral vasodilator. Unpleasant emergence "
        "reactions (vivid dreams, dissociation) are its main limitation.";
    g_roster[(size_t)p.id] = p;

    // ---------------------------------------------------------------- ETOMIDATE
    p = {};
    p.id = DrugID::Etomidate; p.name = "Etomidate"; p.shortName = "ETOM";
    p.color = { 210, 210, 90, 255 };
    p.doseLowMgKg = 0.2f; p.doseHighMgKg = 0.3f;
    p.onsetSeconds = 0.4f;
    p.durationLowMin = 3; p.durationHighMin = 8;
    p.clearanceMlKgMin = 21.5f;             // 18-25, ~5x thiopental's clearance
    p.proteinBindingPct = 77.0f;
    p.contextHalfTimeFactor = 0.2f;         // minimal hemodynamic drift, tolerates repeat dosing well
    p.analgesic = false; p.amnestic = false; p.anticonvulsant = false; p.proconvulsant = false;
    p.raisesICP = false; p.loweredICP = true; p.adrenalSuppressant = true; // 11-beta-hydroxylase inhibition
    p.antiemetic = false; p.proemetic = true; p.hasAntagonist = false;
    p.goodForRSI = true; p.goodForECT = true; // longer seizure duration than propofol/methohexital in ECT
    p.vagalBradycardiaRisk = false; p.sympatheticStimulant = false;
    p.bloodPressureEffect = Effect::Unchanged;   // "unchanged to decreased" -- the headline feature
    p.heartRateEffect = Effect::Unchanged;
    p.respiratoryEffect = Effect::Unchanged;     // "unchanged to decreased", milder than the others
    p.codexBlurb =
        "A carboxylated imidazole prized for near-total cardiovascular stability on "
        "induction -- the reason it is chosen for patients with poor myocardial reserve "
        "or severe aortic stenosis. The catch is dose-dependent inhibition of "
        "11-beta-hydroxylase, suppressing cortisol synthesis for many hours after even a "
        "single dose. Myoclonus is common. Recent meta-analyses have pushed back on "
        "earlier claims that a single induction dose raises mortality in septic patients, "
        "but the adrenal-suppression trade-off is real and worth tracking.";
    g_roster[(size_t)p.id] = p;

    // ---------------------------------------------------------------- DEXMEDETOMIDINE
    p = {};
    p.id = DrugID::Dexmedetomidine; p.name = "Dexmedetomidine"; p.shortName = "DEX";
    p.color = { 150, 235, 220, 255 };
    p.doseLowMgKg = 0.0f; p.doseHighMgKg = 0.0f;   // not used as a bolus induction agent (N/A in Table 8.1)
    p.onsetSeconds = 0.8f;                          // loading infusion given over ~10 minutes clinically
    p.durationLowMin = 20; p.durationHighMin = 40;
    p.clearanceMlKgMin = 20.0f;             // 10-30, high clearance, short elimination half-time
    p.proteinBindingPct = 94.0f;
    p.contextHalfTimeFactor = 0.9f;         // rises steeply: ~4 min after 10-min infusion, ~250 min after 8h
    p.analgesic = true; p.amnestic = false; p.anticonvulsant = false; p.proconvulsant = false; // "yes?" in source table
    p.raisesICP = false; p.loweredICP = false; p.adrenalSuppressant = false;
    p.antiemetic = false; p.proemetic = false; p.hasAntagonist = false; // reversible w/ alpha2-antagonists (not modeled)
    p.goodForRSI = false; p.goodForECT = false; p.vagalBradycardiaRisk = true;
    p.sympatheticStimulant = false;
    p.bloodPressureEffect = Effect::Decrease;    // bolus may transiently increase via peripheral alpha2, then decreases
    p.heartRateEffect = Effect::StrongDecrease;
    p.respiratoryEffect = Effect::Unchanged;     // minimal ventilatory depression -- its signature advantage
    p.codexBlurb =
        "Highly selective alpha-2 agonist producing sedation that resembles natural sleep "
        "via the locus ceruleus, plus spinally-mediated analgesia, with only minimal "
        "respiratory depression -- unique among this roster. A bolus can transiently "
        "raise blood pressure via peripheral alpha-2 stimulation before the infusion "
        "settles into bradycardia and lower blood pressure; heart block or asystole from "
        "unopposed vagal tone is the drug's real hazard. Its context-sensitive half-time "
        "balloons dramatically with prolonged infusion.";
    g_roster[(size_t)p.id] = p;

    g_built = true;
}

const std::array<DrugProfile, (size_t)DrugID::COUNT>& GetDrugRoster() {
    if (!g_built) Build();
    return g_roster;
}

const DrugProfile& GetDrug(DrugID id) {
    return GetDrugRoster()[(size_t)id];
}

// =====================================================================================
//  Effectiveness matrix -- paraphrased directly from the pharmacodynamic spectrum
//  described for each drug (analgesia, amnesia, anticonvulsant activity, effect on
//  airway reflexes, CBF/ICP, PONV, and cardiovascular stimulant/depressant profile).
// =====================================================================================
float DrugEffectivenessVsThreat(DrugID drug, ThreatKind threat) {
    using D = DrugID; using T = ThreatKind;
    // [drug][threat] table, rows in DrugID order, columns in ThreatKind order.
    static const float table[(size_t)D::COUNT][(size_t)T::COUNT] = {
        /*                         Nocic  Aware  Airway  Seiz  Sympath  Vagal  PONV  ICP  */
        /* Propofol         */   { 0.10f, 0.50f, 0.90f, 0.85f, 0.50f,  0.00f, 0.90f, 0.85f },
        /* Thiopental       */   { 0.00f, 0.40f, 0.40f, 0.80f, 0.30f,  0.00f, 0.35f, 0.85f },
        /* Methohexital     */   { 0.00f, 0.30f, 0.50f, 0.05f, 0.30f,  0.00f, 0.35f, 0.55f },
        /* Midazolam        */   { 0.00f, 0.95f, 0.20f, 0.75f, 0.35f,  0.00f, 0.60f, 0.25f },
        /* Diazepam         */   { 0.00f, 0.70f, 0.15f, 0.85f, 0.30f,  0.00f, 0.30f, 0.25f },
        /* Lorazepam        */   { 0.00f, 0.60f, 0.10f, 0.95f, 0.30f,  0.00f, 0.30f, 0.20f },
        /* Ketamine         */   { 0.90f, 0.30f, 0.20f, 0.55f, 0.05f,  0.55f, 0.20f, 0.00f },
        /* Etomidate        */   { 0.00f, 0.30f, 0.50f, 0.00f, 0.50f,  0.00f, 0.05f, 0.80f },
        /* Dexmedetomidine  */   { 0.50f, 0.40f, 0.30f, 0.00f, 0.70f,  0.00f, 0.35f, 0.30f },
    };
    return table[(size_t)drug][(size_t)threat];
}

float DrugBackfireMultiplier(DrugID drug, ThreatKind threat) {
    using D = DrugID; using T = ThreatKind;
    // Methohexital activates epileptic foci -- firing it at a seizure focus strengthens it.
    if (drug == D::Methohexital && threat == T::SeizureFocus) return 1.6f;
    // Ketamine is a cerebral vasodilator -- firing it at a raised-ICP target worsens it.
    if (drug == D::Ketamine && threat == T::ICPSurge) return 1.5f;
    // Ketamine's centrally-mediated sympathetic stimulation feeds a sympathetic surge.
    if (drug == D::Ketamine && threat == T::SympatheticSurge) return 1.3f;
    // Etomidate is associated with more PONV, not less.
    if (drug == D::Etomidate && threat == T::PONV) return 1.2f;
    // Dexmedetomidine's vagal tone can worsen an already-bradycardic/heart-block threat.
    if (drug == D::Dexmedetomidine && threat == T::VagalBradycardia) return 1.4f;
    return 1.0f;
}
