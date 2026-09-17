#include "Enemy.h"

static const EnemyTypeInfo kEnemyInfo[(size_t)ThreatKind::COUNT] = {
    { ThreatKind::Nociception,      "Nociceptive Spike",   "Unblunted surgical stimulus -- needs true analgesia.",
      { 235, 90, 90, 255 },   26.0f, 55.0f, 10.0f, 9.0f },
    { ThreatKind::Awareness,        "Recall Specter",      "Risk of explicit intraoperative memory -- needs amnesia.",
      { 190, 120, 235, 255 }, 22.0f, 40.0f, 11.0f, 11.0f },
    { ThreatKind::AirwayReflex,     "Airway Reflex",       "Laryngospasm/bronchospasm risk on instrumentation.",
      { 235, 170, 90, 255 },  30.0f, 46.0f, 12.0f, 10.0f },
    { ThreatKind::SeizureFocus,     "Seizure Focus",       "Epileptiform activity -- needs an anticonvulsant.",
      { 235, 230, 90, 255 },  34.0f, 35.0f, 13.0f, 13.0f },
    { ThreatKind::SympatheticSurge, "Sympathetic Surge",   "Stress-response tachycardia/hypertension.",
      { 235, 120, 60, 255 },  30.0f, 60.0f, 12.0f, 10.0f },
    { ThreatKind::VagalBradycardia, "Vagal Bradycardia",   "Excess vagal tone -- risk of heart block.",
      { 90, 150, 235, 255 },  26.0f, 38.0f, 11.0f, 10.0f },
    { ThreatKind::PONV,             "PONV Wisp",           "Postoperative nausea & vomiting.",
      { 150, 230, 150, 255 }, 14.0f, 50.0f, 8.0f,  4.0f },
    { ThreatKind::ICPSurge,         "ICP Surge",           "Raised intracranial pressure -- boss-tier threat.",
      { 235, 60, 130, 255 },  260.0f, 18.0f, 26.0f, 22.0f },
};

const EnemyTypeInfo& GetEnemyTypeInfo(ThreatKind k) { return kEnemyInfo[(size_t)k]; }

void Enemy::Init(ThreatKind k, Vector2 spawnPos, float hpMul, float speedMul, bool boss) {
    const EnemyTypeInfo& info = GetEnemyTypeInfo(k);
    kind = k;
    pos = spawnPos;
    maxHp = hp = info.baseHp * hpMul * (boss ? 3.2f : 1.0f);
    speed = info.baseSpeed * speedMul * (boss ? 0.55f : 1.0f);
    radius = info.radius * (boss ? 1.9f : 1.0f);
    alive = true;
    isBoss = boss;
    slowTimer = 0.0f;
    empowerTimer = 0.0f;
    spawnFlashTimer = 0.35f;
    bobPhase = RandRangef(0.0f, 6.28f);
}

void Enemy::Update(float dt, Vector2 target) {
    if (spawnFlashTimer > 0) spawnFlashTimer -= dt;
    if (slowTimer > 0) slowTimer -= dt;
    if (empowerTimer > 0) empowerTimer -= dt;
    bobPhase += dt * 4.0f;

    float curSpeed = speed * (slowTimer > 0 ? 0.35f : 1.0f);
    Vector2 dir = Vector2Normalize(Vector2Subtract(target, pos));

    // Each threat archetype gets a distinct movement signature, loosely echoing its
    // real-world behavior: a sympathetic surge arrives in sudden bursts the way a
    // stress response spikes; an airway reflex is jittery/reactive; a bradycardic
    // threat crawls steadily like a slowing pulse; nociception is a fast, direct dart;
    // PONV wanders (nausea doesn't move in a straight line); a seizure focus twitches.
    switch (kind) {
        case ThreatKind::SympatheticSurge: {
            float burst = (sinf(bobPhase * 1.5f) > 0.6f) ? 2.2f : 0.5f;
            pos = Vector2Add(pos, Vector2Scale(dir, curSpeed * burst * dt));
            break;
        }
        case ThreatKind::AirwayReflex: {
            Vector2 jitter = { sinf(bobPhase * 3.1f) * 30.0f, cosf(bobPhase * 2.3f) * 30.0f };
            Vector2 wobble = Vector2Add(dir, Vector2Scale(jitter, dt));
            pos = Vector2Add(pos, Vector2Scale(Vector2Normalize(wobble), curSpeed * dt));
            break;
        }
        case ThreatKind::VagalBradycardia: {
            float pulse = 0.6f + 0.4f * sinf(bobPhase * 0.8f);
            pos = Vector2Add(pos, Vector2Scale(dir, curSpeed * pulse * dt));
            break;
        }
        case ThreatKind::PONV: {
            Vector2 perp = { -dir.y, dir.x };
            Vector2 wander = Vector2Add(dir, Vector2Scale(perp, sinf(bobPhase * 2.0f) * 0.8f));
            pos = Vector2Add(pos, Vector2Scale(Vector2Normalize(wander), curSpeed * dt));
            break;
        }
        case ThreatKind::SeizureFocus: {
            Vector2 twitch = { RandRangef(-14.0f, 14.0f), RandRangef(-14.0f, 14.0f) };
            Vector2 net = Vector2Add(Vector2Scale(dir, curSpeed), twitch);
            pos = Vector2Add(pos, Vector2Scale(net, dt));
            break;
        }
        case ThreatKind::ICPSurge: {
            // Boss: slow, implacable, unaffected by slow effects (raised ICP doesn't
            // simply "wear off").
            pos = Vector2Add(pos, Vector2Scale(dir, speed * dt));
            break;
        }
        default: {
            pos = Vector2Add(pos, Vector2Scale(dir, curSpeed * dt));
            break;
        }
    }
}

float Enemy::DistanceToCenter(Vector2 center) const {
    return Vector2Distance(pos, center);
}

void Enemy::Draw() const {
    const EnemyTypeInfo& info = GetEnemyTypeInfo(kind);
    Color c = info.color;
    if (empowerTimer > 0) c = ColorLerp(c, WHITE, 0.4f * (sinf(GetTime() * 20.0f) * 0.5f + 0.5f));
    float bob = sinf(bobPhase) * 2.0f;
    Vector2 drawPos = { pos.x, pos.y + bob };

    if (spawnFlashTimer > 0) {
        float t = spawnFlashTimer / 0.35f;
        DrawCircleV(drawPos, radius + t * 18.0f, Fade(c, t * 0.5f));
    }

    DrawCircleV(drawPos, radius, Fade(c, 0.92f));
    DrawCircleLines((int)drawPos.x, (int)drawPos.y, radius, Fade(WHITE, 0.35f));
    if (slowTimer > 0) DrawCircleLines((int)drawPos.x, (int)drawPos.y, radius + 3, Fade(Pal::MONITOR_BLU, 0.8f));

    // HP arc
    float pct = Clampf(hp / maxHp, 0.0f, 1.0f);
    DrawRing(drawPos, radius + 4, radius + 6, -90, -90 + 360 * pct, 24, Fade(Pal::MONITOR_GRN, 0.9f));

    if (isBoss) {
        int w = MeasureText(info.name.c_str(), 14);
        DrawText(info.name.c_str(), (int)drawPos.x - w / 2, (int)drawPos.y - radius - 22, 14, Pal::TEXT_BRIGHT);
    }
}
