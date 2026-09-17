// =====================================================================================
//  Game.cpp -- see Game.h. This file wires together the pharmacology data (DrugData),
//  the threat archetypes (Enemy), the bolus/AoE system (Projectile), the patient model
//  (Vitals) and the eight-scenario campaign (Level) into a playable top-down area
//  defense loop.
// =====================================================================================
#include "Game.h"
#include <cstdio>

// -------------------------------------------------------------------------------------
static float ComputeBolusCost(const DrugProfile& p) {
    if (p.doseHighMgKg <= 0.001f) return 9.0f; // dexmedetomidine: loading-infusion increment, not a bolus dose
    return Clampf(p.doseHighMgKg * 9.5f, 6.0f, 42.0f);
}

static float ComputeBloomRadius(const DrugProfile& p, float cost) {
    return Clampf(48.0f + cost * 0.55f, 46.0f, 95.0f);
}

static float ComputeLingerSeconds(const DrugProfile& p) {
    return Clampf(p.durationLowMin * 0.12f + 1.0f, 1.2f, 7.0f);
}

// =======================================================================================
Game::Game() {
    for (size_t i = 0; i < drugs_.size(); i++) {
        drugs_[i].budget = 100.0f;
        drugs_[i].budgetMax = 100.0f;
    }
    audio_.Init();
}

Game::~Game() {
    audio_.Shutdown();
}

void Game::StartLevel(int idx) {
    const auto& campaign = GetCampaign();
    levelIndex_ = Clampf((float)idx, 0.0f, (float)campaign.size() - 1.0f);
    level_ = campaign[levelIndex_];
    ResetRuntimeForLevel();
    screen_ = GameScreen::BRIEFING;
    screenTimer_ = 0.0f;
}

void Game::ResetRuntimeForLevel() {
    levelTime_ = 0.0f;
    nextEventIdx_ = 0;
    enemies_.clear();
    boluses_.clear();
    particles_.Clear();
    vitals_ = Vitals{};
    score_ = 0;
    enemiesCleared_ = 0;
    enemiesReachedCenter_ = 0;
    levelWon_ = false;
    failReason_.clear();
    lastToast_.clear();
    toastTimer_ = 0.0f;
    firstSeizureKillDrug_ = DrugID::COUNT;
    anySeizureKilled_ = false;
    minMapSeen_ = 999.0f; maxMapSeen_ = 0.0f;
    minHrSeen_ = 999.0f; maxHrSeen_ = 0.0f;
    maxIcpSeen_ = 0.0f;
    peakDelirium_ = 0.0f;
    ectObjectiveActive_ = level_.rewardsSeizureNotKillsIt;
    ectObjectiveDone_ = false;
    ectMeter_ = 40.0f;
    ectPos_ = ArenaCenter();

    for (size_t i = 0; i < drugs_.size(); i++) {
        drugs_[i] = DrugRuntime{};
        drugs_[i].budget = 100.0f * level_.startingBudgetMul;
        drugs_[i].budgetMax = 100.0f * level_.startingBudgetMul;
    }
    selectedDrug_ = DrugID::Propofol;
}

