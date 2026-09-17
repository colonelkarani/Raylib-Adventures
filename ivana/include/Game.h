// =====================================================================================
//  Game.h -- top-level game state machine: menu, codex, level briefing, active play,
//  level clear/fail, and campaign win. Owns all live entities for the current level.
// =====================================================================================
#pragma once
#include "Common.h"
#include "DrugData.h"
#include "Vitals.h"
#include "Enemy.h"
#include "Projectile.h"
#include "Particles.h"
#include "Level.h"
#include "Codex.h"
#include "Audio.h"

struct DrugRuntime {
    float budget = 100.0f;
    float budgetMax = 100.0f;
    float cooldown = 0.0f;
    float load = 0.0f;          // accumulated "drug on board" -- context-sensitive half-time proxy
    int   timesFired = 0;
};

class Game {
public:
    Game();
    ~Game();
    void Update(float dt);
    void Draw();

private:
    // ---- Screens ----
    GameScreen screen_ = GameScreen::MENU;
    int levelIndex_ = 0;
    int menuSelection_ = 0;
    int codexTab_ = 0;          // 0 = drugs, 1 = concepts
    int codexDrugIndex_ = 0;
    int codexConceptIndex_ = 0;
    float screenTimer_ = 0.0f;

    // ---- Settings ----
    AudioManager audio_;
    float volume_ = 0.6f;
    bool  muted_ = false;
    float difficulty_ = 1.0f;   // 0.8 = gentler, 1.0 = standard, 1.3 = harder
    int   settingsSelection_ = 0;

    // ---- Level runtime ----
    LevelDef level_;
    float levelTime_ = 0.0f;
    size_t nextEventIdx_ = 0;
    std::vector<Enemy> enemies_;
    std::vector<Bolus> boluses_;
    ParticleSystem particles_;
    Vitals vitals_;
    std::array<DrugRuntime, (size_t)DrugID::COUNT> drugs_;
    DrugID selectedDrug_ = DrugID::Propofol;
    int score_ = 0;
    int enemiesCleared_ = 0;
    int enemiesReachedCenter_ = 0;
    bool levelWon_ = false;
    std::string failReason_;
    std::string lastToast_;
    float toastTimer_ = 0.0f;
    DrugID firstSeizureKillDrug_ = DrugID::COUNT;
    bool anySeizureKilled_ = false;

    // ---- Clinical-report stat tracking (for the post-level feedback panel) ----
    float minMapSeen_ = 999.0f, maxMapSeen_ = 0.0f;
    float minHrSeen_ = 999.0f, maxHrSeen_ = 0.0f;
    float maxIcpSeen_ = 0.0f;
    float peakDelirium_ = 0.0f;

    // ECT special objective (Level index 4)
    bool  ectObjectiveActive_ = false;
    bool  ectObjectiveDone_ = false;
    float ectMeter_ = 0.0f;      // 0..100
    Vector2 ectPos_{};

    // ---- Helpers ----
    void StartLevel(int idx);
    void ResetRuntimeForLevel();
    void UpdatePlaying(float dt);
    void SpawnScheduledEvents(float dt);
    void SpawnEnemy(const SpawnEvent& ev, int indexInBatch);
    void HandleInput();
    void FireBolus(Vector2 targetPos);
    void UpdateBoluses(float dt);
    void UpdateEnemies(float dt);
    void ResolveBolusEnemyCollisions();
    void ApplyDamageToEnemy(Enemy& e, DrugID drug, float dt);
    void UpdateDrugResources(float dt);
    void UpdateECTObjective();
    void CheckLevelEndConditions();
    void UpdateStatTracking();
    std::string ComputeGrade() const;
    float EffectivenessOverride(DrugID drug, ThreatKind kind, bool isBoss) const;

    void DrawMenu();
    void DrawCodex();
    void DrawBriefing();
    void DrawPlaying();
    void DrawPaused();
    void DrawSettings();
    void DrawLevelClear();
    void DrawLevelFail();
    void DrawGameWin();
    void DrawHUD();
    void DrawVitalsPanel();
    void DrawHotbar();
    void DrawArenaBackdrop();
    void DrawToast();
    void DrawClinicalReport(Rectangle panel) const;

    Vector2 ArenaCenter() const { return ARENA_CENTER(); }
};
