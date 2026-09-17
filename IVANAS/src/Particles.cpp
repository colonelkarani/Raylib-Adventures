#include "Particles.h"

void ParticleSystem::Emit(Vector2 pos, Color color, int count, float speed, float life, float size) {
    for (int i = 0; i < count; i++) {
        Particle* slot = nullptr;
        for (auto& p : pool_) if (!p.alive) { slot = &p; break; }
        if (!slot) { pool_.emplace_back(); slot = &pool_.back(); }

        float ang = RandRangef(0.0f, 2.0f * PI);
        float spd = RandRangef(speed * 0.4f, speed);
        slot->pos = pos;
        slot->vel = { cosf(ang) * spd, sinf(ang) * spd };
        slot->life = 0.0f;
        slot->maxLife = life * RandRangef(0.7f, 1.3f);
        slot->size = size * RandRangef(0.6f, 1.2f);
        slot->color = color;
        slot->alive = true;
    }
}

void ParticleSystem::Update(float dt) {
    for (auto& p : pool_) {
        if (!p.alive) continue;
        p.life += dt;
        if (p.life >= p.maxLife) { p.alive = false; continue; }
        p.pos = Vector2Add(p.pos, Vector2Scale(p.vel, dt));
        p.vel = Vector2Scale(p.vel, 0.94f);
    }
}

void ParticleSystem::Draw() const {
    for (const auto& p : pool_) {
        if (!p.alive) continue;
        float t = 1.0f - (p.life / p.maxLife);
        DrawCircleV(p.pos, p.size * t, Fade(p.color, t));
    }
}

void ParticleSystem::Clear() {
    for (auto& p : pool_) p.alive = false;
}
