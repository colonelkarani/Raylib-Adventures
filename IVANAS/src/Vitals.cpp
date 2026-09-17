#include "Vitals.h"

static float EffectToDelta(Effect e) {
    switch (e) {
        case Effect::StrongDecrease: return -3.2f;
        case Effect::Decrease:       return -1.4f;
        case Effect::Unchanged:      return  0.0f;
        case Effect::Increase:       return  1.4f;
        case Effect::StrongIncrease: return  3.2f;
    }
    return 0.0f;
}

void Vitals::ApplyDrugSideEffects(const DrugProfile& d, float intensity) {
    map       += EffectToDelta(d.bloodPressureEffect) * intensity;
    heartRate += EffectToDelta(d.heartRateEffect) * intensity * 1.1f;
    respRate  += EffectToDelta(d.respiratoryEffect) * intensity * 0.6f;

    if (d.loweredICP) icp -= 2.0f * intensity;
    if (d.raisesICP)  icp += 2.6f * intensity;

    if (d.vagalBradycardiaRisk) heartRate -= 1.6f * intensity;
    if (d.sympatheticStimulant) heartRate += 1.2f * intensity;

    if (d.adrenalSuppressant) cortisolSuppression = Clampf(cortisolSuppression + 6.0f * intensity, 0.0f, 100.0f);
    if (d.amnestic)  awareness = Clampf(awareness - 10.0f * intensity, 0.0f, 100.0f);
    if (d.antiemetic) { /* handled by PONV threat resistance, not a vital directly */ }

    // Benzodiazepines chip away at awareness even without the "amnestic" super-flag
    if (d.id == DrugID::Diazepam || d.id == DrugID::Lorazepam) {
        awareness = Clampf(awareness - 6.0f * intensity, 0.0f, 100.0f);
    }
}

void Vitals::ApplyThreatDamage(ThreatKind k, float amount) {
    switch (k) {
        case ThreatKind::Nociception:      painLoad = Clampf(painLoad + amount, 0, 100); heartRate += amount * 0.05f; map += amount * 0.06f; break;
        case ThreatKind::Awareness:        awareness = Clampf(awareness + amount, 0, 100); break;
        case ThreatKind::AirwayReflex:     respRate -= amount * 0.08f; break;
        case ThreatKind::SeizureFocus:     icp += amount * 0.05f; heartRate += amount * 0.08f; break;
        case ThreatKind::SympatheticSurge: heartRate += amount * 0.12f; map += amount * 0.12f; break;
        case ThreatKind::VagalBradycardia: heartRate -= amount * 0.12f; break;
        case ThreatKind::PONV:             /* cosmetic/score only */ break;
        case ThreatKind::ICPSurge:         icp += amount * 0.18f; break;
        default: break;
    }
}

void Vitals::Update(float dt) {
    // Gentle homeostatic drift back toward normal (the body's own compensation),
    // slow enough that sustained mismanagement still matters.
    map        = Lerp1(map, 85.0f, dt * 0.15f);
    heartRate  = Lerp1(heartRate, 75.0f, dt * 0.15f);
    respRate   = Lerp1(respRate, 14.0f, dt * 0.2f);
    icp        = Lerp1(icp, 10.0f, dt * 0.1f);
    painLoad   = Clampf(painLoad - dt * 4.0f, 0, 100);
    awareness  = Clampf(awareness - dt * 1.5f, 0, 100);
    cortisolSuppression = Clampf(cortisolSuppression - dt * 0.8f, 0, 100);
    delirium   = Clampf(delirium - dt * 0.5f, 0, 100);

    map = Clampf(map, 20.0f, 180.0f);
    heartRate = Clampf(heartRate, 20.0f, 220.0f);
    respRate = Clampf(respRate, 0.0f, 40.0f);
    icp = Clampf(icp, 0.0f, 60.0f);

    if (map < 35.0f || heartRate < 30.0f || icp > 45.0f || painLoad >= 100.0f || awareness >= 100.0f) {
        alive = false;
    }
}

bool Vitals::InDanger() const {
    return map < 55.0f || map > 140.0f || heartRate < 45.0f || heartRate > 160.0f ||
           icp > 25.0f || painLoad > 70.0f || awareness > 70.0f || respRate < 5.0f;
}

std::string Vitals::WorstVitalLabel() const {
    if (icp > 25.0f) return "RAISED ICP";
    if (map < 55.0f) return "HYPOTENSION";
    if (map > 140.0f) return "HYPERTENSION";
    if (heartRate < 45.0f) return "BRADYCARDIA";
    if (heartRate > 160.0f) return "TACHYCARDIA";
    if (painLoad > 70.0f) return "NOCICEPTION";
    if (awareness > 70.0f) return "AWARENESS RISK";
    if (respRate < 5.0f) return "APNEA";
    return "STABLE";
}