// ---------------------------------------------------------------------------- Update
void Game::Update(float dt) {
    screenTimer_ += dt;
    if (toastTimer_ > 0) toastTimer_ -= dt;

    switch (screen_) {
        case GameScreen::MENU: {
            int prevSel = menuSelection_;
            if (IsKeyPressed(KEY_DOWN)) menuSelection_ = (menuSelection_ + 1) % 3;
            if (IsKeyPressed(KEY_UP))   menuSelection_ = (menuSelection_ + 2) % 3;
            if (menuSelection_ != prevSel) audio_.PlayUiMove();
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (menuSelection_ == 0) StartLevel(0);
                else if (menuSelection_ == 1) { screen_ = GameScreen::CODEX; codexTab_ = 0; codexDrugIndex_ = 0; }
                else { screen_ = GameScreen::SETTINGS; settingsSelection_ = 0; }
            }
            if (IsKeyPressed(KEY_C)) { screen_ = GameScreen::CODEX; codexTab_ = 0; codexDrugIndex_ = 0; }
            break;
        }
        case GameScreen::SETTINGS: {
            int prevSel = settingsSelection_;
            if (IsKeyPressed(KEY_DOWN)) settingsSelection_ = (settingsSelection_ + 1) % 3;
            if (IsKeyPressed(KEY_UP))   settingsSelection_ = (settingsSelection_ + 2) % 3;
            if (settingsSelection_ != prevSel) audio_.PlayUiMove();
            bool left = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
            bool right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
            if (settingsSelection_ == 0 && (left || right)) {
                volume_ = Clampf(volume_ + (right ? 0.1f : -0.1f), 0.0f, 1.0f);
            } else if (settingsSelection_ == 1 && (left || right)) {
                difficulty_ = Clampf(difficulty_ + (right ? 0.1f : -0.1f), 0.7f, 1.4f);
            } else if (settingsSelection_ == 2 && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || left || right)) {
                muted_ = !muted_;
            }
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) screen_ = GameScreen::MENU;
            break;
        }
        case GameScreen::CODEX: {
            if (IsKeyPressed(KEY_TAB)) codexTab_ = (codexTab_ + 1) % 3;
            if (codexTab_ == 0) {
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) codexDrugIndex_ = (codexDrugIndex_ + 1) % (int)DrugID::COUNT;
                if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) codexDrugIndex_ = (codexDrugIndex_ + (int)DrugID::COUNT - 1) % (int)DrugID::COUNT;
            } else if (codexTab_ == 1) {
                int n = (int)GetConceptEntries().size();
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) codexConceptIndex_ = (codexConceptIndex_ + 1) % n;
                if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) codexConceptIndex_ = (codexConceptIndex_ + n - 1) % n;
            }
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) screen_ = GameScreen::MENU;
            break;
        }
        case GameScreen::BRIEFING: {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) screen_ = GameScreen::PLAYING;
            if (IsKeyPressed(KEY_ESCAPE)) screen_ = GameScreen::MENU;
            break;
        }
        case GameScreen::PLAYING: {
            UpdatePlaying(dt);
            break;
        }
        case GameScreen::PAUSED: {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) screen_ = GameScreen::PLAYING;
            if (IsKeyPressed(KEY_Q)) screen_ = GameScreen::MENU;
            break;
        }
        case GameScreen::LEVEL_CLEAR: {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (levelIndex_ + 1 >= (int)GetCampaign().size()) screen_ = GameScreen::GAME_WIN;
                else StartLevel(levelIndex_ + 1);
            }
            break;
        }
        case GameScreen::LEVEL_FAIL: {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) StartLevel(levelIndex_);
            if (IsKeyPressed(KEY_ESCAPE)) screen_ = GameScreen::MENU;
            break;
        }
        case GameScreen::GAME_WIN: {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) screen_ = GameScreen::MENU;
            break;
        }
    }
}

void Game::UpdatePlaying(float dt) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) { screen_ = GameScreen::PAUSED; return; }

    HandleInput();

    levelTime_ += dt;
    SpawnScheduledEvents(dt);
    UpdateBoluses(dt);
    UpdateEnemies(dt);
    ResolveBolusEnemyCollisions();
    UpdateDrugResources(dt);
    UpdateECTObjective();
    vitals_.Update(dt);
    particles_.Update(dt);
    UpdateStatTracking();
    audio_.Update(dt, vitals_.heartRate, vitals_.InDanger(), muted_, volume_);

    // remove dead enemies
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
        [](const Enemy& e) { return !e.alive; }), enemies_.end());
    boluses_.erase(std::remove_if(boluses_.begin(), boluses_.end(),
        [](const Bolus& b) { return b.IsDone(); }), boluses_.end());

    CheckLevelEndConditions();
}

// ---------------------------------------------------------------------------- Input
void Game::HandleInput() {
    for (int i = 0; i < (int)DrugID::COUNT; i++) {
        if (IsKeyPressed(KEY_ONE + i)) selectedDrug_ = (DrugID)i;
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        int cur = (int)selectedDrug_;
        cur = (cur + (wheel > 0 ? 1 : -1) + (int)DrugID::COUNT) % (int)DrugID::COUNT;
        selectedDrug_ = (DrugID)cur;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 m = GetMousePosition();
        Vector2 c = ArenaCenter();
        Vector2 rel = Vector2Subtract(m, c);
        float len = Vector2Length(rel);
        if (len > ARENA_RADIUS) rel = Vector2Scale(rel, ARENA_RADIUS / len);
        FireBolus(Vector2Add(c, rel));
    }
}

