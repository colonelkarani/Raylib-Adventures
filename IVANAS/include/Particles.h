// =====================================================================================
//  Particles.h -- lightweight cosmetic particle pool (hit sparks, defeat bursts,
//  monitor blips). No gameplay logic lives here on purpose.
// =====================================================================================
#pragma once
#include "Common.h"

struct Particle {
    Vector2 pos{}, vel{};
    float life = 0.0f, maxLife = 0.5f;
    float size = 3.0f;
    Color color = WHITE;
    bool alive = false;
};

class ParticleSystem {
public:
    void Emit(Vector2 pos, Color color, int count, float speed, float life, float size);
    void Update(float dt);
    void Draw() const;
    void Clear();
private:
    std::vector<Particle> pool_;
};
