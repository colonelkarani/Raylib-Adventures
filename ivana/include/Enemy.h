// =====================================================================================
//  Enemy.h -- physiological "threats" advancing on the patient at the center of the
//  field. Each archetype corresponds to a real perioperative problem discussed in the
//  source chapter; see ThreatKind in Common.h and DrugEffectivenessVsThreat() in
//  DrugData.cpp for how each drug interacts with each archetype.
// =====================================================================================
#pragma once
#include "Common.h"

struct EnemyTypeInfo {
    ThreatKind  kind;
    std::string name;
    std::string flavor;
    Color       color;
    float       baseHp;
    float       baseSpeed;
    float       radius;
    float       damageOnReach; // damage applied to the relevant vital if it reaches center
};

const EnemyTypeInfo& GetEnemyTypeInfo(ThreatKind k);

struct Enemy {
    ThreatKind kind;
    Vector2 pos{};
    float hp = 1.0f, maxHp = 1.0f;
    float speed = 1.0f;
    float radius = 12.0f;
    bool  alive = true;
    bool  isBoss = false;
    float slowTimer = 0.0f;   // >0 while under a temporary drug-induced slow
    float empowerTimer = 0.0f; // >0 while backfired/empowered (visual + stat boost)
    float spawnFlashTimer = 0.35f;
    float bobPhase = 0.0f;

    void Init(ThreatKind k, Vector2 spawnPos, float hpMul, float speedMul, bool boss = false);
    void Update(float dt, Vector2 target);
    void Draw() const;
    float DistanceToCenter(Vector2 center) const;
    void TakeDamage(float amount) { hp -= amount; if (hp <= 0) alive = false; }
    void Empower(float mult, float seconds) { hp *= mult; maxHp *= mult; empowerTimer = seconds; }
};