void Game::FireBolus(Vector2 targetPos) {
    DrugRuntime& r = drugs_[(size_t)selectedDrug_];
    const DrugProfile& p = GetDrug(selectedDrug_);
    float cost = ComputeBolusCost(p);

    if (r.cooldown > 0.0f) { lastToast_ = p.name + " still being drawn up..."; toastTimer_ = 1.2f; return; }
    if (r.budget < cost)   { lastToast_ = p.name + " vial depleted -- wait for clearance."; toastTimer_ = 1.2f; return; }

    r.budget -= cost;
    r.cooldown = p.onsetSeconds * 1.6f + 0.12f;
    r.load += cost * 0.75f;
    r.timesFired++;
    audio_.PlayFire();

    float intensity = cost / 20.0f;
    vitals_.ApplyDrugSideEffects(p, intensity);

    Vector2 ivSite = { ArenaCenter().x, ArenaCenter().y + ARENA_RADIUS - 14.0f };
    Bolus b;
    b.Init(selectedDrug_, ivSite, targetPos, p.onsetSeconds, ComputeBloomRadius(p, cost), ComputeLingerSeconds(p), intensity);
    boluses_.push_back(b);
}

// ---------------------------------------------------------------------------- Spawning
void Game::SpawnScheduledEvents(float dt) {
    while (nextEventIdx_ < level_.events.size() && level_.events[nextEventIdx_].time <= levelTime_) {
        const SpawnEvent& ev = level_.events[nextEventIdx_];
        for (int i = 0; i < ev.count; i++) SpawnEnemy(ev, i);
        nextEventIdx_++;
    }
}

void Game::SpawnEnemy(const SpawnEvent& ev, int indexInBatch) {
    Vector2 c = ArenaCenter();
    float ang = RandRangef(0.0f, 2.0f * PI) + indexInBatch * 0.35f;
    Vector2 spawnPos = { c.x + cosf(ang) * (ARENA_RADIUS + 10.0f), c.y + sinf(ang) * (ARENA_RADIUS + 10.0f) };
    Enemy e;
    e.Init(ev.kind, spawnPos, ev.hpMul * difficulty_, ev.speedMul * (0.85f + difficulty_ * 0.15f), ev.boss);
    enemies_.push_back(e);
    if (ev.boss) { lastToast_ = "! " + GetEnemyTypeInfo(ev.kind).name + " incoming !"; toastTimer_ = 2.5f; }
}

// ---------------------------------------------------------------------------- Update world
void Game::UpdateBoluses(float dt) {
    Vector2 c = ArenaCenter();
    for (auto& b : boluses_) {
        BolusState prev = b.state;
        b.Update(dt);
        if (prev == BolusState::Traveling && b.state != BolusState::Traveling) {
            particles_.Emit(b.target, GetDrug(b.drug).color, 10, 90.0f, 0.4f, 4.0f);
            // one-shot ECT objective interaction on detonation
            if (level_.rewardsSeizureNotKillsIt && ectObjectiveActive_ && !ectObjectiveDone_) {
                if (Vector2Distance(b.target, ectPos_) < b.maxRadius) {
                    const DrugProfile& p = GetDrug(b.drug);
                    if (p.goodForECT) {
                        ectMeter_ = Clampf(ectMeter_ + 30.0f * b.doseIntensity, 0.0f, 100.0f);
                        lastToast_ = p.name + " prolongs the therapeutic seizure.";
                    } else if (p.anticonvulsant) {
                        ectMeter_ = Clampf(ectMeter_ - 42.0f * b.doseIntensity, 0.0f, 100.0f);
                        lastToast_ = p.name + " cut the seizure short!";
                    } else {
                        ectMeter_ = Clampf(ectMeter_ - 6.0f, 0.0f, 100.0f);
                    }
                    toastTimer_ = 1.4f;
                }
            }
        }
    }
    (void)c;
}

