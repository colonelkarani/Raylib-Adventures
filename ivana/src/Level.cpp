// =====================================================================================
//  Level.cpp -- see Level.h. Spawn timings/counts are original game-design content;
//  the briefing/clinicalNote text paraphrases real teaching points from the chapter.
// =====================================================================================
#include "Level.h"

static std::vector<LevelDef> BuildCampaign() {
    std::vector<LevelDef> L;

    // ---------------------------------------------------------------- LEVEL 1
    {
        LevelDef lv;
        lv.title = "Scenario 1: Rapid-Sequence Induction";
        lv.subtitle = "Full stomach, aspiration risk -- get the airway secured fast.";
        lv.briefing =
            "A patient with a full stomach needs the airway secured with minimal delay. "
            "Classic teaching favors a fast-onset hypnotic immediately followed by a "
            "rapid-acting muscle relaxant, avoiding bag-mask ventilation altogether. "
            "You need an agent that suppresses airway reflexes hard enough to instrument "
            "the airway cleanly -- and you need it to act NOW, not in a minute.";
        lv.clinicalNote =
            "Propofol has replaced thiopental as the default rapid-sequence induction "
            "agent for most patients because it blunts airway reflexes more completely. "
            "Etomidate is the fallback whenever the induction dose of propofol would drop "
            "blood pressure too far to tolerate. Anything with a slow onset -- looking at "
            "you, the benzodiazepines -- is the wrong tool here.";
        lv.duration = 50.0f;
        lv.recommended = { DrugID::Propofol, DrugID::Etomidate };
        lv.caution = { DrugID::Lorazepam, DrugID::Diazepam };
        lv.events = {
            {2.0f,  ThreatKind::AirwayReflex, 2, 1.0f, 1.0f},
            {8.0f,  ThreatKind::AirwayReflex, 3, 1.05f, 1.0f},
            {14.0f, ThreatKind::Nociception,  2, 1.0f, 1.1f},
            {20.0f, ThreatKind::AirwayReflex, 3, 1.1f, 1.1f},
            {26.0f, ThreatKind::SympatheticSurge, 2, 1.0f, 1.0f},
            {32.0f, ThreatKind::AirwayReflex, 4, 1.2f, 1.15f},
            {40.0f, ThreatKind::Nociception,  3, 1.1f, 1.1f},
        };
        L.push_back(lv);
    }

    // ---------------------------------------------------------------- LEVEL 2
    {
        LevelDef lv;
        lv.title = "Scenario 2: Balanced Maintenance";
        lv.subtitle = "Keep the surgical field quiet for the whole case.";
        lv.briefing =
            "Induction is done -- now the case has to be maintained for real time. "
            "Rather than piling on one drug, balanced anesthesia leans on smaller doses "
            "of several agents together: something to keep pain signals from reaching "
            "consciousness, and something else to guarantee no memory of the case forms.";
        lv.clinicalNote =
            "Neither propofol nor a benzodiazepine is analgesic on its own -- that's why "
            "modern maintenance is 'balanced' across drug classes rather than relying on "
            "one agent to do everything. Explicit recall is rare, but when it happens the "
            "amnestic cover of a benzodiazepine is usually what was missing.";
        lv.duration = 75.0f;
        lv.recommended = { DrugID::Propofol, DrugID::Midazolam, DrugID::Ketamine };
        lv.events = {
            {2.0f,  ThreatKind::Nociception, 3, 1.0f, 1.0f},
            {10.0f, ThreatKind::Awareness,   2, 1.0f, 1.0f},
            {18.0f, ThreatKind::Nociception, 3, 1.05f, 1.05f},
            {26.0f, ThreatKind::PONV,        3, 1.0f, 1.1f},
            {34.0f, ThreatKind::Awareness,   3, 1.1f, 1.05f},
            {42.0f, ThreatKind::Nociception, 4, 1.1f, 1.1f},
            {50.0f, ThreatKind::SympatheticSurge, 2, 1.1f, 1.0f},
            {58.0f, ThreatKind::Awareness,   3, 1.2f, 1.1f},
            {66.0f, ThreatKind::Nociception, 5, 1.2f, 1.15f},
        };
        L.push_back(lv);
    }

    // ---------------------------------------------------------------- LEVEL 3
    {
        LevelDef lv;
        lv.title = "Scenario 3: ICU Sedation";
        lv.subtitle = "Hours matter now, not minutes -- infusion kinetics decide the winner.";
        lv.briefing =
            "A mechanically ventilated ICU patient needs sedation that can run for hours "
            "without leaving them foggy for days afterward. This is where context-sensitive "
            "half-time stops being trivia and starts being the whole game: an agent that "
            "recovers quickly after a short infusion can still accumulate badly over a long one.";
        lv.clinicalNote =
            "Prolonged benzodiazepine infusions are associated with longer ICU stays and "
            "more delirium than propofol or dexmedetomidine -- lean on this level's short "
            "context-sensitive half-time agents for sustained sedation, and treat "
            "thiopental-class drugs as strictly single-shot.";
        lv.duration = 90.0f;
        lv.recommended = { DrugID::Propofol, DrugID::Dexmedetomidine };
        lv.caution = { DrugID::Midazolam, DrugID::Thiopental, DrugID::Diazepam };
        lv.events = {
            {2.0f,  ThreatKind::Nociception, 2, 1.0f, 1.0f},
            {10.0f, ThreatKind::Awareness,   2, 1.0f, 1.0f},
            {20.0f, ThreatKind::PONV,        3, 1.0f, 1.0f},
            {30.0f, ThreatKind::Nociception, 3, 1.1f, 1.05f},
            {40.0f, ThreatKind::VagalBradycardia, 2, 1.0f, 1.0f},
            {50.0f, ThreatKind::Awareness,   3, 1.15f, 1.1f},
            {60.0f, ThreatKind::Nociception, 4, 1.2f, 1.1f},
            {70.0f, ThreatKind::SympatheticSurge, 3, 1.1f, 1.1f},
            {80.0f, ThreatKind::Nociception, 5, 1.25f, 1.15f},
        };
        L.push_back(lv);
    }

    // ---------------------------------------------------------------- LEVEL 4
    {
        LevelDef lv;
        lv.title = "Scenario 4: Raised Intracranial Pressure";
        lv.subtitle = "A space-occupying lesion. One wrong drug and pressure spikes.";
        lv.briefing =
            "This patient has reduced intracranial compliance. Cerebral vasoconstrictors "
            "that lower cerebral blood flow and metabolic demand are the whole strategy "
            "here. Anything that dilates cerebral vessels is not just unhelpful -- it is "
            "actively dangerous.";
        lv.clinicalNote =
            "Ketamine is a cerebral vasodilator that raises cerebral blood flow and ICP, "
            "which is why it is generally avoided with intracranial hypertension. Propofol, "
            "thiopental, and etomidate are all potent cerebral vasoconstrictors and remain "
            "the mainstay for neuroanesthesia induction.";
        lv.duration = 70.0f;
        lv.recommended = { DrugID::Propofol, DrugID::Thiopental, DrugID::Etomidate };
        lv.caution = { DrugID::Ketamine };
        lv.events = {
            {2.0f,  ThreatKind::Nociception, 2, 1.0f, 1.0f},
            {10.0f, ThreatKind::ICPSurge,    1, 0.5f, 1.0f}, // early warning pulse, reduced HP
            {20.0f, ThreatKind::SeizureFocus,2, 1.0f, 1.0f},
            {30.0f, ThreatKind::Nociception, 3, 1.05f, 1.05f},
            {40.0f, ThreatKind::ICPSurge,    1, 0.85f, 1.0f},
            {50.0f, ThreatKind::SeizureFocus,2, 1.15f, 1.05f},
            {62.0f, ThreatKind::ICPSurge,    1, 1.3f, 1.0f, true}, // full boss
        };
        L.push_back(lv);
    }

    // ---------------------------------------------------------------- LEVEL 5
    {
        LevelDef lv;
        lv.title = "Scenario 5: Electroconvulsive Therapy";
        lv.subtitle = "For once, the seizure is the goal -- protect it, then end it cleanly.";
        lv.briefing =
            "Counterintuitively, this scenario needs a seizure to be adequate, not "
            "suppressed. An agent that activates epileptic activity and lengthens seizure "
            "duration is what you want on the objective; a strong anticonvulsant fired at "
            "it will cut the therapeutic seizure short.";
        lv.clinicalNote =
            "Methohexital and etomidate both produce longer seizure durations than propofol "
            "during electroconvulsive therapy, which is exactly why they remain favored "
            "hypnotics for the procedure despite propofol's popularity elsewhere.";
        lv.duration = 55.0f;
        lv.recommended = { DrugID::Methohexital, DrugID::Etomidate };
        lv.caution = { DrugID::Propofol, DrugID::Midazolam, DrugID::Lorazepam };
        lv.rewardsSeizureNotKillsIt = true;
        lv.events = {
            {4.0f,  ThreatKind::Awareness, 2, 1.0f, 1.0f},
            {14.0f, ThreatKind::Nociception, 2, 1.0f, 1.0f},
            {24.0f, ThreatKind::Awareness, 2, 1.1f, 1.05f},
            {34.0f, ThreatKind::Nociception, 3, 1.1f, 1.05f},
            {44.0f, ThreatKind::Awareness, 3, 1.15f, 1.1f},
        };
        L.push_back(lv);
    }

    // ---------------------------------------------------------------- LEVEL 6
    {
        LevelDef lv;
        lv.title = "Scenario 6: Septic Shock Intubation";
        lv.subtitle = "Borderline pressures. Pick the agent that won't finish the job for you.";
        lv.briefing =
            "This patient's blood pressure has almost no reserve left. Any drug that adds "
            "even a modest additional vasodilator hit on top of established shock could be "
            "the last straw. You need the induction agent famous for near-total "
            "cardiovascular stability -- and you need to keep an eye on what it costs you.";
        lv.clinicalNote =
            "Etomidate is often chosen for induction in hemodynamically fragile patients "
            "precisely because it changes blood pressure so little. It does suppress "
            "cortisol synthesis for many hours afterward, which drove real concern about "
            "mortality in septic patients -- concern that more recent meta-analyses have "
            "not clearly upheld. The trade-off is worth knowing either way.";
        lv.duration = 65.0f;
        lv.recommended = { DrugID::Etomidate };
        lv.caution = { DrugID::Propofol, DrugID::Thiopental };
        lv.events = {
            {2.0f,  ThreatKind::AirwayReflex, 2, 1.0f, 1.0f},
            {10.0f, ThreatKind::SympatheticSurge, 2, 1.0f, 1.0f},
            {18.0f, ThreatKind::AirwayReflex, 3, 1.1f, 1.05f},
            {28.0f, ThreatKind::PONV, 2, 1.0f, 1.0f},
            {36.0f, ThreatKind::SympatheticSurge, 3, 1.15f, 1.1f},
            {46.0f, ThreatKind::AirwayReflex, 3, 1.2f, 1.1f},
            {56.0f, ThreatKind::SympatheticSurge, 3, 1.2f, 1.15f},
        };
        L.push_back(lv);
    }

    // ---------------------------------------------------------------- LEVEL 7
    {
        LevelDef lv;
        lv.title = "Scenario 7: Status Epilepticus";
        lv.subtitle = "Escalating seizures. Work the ladder, not just the biggest hammer.";
        lv.briefing =
            "Seizure activity is escalating and won't stop on its own. There is a real "
            "treatment order here: a benzodiazepine first, and only if that fails do "
            "you reach for a propofol or barbiturate infusion.";
        lv.clinicalNote =
            "Lorazepam is generally regarded as the intravenous benzodiazepine of choice "
            "for status epilepticus because of its long duration; diazepam is a reasonable "
            "alternative, and intramuscular midazolam is used prehospital when an IV isn't "
            "available yet. Barbiturate or propofol infusions are reserved as third-line "
            "therapy once benzodiazepines have failed.";
        lv.duration = 80.0f;
        lv.recommended = { DrugID::Lorazepam, DrugID::Diazepam, DrugID::Midazolam };
        lv.caution = { DrugID::Methohexital };
        lv.events = {
            {2.0f,  ThreatKind::SeizureFocus, 1, 1.0f, 1.0f},
            {10.0f, ThreatKind::SeizureFocus, 2, 1.0f, 1.0f},
            {20.0f, ThreatKind::SeizureFocus, 2, 1.1f, 1.05f},
            {30.0f, ThreatKind::Nociception,  2, 1.0f, 1.0f},
            {38.0f, ThreatKind::SeizureFocus, 3, 1.15f, 1.05f},
            {48.0f, ThreatKind::SeizureFocus, 3, 1.2f, 1.1f},
            {60.0f, ThreatKind::SeizureFocus, 4, 1.25f, 1.1f},
            {72.0f, ThreatKind::SeizureFocus, 2, 1.4f, 1.2f, true},
        };
        L.push_back(lv);
    }

    // ---------------------------------------------------------------- LEVEL 8 (FINAL BOSS)
    {
        LevelDef lv;
        lv.title = "Scenario 8: Acute Type-A Aortic Dissection Repair";
        lv.subtitle = "Final case. Don't reach for barbiturates hoping for a miracle.";
        lv.briefing =
            "A repair carrying real risk of neurologic injury from interrupted cerebral "
            "perfusion. It is tempting to lean on high-dose barbiturate 'neuroprotection', "
            "the way it's used for focal ischemia elsewhere -- but a large outcomes "
            "registry for exactly this operation found no such benefit, and the "
            "accompanying hypotension can make things worse.";
        lv.clinicalNote =
            "In a large registry of acute type-A aortic dissection repairs, barbiturates "
            "gave no measurable benefit for preventing permanent neurologic dysfunction, "
            "and are considered unlikely to help during cardiac surgery generally. "
            "Cardiovascular stability (etomidate) and genuine cerebral protection without "
            "hemodynamic collapse (propofol, careful dosing) matter more than chasing an "
            "isoelectric EEG with a barbiturate here.";
        lv.duration = 100.0f;
        lv.recommended = { DrugID::Etomidate, DrugID::Propofol };
        lv.caution = { DrugID::Thiopental, DrugID::Methohexital, DrugID::Ketamine };
        lv.events = {
            {2.0f,  ThreatKind::SympatheticSurge, 2, 1.0f, 1.0f},
            {10.0f, ThreatKind::Nociception, 3, 1.0f, 1.0f},
            {18.0f, ThreatKind::AirwayReflex, 2, 1.0f, 1.0f},
            {26.0f, ThreatKind::SeizureFocus, 2, 1.1f, 1.0f},
            {34.0f, ThreatKind::SympatheticSurge, 3, 1.1f, 1.05f},
            {42.0f, ThreatKind::Nociception, 4, 1.15f, 1.1f},
            {50.0f, ThreatKind::VagalBradycardia, 2, 1.1f, 1.0f},
            {58.0f, ThreatKind::AirwayReflex, 3, 1.2f, 1.1f},
            {66.0f, ThreatKind::Nociception, 4, 1.25f, 1.15f},
            {76.0f, ThreatKind::ICPSurge, 1, 1.6f, 1.0f, true},
        };
        L.push_back(lv);
    }

    return L;
}

const std::vector<LevelDef>& GetCampaign() {
    static std::vector<LevelDef> campaign = BuildCampaign();
    return campaign;
}
