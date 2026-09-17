// =====================================================================================
//  Projectile.h -- a "bolus" fired by the player: travels to the target point (onset
//  delay visualized as flight time, scaled from each drug's relative onset speed),
//  then blooms into a circular field-of-effect that lingers for a duration scaled from
//  the drug's real duration of action / context-sensitive half-time.
// =====================================================================================
#pragma once
#include "Common.h"
#include "DrugData.h"

enum class BolusState { Traveling, Blooming, Lingering, Done };

struct Bolus {
    DrugID drug;
    Vector2 start{}, target{};
    Vector2 pos{};
    float travelT = 0.0f;
    float travelDuration = 0.4f;
    float bloomRadius = 0.0f;
    float maxRadius = 60.0f;
    float lingerTimer = 0.0f;
    float lingerDuration = 1.0f;
    float tickTimer = 0.0f;
    BolusState state = BolusState::Traveling;
    float doseIntensity = 1.0f; // scales patient side effects + damage
    bool  hasHitEnemiesThisTick = false;

    void Init(DrugID d, Vector2 s, Vector2 t, float travelSec, float radius, float lingerSec, float intensity);
    void Update(float dt);
    void Draw() const;
    bool IsDone() const { return state == BolusState::Done; }
};