void Game::UpdateEnemies(float dt) {
    Vector2 c = ArenaCenter();
    for (auto& e : enemies_) {
        if (!e.alive) continue;
        e.Update(dt, c);
        float d = e.DistanceToCenter(c);
        const EnemyTypeInfo& info = GetEnemyTypeInfo(e.kind);
        if (d < e.radius + 10.0f) {
            if (e.isBoss) {
                vitals_.ApplyThreatDamage(e.kind, info.damageOnReach * dt * 6.0f);
                // gently push the boss back out so it doesn't sit exactly on the patient sprite
                Vector2 away = Vector2Scale(Vector2Normalize(Vector2Subtract(e.pos, c)), 40.0f * dt);
                e.pos = Vector2Add(e.pos, away);
            } else {
                vitals_.ApplyThreatDamage(e.kind, info.damageOnReach);
                enemiesReachedCenter_++;
                particles_.Emit(c, Pal::MONITOR_RED, 14, 120.0f, 0.5f, 4.0f);
                e.alive = false;
            }
        }
    }
}

void Game::ResolveBolusEnemyCollisions() {
    const float TICK_INTERVAL = 0.18f;
    for (auto& b : boluses_) {
        if (b.state != BolusState::Blooming && b.state != BolusState::Lingering) continue;
        if (b.tickTimer < TICK_INTERVAL) continue;
        b.tickTimer = 0.0f;
        float radius = (b.state == BolusState::Blooming) ? b.bloomRadius : b.maxRadius;
        for (auto& e : enemies_) {
            if (!e.alive) continue;
            if (Vector2Distance(e.pos, b.target) <= radius + e.radius) {
                ApplyDamageToEnemy(e, b.drug, b.doseIntensity);
            }
        }
    }
}

float Game::EffectivenessOverride(DrugID drug, ThreatKind kind, bool isBoss) const {
    float base = DrugEffectivenessVsThreat(drug, kind);
    // Level 8 teaching moment: a large aortic-dissection outcomes registry found no
    // neuroprotective benefit from barbiturates -- so they under-perform here specifically.
    if (isBoss && levelIndex_ == 7 && kind == ThreatKind::ICPSurge &&
        (drug == DrugID::Thiopental || drug == DrugID::Methohexital)) {
        base *= 0.08f;
    }
    return base;
}

void Game::ApplyDamageToEnemy(Enemy& e, DrugID drug, float intensity) {
    const DrugProfile& p = GetDrug(drug);
    float backfire = DrugBackfireMultiplier(drug, e.kind);
    if (backfire > 1.0f) {
        if (e.empowerTimer <= 0.0f) {
            e.Empower(backfire, 3.0f);
            particles_.Emit(e.pos, Pal::MONITOR_RED, 16, 100.0f, 0.5f, 5.0f);
            lastToast_ = p.name + " backfires against a " + GetEnemyTypeInfo(e.kind).name + "!";
            toastTimer_ = 1.6f;
            audio_.PlayBackfire();
        }
        return;
    }
    float eff = EffectivenessOverride(drug, e.kind, e.isBoss);
    if (eff <= 0.02f) return;
    float dmg = 15.0f * eff * (0.6f + intensity * 0.6f);
    e.TakeDamage(dmg);
    if (!e.alive) {
        enemiesCleared_++;
        score_ += e.isBoss ? 500 : 20;
        particles_.Emit(e.pos, GetEnemyTypeInfo(e.kind).color, 20, 140.0f, 0.55f, 4.0f);
        audio_.PlayKill();
        if (e.kind == ThreatKind::SeizureFocus) {
            anySeizureKilled_ = true;
            if (firstSeizureKillDrug_ == DrugID::COUNT) firstSeizureKillDrug_ = drug;
        }
    } else {
        particles_.Emit(e.pos, Fade(p.color, 0.8f), 3, 40.0f, 0.2f, 2.5f);
        audio_.PlayHit();
    }
}

