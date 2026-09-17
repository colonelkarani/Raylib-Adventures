#include "Projectile.h"

void Bolus::Init(DrugID d, Vector2 s, Vector2 t, float travelSec, float radius, float lingerSec, float intensity) {
    drug = d; start = s; target = t; pos = s;
    travelT = 0.0f; travelDuration = std::max(0.08f, travelSec);
    bloomRadius = 0.0f; maxRadius = radius;
    lingerTimer = 0.0f; lingerDuration = lingerSec;
    tickTimer = 0.0f;
    state = BolusState::Traveling;
    doseIntensity = intensity;
}

void Bolus::Update(float dt) {
    switch (state) {
        case BolusState::Traveling: {
            travelT += dt / travelDuration;
            pos = Vector2Lerp(start, target, Clampf(travelT, 0, 1));
            if (travelT >= 1.0f) state = BolusState::Blooming;
            break;
        }
        case BolusState::Blooming: {
            bloomRadius += (maxRadius) * dt * 6.0f;
            if (bloomRadius >= maxRadius) { bloomRadius = maxRadius; state = BolusState::Lingering; }
            break;
        }
        case BolusState::Lingering: {
            lingerTimer += dt;
            tickTimer += dt;
            if (lingerTimer >= lingerDuration) state = BolusState::Done;
            break;
        }
        default: break;
    }
}

void Bolus::Draw() const {
    const DrugProfile& d = GetDrug(drug);
    if (state == BolusState::Traveling) {
        DrawCircleV(pos, 6.0f, d.color);
        DrawCircleV(pos, 10.0f, Fade(d.color, 0.35f));
        DrawLineEx(start, pos, 2.0f, Fade(d.color, 0.25f));
    } else if (state != BolusState::Done) {
        float alpha = state == BolusState::Lingering ? Lerp1(0.35f, 0.08f, lingerTimer / lingerDuration) : 0.45f;
        DrawCircleV(target, bloomRadius, Fade(d.color, alpha));
        DrawCircleLines((int)target.x, (int)target.y, bloomRadius, Fade(d.color, 0.6f));
    }
}