void Game::UpdateDrugResources(float dt) {
    const auto& roster = GetDrugRoster();
    for (size_t i = 0; i < drugs_.size(); i++) {
        DrugRuntime& r = drugs_[i];
        const DrugProfile& p = roster[i];
        r.cooldown = std::max(0.0f, r.cooldown - dt);
        float regen = p.clearanceMlKgMin * 0.11f + 0.6f;
        r.budget = Clampf(r.budget + regen * dt, 0.0f, r.budgetMax);
        float loadDecay = p.clearanceMlKgMin * 0.15f + 0.4f;
        r.load = std::max(0.0f, r.load - loadDecay * dt);

        if (r.load > 45.0f) {
            float driftIntensity = ((r.load - 45.0f) / 55.0f) * p.contextHalfTimeFactor;
            vitals_.respRate -= driftIntensity * dt * 2.4f;
            vitals_.delirium = Clampf(vitals_.delirium + driftIntensity * dt * 7.0f, 0.0f, 100.0f);
        }
    }
}

void Game::UpdateECTObjective() {
    if (!level_.rewardsSeizureNotKillsIt || !ectObjectiveActive_ || ectObjectiveDone_) return;
    ectMeter_ = Clampf(ectMeter_ - GetFrameTime() * 2.0f, 0.0f, 100.0f);
    if (ectMeter_ >= 100.0f) {
        ectObjectiveDone_ = true;
        lastToast_ = "Adequate seizure duration achieved -- safe to terminate.";
        toastTimer_ = 2.0f;
    }
}

void Game::UpdateStatTracking() {
    minMapSeen_ = std::min(minMapSeen_, vitals_.map);
    maxMapSeen_ = std::max(maxMapSeen_, vitals_.map);
    minHrSeen_  = std::min(minHrSeen_, vitals_.heartRate);
    maxHrSeen_  = std::max(maxHrSeen_, vitals_.heartRate);
    maxIcpSeen_ = std::max(maxIcpSeen_, vitals_.icp);
    peakDelirium_ = std::max(peakDelirium_, vitals_.delirium);
}

std::string Game::ComputeGrade() const {
    // A simple, transparent rubric: start at 100 and deduct for breaches, extremes,
    // and drift/delirium accumulation, rewarding steady, well-matched drug choices
    // over brute-forcing a level with a single overused agent.
    float points = 100.0f;
    points -= enemiesReachedCenter_ * 6.0f;
    if (maxIcpSeen_ > 25.0f) points -= (maxIcpSeen_ - 25.0f) * 1.5f;
    if (minMapSeen_ < 55.0f) points -= (55.0f - minMapSeen_) * 0.8f;
    if (maxMapSeen_ > 140.0f) points -= (maxMapSeen_ - 140.0f) * 0.8f;
    if (minHrSeen_ < 45.0f) points -= (45.0f - minHrSeen_) * 0.9f;
    if (maxHrSeen_ > 160.0f) points -= (maxHrSeen_ - 160.0f) * 0.6f;
    points -= peakDelirium_ * 0.25f;
    points = Clampf(points, 0.0f, 100.0f);

    if (points >= 90.0f) return "A -- textbook management";
    if (points >= 75.0f) return "B -- solid, minor rough patches";
    if (points >= 55.0f) return "C -- patient survived, but it was close";
    if (points >= 35.0f) return "D -- several avoidable decompensations";
    return "F -- review the Codex before the next case";
}

void Game::CheckLevelEndConditions() {
    if (!vitals_.alive) {
        failReason_ = vitals_.WorstVitalLabel() + " became unrecoverable.";
        screen_ = GameScreen::LEVEL_FAIL;
        audio_.PlayLevelFail();
        return;
    }
    if (level_.rewardsSeizureNotKillsIt) {
        if (ectObjectiveDone_) { levelWon_ = true; screen_ = GameScreen::LEVEL_CLEAR; audio_.PlayLevelClear(); }
        return;
    }
    bool allSpawned = nextEventIdx_ >= level_.events.size();
    bool noneAlive = enemies_.empty();
    if (allSpawned && noneAlive) {
        levelWon_ = true;
        screen_ = GameScreen::LEVEL_CLEAR;
        audio_.PlayLevelClear();
    }
}
