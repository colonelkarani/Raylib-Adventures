/*
    ANESTHESIA AREA DEFENSE
    -----------------------
    Educational 2D top-down Raylib C++ game.

    Build:
      g++ anesthesia_area_defense_raylib.cpp -o anesthesia_game \
          -lraylib -lopengl32 -lgdi32 -lwinmm

    Linux:
      g++ anesthesia_area_defense_raylib.cpp -o anesthesia_game \
          -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

    IMPORTANT:
      This is an educational game, NOT a clinical decision-support system.
      It deliberately avoids real patient-specific dosing and treatment orders.

    CORE CONCEPT:
      The "enemies" are adverse physiologic states and competing anesthesia
      requirements. The player is an anesthesia team protecting a simulated
      patient state while trying to maintain a balanced anesthetic.

    Concepts represented from the supplied source:
      * IV nonopioid anesthetics are used for induction and sedation.
      * Balanced anesthesia combines smaller amounts of multiple drug classes
        because no single IV drug produces every desirable endpoint.
      * IV induction drugs reach highly perfused lipid-rich tissues rapidly.
      * Effect after a bolus can terminate substantially through redistribution.
      * Propofol: hypnotic; rapid onset; hepatic metabolism; IV induction/
        maintenance/sedation; milky emulsion.
      * Benzodiazepines: amnesia/anxiolysis/sedation; reduce ventilatory
        response to CO2; respiratory depression becomes more significant with
        opioids; flumazenil may reverse delayed awakening but is short acting.
      * Ketamine, etomidate and dexmedetomidine have distinct profiles.
      * Sterile handling matters because propofol formulations can support
        bacterial growth.

    The game uses abstract "effect points" rather than clinical doses.
*/

#include "raylib.h"

// Custom UI color used by the ventilation monitor.
static const Color TEAL = {0, 200, 180, 255};
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>

using std::string;
using std::vector;
using std::array;

static const int SCREEN_W = 1280;
static const int SCREEN_H = 720;
static const int ARENA_X = 28;
static const int ARENA_Y = 116;
static const int ARENA_W = 820;
static const int ARENA_H = 560;

static float ClampF(float x, float a, float b) {
    return std::max(a, std::min(b, x));
}

static Vector2 V2(float x, float y) {
    return Vector2{x, y};
}

static float Dist(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

static Vector2 Normalize(Vector2 v) {
    float d = std::sqrt(v.x * v.x + v.y * v.y);
    if (d < 0.0001f) return V2(0, 0);
    return V2(v.x / d, v.y / d);
}

static Vector2 Add(Vector2 a, Vector2 b) {
    return V2(a.x + b.x, a.y + b.y);
}

static Vector2 Sub(Vector2 a, Vector2 b) {
    return V2(a.x - b.x, a.y - b.y);
}

static Vector2 Mul(Vector2 a, float s) {
    return V2(a.x * s, a.y * s);
}

enum class GameScreen {
    TITLE,
    PLAYING,
    PAUSED,
    EDUCATION,
    GAME_OVER,
    VICTORY
};

enum class EnemyKind {
    HYPNOSIS,
    PAIN,
    AWARENESS,
    MOVEMENT,
    APNEA,
    HYPOTENSION,
    BRADYCARDIA,
    EMERGENCE
};

enum class Intervention {
    PROPOFOL,
    MIDAZOLAM,
    KETAMINE,
    ETOMIDATE,
    DEXMEDETOMIDINE,
    OXYGEN_SUPPORT,
    VENTILATION_SUPPORT,
    REVERSAL_FLUMAZENIL,
    BALANCE_ANESTHESIA
};

struct Enemy {
    EnemyKind kind;
    Vector2 pos;
    Vector2 vel;
    float radius;
    float hp;
    float maxHp;
    float speed;
    float damage;
    float spawnTime;
    float hitFlash;
    bool alive;
    bool elite;
};

struct Projectile {
    Vector2 pos;
    Vector2 vel;
    float radius;
    float damage;
    Intervention source;
    float life;
    bool alive;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
};

struct TextPopup {
    Vector2 pos;
    string text;
    float life;
    float maxLife;
};

struct VitalState {
    float oxygen;
    float ventilation;
    float bloodPressure;
    float heartRate;
    float sedation;
    float amnesia;
    float analgesia;
    float immobility;
    float awarenessRisk;
    float painRisk;
    float movementRisk;
    float emergenceRisk;

    float sterileIntegrity;
    float airwayReserve;
    float hemodynamicReserve;
    float score;

    void Reset() {
        oxygen = 96;
        ventilation = 92;
        bloodPressure = 88;
        heartRate = 74;
        sedation = 20;
        amnesia = 10;
        analgesia = 15;
        immobility = 10;
        awarenessRisk = 8;
        painRisk = 15;
        movementRisk = 8;
        emergenceRisk = 4;
        sterileIntegrity = 100;
        airwayReserve = 100;
        hemodynamicReserve = 100;
        score = 0;
    }
};

struct DrugState {
    float propofol;
    float midazolam;
    float ketamine;
    float etomidate;
    float dexmedetomidine;
    float oxygenSupport;
    float ventilationSupport;
    float flumazenil;

    void Reset() {
        propofol = 0;
        midazolam = 0;
        ketamine = 0;
        etomidate = 0;
        dexmedetomidine = 0;
        oxygenSupport = 0;
        ventilationSupport = 0;
        flumazenil = 0;
    }
};

struct GameState {
    GameScreen screen = GameScreen::TITLE;
    VitalState vitals;
    DrugState drugs;

    Vector2 player;
    float playerSpeed;
    float fireCooldown;
    float globalTime;
    float waveTime;
    float waveDuration;
    float spawnTimer;
    int wave;
    int score;
    int combo;
    float comboTimer;

    int patientsSaved;
    int failures;
    int escapedThreats;

    bool sterileWarning;
    bool educationOverlay;
    int educationPage;

    vector<Enemy> enemies;
    vector<Projectile> projectiles;
    vector<Particle> particles;
    vector<TextPopup> popups;

    float shake;
    float damageFlash;
    float successFlash;

    void Reset() {
        screen = GameScreen::TITLE;
        vitals.Reset();
        drugs.Reset();
        player = V2(430, 390);
        playerSpeed = 240;
        fireCooldown = 0;
        globalTime = 0;
        waveTime = 0;
        waveDuration = 48;
        spawnTimer = 0;
        wave = 1;
        score = 0;
        combo = 0;
        comboTimer = 0;
        patientsSaved = 0;
        failures = 0;
        escapedThreats = 0;
        sterileWarning = false;
        educationOverlay = false;
        educationPage = 0;
        enemies.clear();
        projectiles.clear();
        particles.clear();
        popups.clear();
        shake = 0;
        damageFlash = 0;
        successFlash = 0;
    }
};

static GameState G;

static const char* EnemyName(EnemyKind k) {
    switch (k) {
        case EnemyKind::HYPNOSIS: return "Hypnosis gap";
        case EnemyKind::PAIN: return "Analgesia gap";
        case EnemyKind::AWARENESS: return "Awareness";
        case EnemyKind::MOVEMENT: return "Movement";
        case EnemyKind::APNEA: return "Ventilatory depression";
        case EnemyKind::HYPOTENSION: return "Hypotension";
        case EnemyKind::BRADYCARDIA: return "Bradycardia";
        case EnemyKind::EMERGENCE: return "Emergence";
    }
    return "Unknown";
}

static Color EnemyColor(EnemyKind k) {
    switch (k) {
        case EnemyKind::HYPNOSIS: return Color{180, 100, 240, 255};
        case EnemyKind::PAIN: return Color{235, 80, 90, 255};
        case EnemyKind::AWARENESS: return Color{245, 180, 55, 255};
        case EnemyKind::MOVEMENT: return Color{70, 170, 245, 255};
        case EnemyKind::APNEA: return Color{70, 210, 200, 255};
        case EnemyKind::HYPOTENSION: return Color{225, 115, 70, 255};
        case EnemyKind::BRADYCARDIA: return Color{110, 130, 230, 255};
        case EnemyKind::EMERGENCE: return Color{240, 70, 170, 255};
    }
    return WHITE;
}

static const char* InterventionName(Intervention i) {
    switch (i) {
        case Intervention::PROPOFOL: return "Propofol";
        case Intervention::MIDAZOLAM: return "Midazolam";
        case Intervention::KETAMINE: return "Ketamine";
        case Intervention::ETOMIDATE: return "Etomidate";
        case Intervention::DEXMEDETOMIDINE: return "Dexmedetomidine";
        case Intervention::OXYGEN_SUPPORT: return "Oxygen support";
        case Intervention::VENTILATION_SUPPORT: return "Ventilation support";
        case Intervention::REVERSAL_FLUMAZENIL: return "Flumazenil";
        case Intervention::BALANCE_ANESTHESIA: return "Balanced anesthesia";
    }
    return "Unknown";
}

static void AddPopup(Vector2 pos, const string& text, float duration = 1.4f) {
    TextPopup p;
    p.pos = pos;
    p.text = text;
    p.life = duration;
    p.maxLife = duration;
    G.popups.push_back(p);
}

static void AddBurst(Vector2 p, int count = 8) {
    for (int i = 0; i < count; ++i) {
        float a = ((float)std::rand() / RAND_MAX) * 2.0f * PI;
        float s = 25 + ((float)std::rand() / RAND_MAX) * 90;
        Particle q;
        q.pos = p;
        q.vel = V2(std::cos(a) * s, std::sin(a) * s);
        q.life = 0.45f + ((float)std::rand() / RAND_MAX) * 0.6f;
        q.maxLife = q.life;
        q.size = 2 + ((float)std::rand() / RAND_MAX) * 4;
        G.particles.push_back(q);
    }
}

static Color InterventionColor(Intervention i);

static void DrawCenteredText(const char* text, int y, int size, Color c) {
    int w = MeasureText(text, size);
    DrawText(text, SCREEN_W / 2 - w / 2, y, size, c);
}

static void DrawBar(Rectangle r, float value, float maxValue, Color fill, Color back) {
    DrawRectangleRec(r, back);
    float pct = maxValue <= 0 ? 0 : ClampF(value / maxValue, 0, 1);
    DrawRectangle((int)r.x, (int)r.y, (int)(r.width * pct), (int)r.height, fill);
    DrawRectangleLinesEx(r, 1, Fade(WHITE, 0.20f));
}

static void ResetGameToPlay() {
    G.Reset();
    G.screen = GameScreen::PLAYING;
    G.wave = 1;
    G.waveTime = 0;
}

static void SpawnEnemy() {
    Enemy e{};
    int side = std::rand() % 4;
    if (side == 0) e.pos = V2(ARENA_X + 10, ARENA_Y + 10 + std::rand() % (ARENA_H - 20));
    if (side == 1) e.pos = V2(ARENA_X + ARENA_W - 10, ARENA_Y + 10 + std::rand() % (ARENA_H - 20));
    if (side == 2) e.pos = V2(ARENA_X + 10 + std::rand() % (ARENA_W - 20), ARENA_Y + 10);
    if (side == 3) e.pos = V2(ARENA_X + 10 + std::rand() % (ARENA_W - 20), ARENA_Y + ARENA_H - 10);

    int r = std::rand() % 100;
    if (G.wave < 2) {
        if (r < 30) e.kind = EnemyKind::HYPNOSIS;
        else if (r < 55) e.kind = EnemyKind::PAIN;
        else if (r < 75) e.kind = EnemyKind::AWARENESS;
        else e.kind = EnemyKind::MOVEMENT;
    } else if (G.wave < 4) {
        if (r < 18) e.kind = EnemyKind::APNEA;
        else if (r < 35) e.kind = EnemyKind::HYPOTENSION;
        else if (r < 50) e.kind = EnemyKind::BRADYCARDIA;
        else if (r < 70) e.kind = EnemyKind::HYPNOSIS;
        else e.kind = EnemyKind::PAIN;
    } else {
        if (r < 15) e.kind = EnemyKind::APNEA;
        else if (r < 28) e.kind = EnemyKind::HYPOTENSION;
        else if (r < 40) e.kind = EnemyKind::BRADYCARDIA;
        else if (r < 55) e.kind = EnemyKind::EMERGENCE;
        else if (r < 70) e.kind = EnemyKind::AWARENESS;
        else e.kind = EnemyKind::PAIN;
    }

    e.elite = (G.wave >= 5 && std::rand() % 100 < 12);
    e.radius = e.elite ? 18 : 13;
    e.maxHp = e.elite ? 90 + G.wave * 8 : 42 + G.wave * 5;
    e.hp = e.maxHp;
    e.speed = e.elite ? 22 + G.wave * 1.5f : 30 + G.wave * 2.0f;
    e.damage = e.elite ? 7 : 3.5f;
    e.spawnTime = G.globalTime;
    e.hitFlash = 0;
    e.alive = true;

    G.enemies.push_back(e);
}

static Vector2 AimDirection() {
    Vector2 mouse = GetMousePosition();
    return Normalize(Sub(mouse, G.player));
}

static void Fire(Intervention type) {
    if (G.fireCooldown > 0) return;

    Vector2 dir = AimDirection();
    Projectile p;
    p.pos = Add(G.player, Mul(dir, 25));
    p.vel = Mul(dir, 470);
    p.radius = 4;
    p.source = type;
    p.life = 1.7f;
    p.alive = true;

    switch (type) {
        case Intervention::PROPOFOL: p.damage = 22; break;
        case Intervention::MIDAZOLAM: p.damage = 17; break;
        case Intervention::KETAMINE: p.damage = 24; break;
        case Intervention::ETOMIDATE: p.damage = 21; break;
        case Intervention::DEXMEDETOMIDINE: p.damage = 15; break;
        case Intervention::OXYGEN_SUPPORT: p.damage = 10; break;
        case Intervention::VENTILATION_SUPPORT: p.damage = 26; break;
        case Intervention::REVERSAL_FLUMAZENIL: p.damage = 20; break;
        case Intervention::BALANCE_ANESTHESIA: p.damage = 30; break;
    }

    G.projectiles.push_back(p);
    G.fireCooldown = 0.18f;
}

static void ApplyIntervention(Intervention i) {
    // These are abstract game effects, not dosing recommendations.
    switch (i) {
        case Intervention::PROPOFOL:
            G.drugs.propofol = ClampF(G.drugs.propofol + 12, 0, 100);
            G.vitals.sedation += 13;
            G.vitals.awarenessRisk -= 7;
            G.vitals.movementRisk -= 3;
            G.vitals.bloodPressure -= 2.2f;
            G.vitals.airwayReserve -= 2.5f;
            AddPopup(G.player, "Hypnosis / rapid onset", 1.1f);
            break;

        case Intervention::MIDAZOLAM:
            G.drugs.midazolam = ClampF(G.drugs.midazolam + 9, 0, 100);
            G.vitals.amnesia += 12;
            G.vitals.sedation += 8;
            G.vitals.ventilation -= 2.5f;
            G.vitals.awarenessRisk -= 5;
            AddPopup(G.player, "Amnesia + sedation", 1.1f);
            break;

        case Intervention::KETAMINE:
            G.drugs.ketamine = ClampF(G.drugs.ketamine + 10, 0, 100);
            G.vitals.analgesia += 12;
            G.vitals.sedation += 5;
            G.vitals.immobility += 5;
            G.vitals.bloodPressure += 1.5f;
            AddPopup(G.player, "Analgesia + dissociation", 1.1f);
            break;

        case Intervention::ETOMIDATE:
            G.drugs.etomidate = ClampF(G.drugs.etomidate + 10, 0, 100);
            G.vitals.sedation += 10;
            G.vitals.awarenessRisk -= 6;
            G.vitals.bloodPressure -= 0.4f;
            AddPopup(G.player, "Hypnotic support", 1.1f);
            break;

        case Intervention::DEXMEDETOMIDINE:
            G.drugs.dexmedetomidine = ClampF(G.drugs.dexmedetomidine + 10, 0, 100);
            G.vitals.sedation += 7;
            G.vitals.awarenessRisk -= 3;
            G.vitals.heartRate -= 2;
            G.vitals.bloodPressure -= 1;
            AddPopup(G.player, "Sedation / sympatholysis", 1.1f);
            break;

        case Intervention::OXYGEN_SUPPORT:
            G.drugs.oxygenSupport = ClampF(G.drugs.oxygenSupport + 16, 0, 100);
            G.vitals.oxygen += 12;
            AddPopup(G.player, "Oxygenation supported", 1.1f);
            break;

        case Intervention::VENTILATION_SUPPORT:
            G.drugs.ventilationSupport = ClampF(G.drugs.ventilationSupport + 18, 0, 100);
            G.vitals.ventilation += 15;
            G.vitals.oxygen += 4;
            G.vitals.airwayReserve += 5;
            AddPopup(G.player, "Ventilation supported", 1.1f);
            break;

        case Intervention::REVERSAL_FLUMAZENIL:
            G.drugs.flumazenil = ClampF(G.drugs.flumazenil + 20, 0, 100);
            G.drugs.midazolam = ClampF(G.drugs.midazolam - 18, 0, 100);
            G.vitals.amnesia -= 10;
            G.vitals.sedation -= 12;
            AddPopup(G.player, "Short-acting benzodiazepine reversal", 1.4f);
            break;

        case Intervention::BALANCE_ANESTHESIA:
            // Represents the educational concept rather than a prescription.
            G.vitals.sedation += 5;
            G.vitals.amnesia += 4;
            G.vitals.analgesia += 4;
            G.vitals.immobility += 4;
            G.vitals.awarenessRisk -= 4;
            G.vitals.painRisk -= 4;
            G.vitals.movementRisk -= 4;
            G.vitals.emergenceRisk -= 2;
            AddPopup(G.player, "Balanced anesthesia concept", 1.4f);
            break;
    }

    G.vitals.sedation = ClampF(G.vitals.sedation, 0, 100);
    G.vitals.amnesia = ClampF(G.vitals.amnesia, 0, 100);
    G.vitals.analgesia = ClampF(G.vitals.analgesia, 0, 100);
    G.vitals.immobility = ClampF(G.vitals.immobility, 0, 100);
    G.vitals.awarenessRisk = ClampF(G.vitals.awarenessRisk, 0, 100);
    G.vitals.painRisk = ClampF(G.vitals.painRisk, 0, 100);
    G.vitals.movementRisk = ClampF(G.vitals.movementRisk, 0, 100);
    G.vitals.oxygen = ClampF(G.vitals.oxygen, 0, 100);
    G.vitals.ventilation = ClampF(G.vitals.ventilation, 0, 100);
    G.vitals.bloodPressure = ClampF(G.vitals.bloodPressure, 0, 100);
    G.vitals.heartRate = ClampF(G.vitals.heartRate, 0, 140);
    G.vitals.airwayReserve = ClampF(G.vitals.airwayReserve, 0, 100);
}

static void UpdateDrugPharmacology(float dt) {
    // Abstract redistribution/metabolism model:
    // a single bolus effect declines with time and is not a clinical PK model.
    float propofolDecay = 0.90f * dt;
    float benzoDecay = 0.12f * dt;
    float ketDecay = 0.24f * dt;
    float etomidateDecay = 0.28f * dt;
    float dexDecay = 0.16f * dt;
    float supportDecay = 0.08f * dt;
    float flumDecay = 0.45f * dt;

    G.drugs.propofol = ClampF(G.drugs.propofol - propofolDecay, 0, 100);
    G.drugs.midazolam = ClampF(G.drugs.midazolam - benzoDecay, 0, 100);
    G.drugs.ketamine = ClampF(G.drugs.ketamine - ketDecay, 0, 100);
    G.drugs.etomidate = ClampF(G.drugs.etomidate - etomidateDecay, 0, 100);
    G.drugs.dexmedetomidine = ClampF(G.drugs.dexmedetomidine - dexDecay, 0, 100);
    G.drugs.oxygenSupport = ClampF(G.drugs.oxygenSupport - supportDecay, 0, 100);
    G.drugs.ventilationSupport = ClampF(G.drugs.ventilationSupport - supportDecay, 0, 100);
    G.drugs.flumazenil = ClampF(G.drugs.flumazenil - flumDecay, 0, 100);

    // Natural recovery toward baseline.
    G.vitals.sedation -= 2.2f * dt;
    G.vitals.amnesia -= 0.8f * dt;
    G.vitals.analgesia -= 1.2f * dt;
    G.vitals.immobility -= 1.1f * dt;

    // Drug-associated physiologic tendencies.
    G.vitals.ventilation += G.drugs.ventilationSupport * 0.030f * dt;
    G.vitals.oxygen += G.drugs.oxygenSupport * 0.025f * dt;

    // Benzodiazepine ventilatory effect.
    G.vitals.ventilation -= G.drugs.midazolam * 0.020f * dt;

    // Propofol can reduce BP/airway reserve in this simplified model.
    G.vitals.bloodPressure += 0.08f * dt;
    G.vitals.heartRate += 0.25f * dt;
    G.vitals.airwayReserve += 0.7f * dt;

    if (G.drugs.propofol > 20) {
        G.vitals.bloodPressure -= 0.16f * dt;
        G.vitals.airwayReserve -= 0.10f * dt;
    }

    if (G.drugs.dexmedetomidine > 25) {
        G.vitals.heartRate -= 0.10f * dt;
        G.vitals.bloodPressure -= 0.05f * dt;
    }

    if (G.drugs.ketamine > 25) {
        G.vitals.bloodPressure += 0.08f * dt;
        G.vitals.heartRate += 0.06f * dt;
    }

    // Combined benzodiazepine + respiratory depressant warning is represented
    // explicitly as a game hazard. This is not a dosing model.
    if (G.drugs.midazolam > 25) {
        G.vitals.ventilation -= 0.10f * dt;
    }

    G.vitals.sedation = ClampF(G.vitals.sedation, 0, 100);
    G.vitals.amnesia = ClampF(G.vitals.amnesia, 0, 100);
    G.vitals.analgesia = ClampF(G.vitals.analgesia, 0, 100);
    G.vitals.immobility = ClampF(G.vitals.immobility, 0, 100);
    G.vitals.oxygen = ClampF(G.vitals.oxygen, 0, 100);
    G.vitals.ventilation = ClampF(G.vitals.ventilation, 0, 100);
    G.vitals.bloodPressure = ClampF(G.vitals.bloodPressure, 0, 100);
    G.vitals.heartRate = ClampF(G.vitals.heartRate, 0, 140);
    G.vitals.airwayReserve = ClampF(G.vitals.airwayReserve, 0, 100);

    // Risk variables naturally rise as protective endpoints fall.
    G.vitals.awarenessRisk += (25 - G.vitals.sedation * 0.22f) * dt;
    G.vitals.painRisk += (26 - G.vitals.analgesia * 0.20f) * dt;
    G.vitals.movementRisk += (20 - G.vitals.immobility * 0.18f) * dt;
    G.vitals.emergenceRisk += 2.5f * dt;

    G.vitals.awarenessRisk = ClampF(G.vitals.awarenessRisk, 0, 100);
    G.vitals.painRisk = ClampF(G.vitals.painRisk, 0, 100);
    G.vitals.movementRisk = ClampF(G.vitals.movementRisk, 0, 100);
    G.vitals.emergenceRisk = ClampF(G.vitals.emergenceRisk, 0, 100);
}

static void UpdateVitalsDamage(float dt) {
    // Threats reaching the patient alter the relevant physiologic reserve.
    for (Enemy& e : G.enemies) {
        if (!e.alive) continue;

        Vector2 target = V2(438, 392);
        float d = Dist(e.pos, target);
        if (d < 55) {
            float pressure = e.damage * dt;

            switch (e.kind) {
                case EnemyKind::HYPNOSIS:
                    G.vitals.sedation -= pressure * 1.1f;
                    break;
                case EnemyKind::PAIN:
                    G.vitals.analgesia -= pressure * 1.0f;
                    break;
                case EnemyKind::AWARENESS:
                    G.vitals.awarenessRisk += pressure * 1.6f;
                    break;
                case EnemyKind::MOVEMENT:
                    G.vitals.immobility -= pressure * 1.0f;
                    break;
                case EnemyKind::APNEA:
                    G.vitals.ventilation -= pressure * 1.2f;
                    G.vitals.oxygen -= pressure * 0.8f;
                    break;
                case EnemyKind::HYPOTENSION:
                    G.vitals.bloodPressure -= pressure * 1.1f;
                    break;
                case EnemyKind::BRADYCARDIA:
                    G.vitals.heartRate -= pressure * 1.1f;
                    break;
                case EnemyKind::EMERGENCE:
                    G.vitals.emergenceRisk += pressure * 1.4f;
                    break;
            }

            G.damageFlash = 0.15f;
        }
    }

    G.vitals.oxygen -= std::max(0.0f, 92.0f - G.vitals.ventilation) * 0.014f * dt;
    G.vitals.oxygen += (G.drugs.oxygenSupport * 0.006f) * dt;
    G.vitals.ventilation += (G.drugs.ventilationSupport * 0.009f) * dt;

    if (G.drugs.midazolam > 35 && G.drugs.propofol > 35) {
        G.vitals.ventilation -= 0.20f * dt;
        G.vitals.airwayReserve -= 0.10f * dt;
    }

    G.vitals.oxygen = ClampF(G.vitals.oxygen, 0, 100);
    G.vitals.ventilation = ClampF(G.vitals.ventilation, 0, 100);
    G.vitals.bloodPressure = ClampF(G.vitals.bloodPressure, 0, 100);
    G.vitals.heartRate = ClampF(G.vitals.heartRate, 0, 140);
}

static void UpdateThreats(float dt) {
    Vector2 patient = V2(438, 392);

    for (Enemy& e : G.enemies) {
        if (!e.alive) continue;

        Vector2 toPatient = Normalize(Sub(patient, e.pos));
        float wobble = std::sin(G.globalTime * 2.0f + e.spawnTime) * 0.18f;
        Vector2 tangent = V2(-toPatient.y, toPatient.x);
        e.vel = Mul(Add(toPatient, Mul(tangent, wobble)), e.speed);
        e.pos = Add(e.pos, Mul(e.vel, dt));

        e.pos.x = ClampF(e.pos.x, ARENA_X, ARENA_X + ARENA_W);
        e.pos.y = ClampF(e.pos.y, ARENA_Y, ARENA_Y + ARENA_H);

        e.hitFlash -= dt;

        if (Dist(e.pos, patient) < 40) {
            // Threat stays near the patient rather than repeatedly damaging by
            // physical collision; the vital update handles pressure over time.
            e.speed *= 0.999f;
        }
    }
}

static void UpdateProjectiles(float dt) {
    for (Projectile& p : G.projectiles) {
        if (!p.alive) continue;

        p.pos = Add(p.pos, Mul(p.vel, dt));
        p.life -= dt;

        if (p.life <= 0 ||
            p.pos.x < ARENA_X || p.pos.x > ARENA_X + ARENA_W ||
            p.pos.y < ARENA_Y || p.pos.y > ARENA_Y + ARENA_H) {
            p.alive = false;
            continue;
        }

        for (Enemy& e : G.enemies) {
            if (!e.alive) continue;
            if (Dist(p.pos, e.pos) < e.radius + p.radius + 2) {
                e.hp -= p.damage;
                e.hitFlash = 0.08f;
                p.alive = false;
                AddBurst(e.pos, 4);

                if (e.hp <= 0) {
                    e.alive = false;
                    int gain = e.elite ? 30 : 10;
                    G.score += gain * std::max(1, G.combo);
                    G.combo = std::min(10, G.combo + 1);
                    G.comboTimer = 2.5f;
                    AddPopup(e.pos, "+" + std::to_string(gain), 0.9f);
                }
                break;
            }
        }
    }

    G.projectiles.erase(
        std::remove_if(G.projectiles.begin(), G.projectiles.end(),
            [](const Projectile& p) { return !p.alive; }),
        G.projectiles.end()
    );
}

static void UpdateParticles(float dt) {
    for (Particle& p : G.particles) {
        p.pos = Add(p.pos, Mul(p.vel, dt));
        p.vel = Mul(p.vel, 0.96f);
        p.life -= dt;
    }

    G.particles.erase(
        std::remove_if(G.particles.begin(), G.particles.end(),
            [](const Particle& p) { return p.life <= 0; }),
        G.particles.end()
    );

    for (TextPopup& p : G.popups) {
        p.pos.y -= 20 * dt;
        p.life -= dt;
    }

    G.popups.erase(
        std::remove_if(G.popups.begin(), G.popups.end(),
            [](const TextPopup& p) { return p.life <= 0; }),
        G.popups.end()
    );
}

static void UpdatePlayer(float dt) {
    Vector2 move = V2(0, 0);

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) move.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) move.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1;

    move = Normalize(move);
    G.player = Add(G.player, Mul(move, G.playerSpeed * dt));

    G.player.x = ClampF(G.player.x, ARENA_X + 16, ARENA_X + ARENA_W - 16);
    G.player.y = ClampF(G.player.y, ARENA_Y + 16, ARENA_Y + ARENA_H - 16);

    G.fireCooldown -= dt;

    // Number keys choose the intervention.
    if (IsKeyPressed(KEY_ONE)) Fire(Intervention::PROPOFOL);
    if (IsKeyPressed(KEY_TWO)) Fire(Intervention::MIDAZOLAM);
    if (IsKeyPressed(KEY_THREE)) Fire(Intervention::KETAMINE);
    if (IsKeyPressed(KEY_FOUR)) Fire(Intervention::ETOMIDATE);
    if (IsKeyPressed(KEY_FIVE)) Fire(Intervention::DEXMEDETOMIDINE);
    if (IsKeyPressed(KEY_SIX)) Fire(Intervention::OXYGEN_SUPPORT);
    if (IsKeyPressed(KEY_SEVEN)) Fire(Intervention::VENTILATION_SUPPORT);
    if (IsKeyPressed(KEY_EIGHT)) Fire(Intervention::REVERSAL_FLUMAZENIL);
    if (IsKeyPressed(KEY_NINE)) Fire(Intervention::BALANCE_ANESTHESIA);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        // Mouse firing defaults to the selected intervention.
        Fire(Intervention::PROPOFOL);
    }
}

static void CheckGameState() {
    bool collapse =
        G.vitals.oxygen < 18 ||
        G.vitals.ventilation < 15 ||
        G.vitals.bloodPressure < 18 ||
        G.vitals.heartRate < 25 ||
        G.vitals.airwayReserve < 10;

    if (collapse) {
        G.screen = GameScreen::GAME_OVER;
        G.failures++;
        return;
    }

    // Victory is deliberately about surviving waves, not producing a
    // "perfect" anesthetic.
    if (G.wave >= 8 && G.waveTime > G.waveDuration) {
        G.screen = GameScreen::VICTORY;
    }
}

static void UpdateGame(float dt) {
    G.globalTime += dt;
    G.waveTime += dt;

    UpdatePlayer(dt);
    UpdateThreats(dt);
    UpdateProjectiles(dt);
    UpdateParticles(dt);
    UpdateDrugPharmacology(dt);
    UpdateVitalsDamage(dt);

    G.spawnTimer -= dt;

    float spawnRate = std::max(0.30f, 1.25f - G.wave * 0.07f);
    if (G.spawnTimer <= 0) {
        SpawnEnemy();
        if (G.wave >= 3 && std::rand() % 100 < 30) SpawnEnemy();
        if (G.wave >= 6 && std::rand() % 100 < 20) SpawnEnemy();
        G.spawnTimer = spawnRate;
    }

    if (G.waveTime >= G.waveDuration) {
        G.wave++;
        G.waveTime = 0;
        G.patientsSaved++;
        G.score += 100 * G.wave;
        G.successFlash = 0.8f;
        AddPopup(V2(438, 392), "Wave stabilized", 1.2f);
        if (G.wave > 8) G.wave = 8;
    }

    G.comboTimer -= dt;
    if (G.comboTimer <= 0) G.combo = 0;

    G.shake = std::max(0.0f, G.shake - dt * 4);
    G.damageFlash = std::max(0.0f, G.damageFlash - dt);
    G.successFlash = std::max(0.0f, G.successFlash - dt);

    if (G.sterileWarning) {
        G.vitals.sterileIntegrity -= 1.4f * dt;
        if (G.vitals.sterileIntegrity < 35) {
            // Sterility is represented as a score/risk mechanic, not an
            // infection-treatment simulator.
            G.vitals.emergenceRisk += 0.2f * dt;
        }
    }

    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
        G.screen = GameScreen::PAUSED;
    }

    if (IsKeyPressed(KEY_H)) {
        G.educationOverlay = !G.educationOverlay;
        G.educationPage = 0;
    }

    if (G.educationOverlay) {
        if (IsKeyPressed(KEY_RIGHT)) G.educationPage = (G.educationPage + 1) % 7;
        if (IsKeyPressed(KEY_LEFT)) G.educationPage = (G.educationPage + 6) % 7;
    }

    CheckGameState();
}

static void DrawArenaBackground() {
    DrawRectangle(ARENA_X, ARENA_Y, ARENA_W, ARENA_H, Color{12, 18, 28, 255});

    for (int x = ARENA_X; x < ARENA_X + ARENA_W; x += 40) {
        DrawLine(x, ARENA_Y, x, ARENA_Y + ARENA_H, Color{24, 34, 48, 255});
    }

    for (int y = ARENA_Y; y < ARENA_Y + ARENA_H; y += 40) {
        DrawLine(ARENA_X, y, ARENA_X + ARENA_W, y, Color{24, 34, 48, 255});
    }

    DrawRectangleLinesEx(
        Rectangle{(float)ARENA_X, (float)ARENA_Y, (float)ARENA_W, (float)ARENA_H},
        2,
        Color{70, 95, 120, 255}
    );

    // Patient monitoring zone.
    DrawCircle(438, 392, 66, Fade(SKYBLUE, 0.06f));
    DrawCircleLines(438, 392, 66, Fade(SKYBLUE, 0.28f));
    DrawCircle(438, 392, 45, Fade(GREEN, 0.05f));
    DrawCircleLines(438, 392, 45, Fade(GREEN, 0.25f));

    DrawText("PATIENT ZONE", 386, 322, 14, Fade(SKYBLUE, 0.8f));
}

static void DrawPlayer() {
    Vector2 aim = AimDirection();
    Vector2 barrel = Add(G.player, Mul(aim, 30));

    DrawCircleV(G.player, 18, Color{225, 225, 235, 255});
    DrawCircleV(G.player, 12, Color{70, 100, 145, 255});
    DrawLineEx(G.player, barrel, 7, Color{220, 225, 235, 255});
    DrawCircleV(barrel, 5, Color{110, 220, 210, 255});

    DrawCircleLines((int)G.player.x, (int)G.player.y, 22, Fade(SKYBLUE, 0.35f));
}

static void DrawPatient() {
    Vector2 p = V2(438, 392);

    DrawEllipse((int)p.x, (int)p.y, 40, 24, Color{95, 105, 120, 255});
    DrawCircle((int)p.x + 34, (int)p.y - 2, 13, Color{115, 125, 140, 255});
    DrawLineEx(V2(p.x - 35, p.y), V2(p.x - 65, p.y - 12), 7, Color{115, 125, 140, 255});
    DrawLineEx(V2(p.x - 35, p.y), V2(p.x - 65, p.y + 12), 7, Color{115, 125, 140, 255});

    Color status = GREEN;
    if (G.vitals.oxygen < 55 || G.vitals.ventilation < 55) status = ORANGE;
    if (G.vitals.oxygen < 30 || G.vitals.ventilation < 30) status = RED;

    DrawCircle((int)p.x, (int)p.y, 5, status);
}

static void DrawEnemies() {
    for (const Enemy& e : G.enemies) {
        if (!e.alive) continue;

        Color c = EnemyColor(e.kind);
        if (e.hitFlash > 0) c = WHITE;

        DrawCircleV(e.pos, e.radius + 4, Fade(c, 0.14f));
        DrawCircleV(e.pos, e.radius, c);

        if (e.elite) {
            DrawCircleLines((int)e.pos.x, (int)e.pos.y, e.radius + 4, WHITE);
        }

        float hp = ClampF(e.hp / e.maxHp, 0, 1);
        DrawBar(
            Rectangle{e.pos.x - 17, e.pos.y - e.radius - 10, 34, 4},
            hp,
            1,
            GREEN,
            Color{50, 50, 50, 255}
        );
    }
}

static void DrawProjectiles() {
    for (const Projectile& p : G.projectiles) {
        if (!p.alive) continue;
        Color c = InterventionColor(p.source);
        DrawCircleV(p.pos, p.radius + 2, Fade(c, 0.25f));
        DrawCircleV(p.pos, p.radius, c);
    }
}

static Color InterventionColor(Intervention i) {
    switch (i) {
        case Intervention::PROPOFOL: return Color{240, 240, 245, 255};
        case Intervention::MIDAZOLAM: return Color{165, 110, 245, 255};
        case Intervention::KETAMINE: return Color{90, 200, 245, 255};
        case Intervention::ETOMIDATE: return Color{240, 190, 80, 255};
        case Intervention::DEXMEDETOMIDINE: return Color{110, 225, 160, 255};
        case Intervention::OXYGEN_SUPPORT: return Color{110, 190, 255, 255};
        case Intervention::VENTILATION_SUPPORT: return Color{80, 220, 210, 255};
        case Intervention::REVERSAL_FLUMAZENIL: return Color{245, 115, 180, 255};
        case Intervention::BALANCE_ANESTHESIA: return Color{230, 230, 100, 255};
    }
    return WHITE;
}

static void DrawParticles() {
    for (const Particle& p : G.particles) {
        float a = ClampF(p.life / p.maxLife, 0, 1);
        DrawCircleV(p.pos, p.size, Fade(WHITE, a));
    }

    for (const TextPopup& p : G.popups) {
        float a = ClampF(p.life / p.maxLife, 0, 1);
        int w = MeasureText(p.text.c_str(), 16);
        DrawText(p.text.c_str(), (int)p.pos.x - w / 2, (int)p.pos.y, 16, Fade(WHITE, a));
    }
}

static void DrawTopBar() {
    DrawRectangle(0, 0, SCREEN_W, 92, Color{9, 13, 21, 255});
    DrawLine(0, 91, SCREEN_W, 91, Color{55, 70, 90, 255});

    DrawText("ANESTHESIA AREA DEFENSE", 24, 16, 24, WHITE);
    DrawText("Educational simulation • no clinical dosing", 24, 46, 14, Color{150, 165, 185, 255});

    string waveText = "WAVE " + std::to_string(G.wave);
    DrawText(waveText.c_str(), 660, 18, 22, WHITE);
    string scoreText = "SCORE " + std::to_string(G.score);
    DrawText(scoreText.c_str(), 660, 49, 15, Color{170, 200, 220, 255});

    DrawText("H = teaching   P = pause", 910, 18, 14, Color{160, 180, 195, 255});
    DrawText("WASD / arrows = move", 910, 40, 14, Color{160, 180, 195, 255});
    DrawText("1–9 = interventions", 910, 62, 14, Color{160, 180, 195, 255});
}

static void DrawVitalPanel() {
    int x = 870;
    int y = 116;

    DrawRectangle(x, y, 382, 560, Color{14, 20, 30, 255});
    DrawRectangleLines(x, y, 382, 560, Color{55, 70, 90, 255});

    DrawText("PATIENT MONITOR", x + 18, y + 16, 21, WHITE);

    auto vital = [&](const char* label, float value, Color c, int yy) {
        DrawText(label, x + 18, yy, 14, Color{175, 190, 205, 255});
        DrawBar(Rectangle{(float)x + 150, (float)yy + 2, 190, 14}, value, 100, c,
                Color{35, 42, 52, 255});
        string s = std::to_string((int)value);
        DrawText(s.c_str(), x + 347, yy, 13, WHITE);
    };

    vital("Oxygenation", G.vitals.oxygen, SKYBLUE, y + 55);
    vital("Ventilation", G.vitals.ventilation, TEAL, y + 83);
    vital("Blood pressure", G.vitals.bloodPressure, ORANGE, y + 111);
    vital("Heart rate", G.vitals.heartRate / 1.4f, RED, y + 139);
    vital("Airway reserve", G.vitals.airwayReserve, GREEN, y + 167);

    DrawText("ANESTHETIC ENDPOINTS", x + 18, y + 215, 17, WHITE);
    vital("Sedation / hypnosis", G.vitals.sedation, PURPLE, y + 245);
    vital("Amnesia", G.vitals.amnesia, Color{170, 120, 245, 255}, y + 273);
    vital("Analgesia", G.vitals.analgesia, Color{245, 100, 110, 255}, y + 301);
    vital("Immobility", G.vitals.immobility, Color{90, 170, 245, 255}, y + 329);

    DrawText("RISKS", x + 18, y + 373, 17, WHITE);
    vital("Awareness risk", G.vitals.awarenessRisk, ORANGE, y + 403);
    vital("Pain risk", G.vitals.painRisk, RED, y + 431);
    vital("Movement risk", G.vitals.movementRisk, SKYBLUE, y + 459);
    vital("Emergence risk", G.vitals.emergenceRisk, PINK, y + 487);

    DrawText("Sterile integrity", x + 18, y + 523, 13, Color{175, 190, 205, 255});
    DrawBar(Rectangle{(float)x + 150, (float)y + 525, 190, 12},
            G.vitals.sterileIntegrity, 100, GREEN, Color{35, 42, 52, 255});
}

static void DrawAbilityBar() {
    int x = 44;
    int y = 650;

    DrawRectangle(x, y, 790, 38, Color{9, 13, 21, 235});
    DrawRectangleLines(x, y, 790, 38, Color{55, 70, 90, 255});

    const char* labels[9] = {
        "1 Propofol", "2 Midazolam", "3 Ketamine", "4 Etomidate",
        "5 Dexmed.", "6 O2", "7 Vent.", "8 Flumazenil", "9 Balance"
    };

    int widths[9] = {86, 94, 92, 86, 90, 55, 62, 108, 92};
    int xx = x + 6;

    for (int i = 0; i < 9; ++i) {
        DrawRectangle(xx, y + 5, widths[i] - 4, 28,
                      Color{25, 33, 44, 255});
        DrawText(labels[i], xx + 5, y + 12, 11, WHITE);
        xx += widths[i];
    }
}

static void DrawThreatLegend() {
    int x = 48;
    int y = 126;

    DrawRectangle(x, y, 240, 116, Fade(Color{5, 8, 14, 255}, 0.85f));
    DrawText("THREATS", x + 10, y + 8, 14, WHITE);

    int yy = y + 31;
    EnemyKind kinds[4] = {
        EnemyKind::HYPNOSIS,
        EnemyKind::PAIN,
        EnemyKind::APNEA,
        EnemyKind::HYPOTENSION
    };

    for (int i = 0; i < 4; ++i) {
        DrawCircle(x + 18, yy + i * 20 + 5, 6, EnemyColor(kinds[i]));
        DrawText(EnemyName(kinds[i]), x + 32, yy + i * 20 - 1, 12,
                 Color{190, 200, 210, 255});
    }
}

static void DrawTeachingOverlay() {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.78f));
    DrawRectangle(110, 75, 1060, 570, Color{16, 23, 34, 255});
    DrawRectangleLinesEx(Rectangle{110, 75, 1060, 570}, 2,
                         Color{90, 120, 150, 255});

    const char* title = "";
    const char* body = "";

    switch (G.educationPage) {
        case 0:
            title = "1 / 7  BALANCED ANESTHESIA";
            body =
                "No single IV anesthetic produces every desired endpoint.\n\n"
                "The game separates four goals: hypnosis/sedation, amnesia,\n"
                "analgesia, and immobility. Balanced anesthesia represents using\n"
                "multiple complementary drug classes rather than relying on one\n"
                "drug to do everything.\n\n"
                "Gameplay translation: different threats require different tools.";
            break;

        case 1:
            title = "2 / 7  PROPOFOL";
            body =
                "Propofol is an IV hypnotic commonly used for induction,\n"
                "maintenance, and sedation. It has rapid onset and is formulated\n"
                "as a milky lipid emulsion. After a bolus, termination of effect\n"
                "is strongly influenced by redistribution into less-perfused\n"
                "tissues; metabolism also contributes to elimination.\n\n"
                "Gameplay translation: strong hypnosis, but watch airway and BP.";
            break;

        case 2:
            title = "3 / 7  BENZODIAZEPINES";
            body =
                "Midazolam is a benzodiazepine used for premedication and IV\n"
                "sedation and provides anxiolysis and amnesia. Benzodiazepines\n"
                "decrease the ventilatory response to carbon dioxide. Respiratory\n"
                "depression is more concerning when combined with opioids.\n\n"
                "Gameplay translation: amnesia is valuable, but ventilation must\n"
                "remain monitored. Flumazenil is represented as short acting.";
            break;

        case 3:
            title = "4 / 7  KETAMINE";
            body =
                "Ketamine is a dissociative anesthetic with important analgesic\n"
                "properties. It has a different pharmacologic profile from\n"
                "propofol and benzodiazepines and can support analgesia while\n"
                "providing anesthesia/sedation.\n\n"
                "Gameplay translation: useful when the analgesia threat is high.";
            break;

        case 4:
            title = "5 / 7  ETOMIDATE";
            body =
                "Etomidate is an IV hypnotic used in selected induction settings.\n"
                "The game intentionally abstracts its effects and does not model\n"
                "patient-specific indications, contraindications, or dosing.\n\n"
                "Gameplay translation: a distinct hypnotic option rather than a\n"
                "universal replacement for every other component.";
            break;

        case 5:
            title = "6 / 7  DEXMEDETOMIDINE";
            body =
                "Dexmedetomidine is an alpha-2 adrenergic agonist used for\n"
                "sedation. Its pharmacology differs from classic hypnotics and\n"
                "benzodiazepines; bradycardia and hypotension are relevant\n"
                "physiologic effects to monitor.\n\n"
                "Gameplay translation: useful sedation with a cardiovascular cost.";
            break;

        case 6:
            title = "7 / 7  AIRWAY + STERILE TECHNIQUE";
            body =
                "Anesthetic care is not only about selecting drugs. Oxygenation,\n"
                "ventilation, airway reserve and hemodynamics must be watched.\n\n"
                "Propofol emulsions can support bacterial growth, so sterile\n"
                "technique and appropriate handling are important. This game uses\n"
                "sterility as an educational integrity meter, not as an infection\n"
                "treatment simulator.";
            break;
    }

    DrawText(title, 145, 110, 25, WHITE);

    int yy = 165;
    std::istringstream ss(body);
    string line;
    while (std::getline(ss, line)) {
        DrawText(line.c_str(), 145, yy, 18, Color{205, 215, 225, 255});
        yy += 30;
    }

    DrawText("LEFT / RIGHT = previous / next    H = close", 145, 590, 15,
             Color{145, 175, 200, 255});
}

static void DrawPause() {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.62f));
    DrawCenteredText("PAUSED", 250, 48, WHITE);
    DrawCenteredText("Press P or ESC to continue", 320, 20,
                     Color{180, 200, 220, 255});
    DrawCenteredText("H = teaching overlay", 355, 16,
                     Color{145, 165, 185, 255});
}

static void DrawTitle() {
    ClearBackground(Color{8, 12, 19, 255});

    DrawCenteredText("ANESTHESIA", 115, 60, WHITE);
    DrawCenteredText("AREA DEFENSE", 180, 46, Color{100, 205, 220, 255});
    DrawCenteredText("A medical pharmacology learning game", 245, 20,
                     Color{170, 185, 200, 255});

    DrawRectangle(310, 305, 660, 160, Color{14, 21, 31, 255});
    DrawRectangleLines(310, 305, 660, 160, Color{55, 75, 95, 255});

    DrawCenteredText("Protect the simulated patient by balancing", 330, 20, WHITE);
    DrawCenteredText("hypnosis • amnesia • analgesia • immobility", 365, 20,
                     Color{190, 210, 225, 255});
    DrawCenteredText("while monitoring oxygenation, ventilation and hemodynamics.", 400,
                     17, Color{160, 180, 195, 255});

    DrawCenteredText("ENTER  Start game", 515, 24, Color{110, 225, 175, 255});
    DrawCenteredText("H  Teaching mode", 555, 17, Color{150, 180, 205, 255});

    DrawCenteredText("Educational abstraction — not a clinical simulator.", 625, 14,
                     Color{120, 130, 145, 255});
}

static void DrawGameOver() {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(Color{10, 0, 5, 255}, 0.90f));
    DrawCenteredText("PATIENT STATE COLLAPSED", 190, 40, RED);

    DrawCenteredText("The simulation ended because a critical physiologic reserve fell too low.",
                     265, 18, WHITE);

    string s1 = "Score: " + std::to_string(G.score);
    string s2 = "Wave reached: " + std::to_string(G.wave);
    DrawCenteredText(s1.c_str(), 325, 21, Color{190, 205, 220, 255});
    DrawCenteredText(s2.c_str(), 360, 21, Color{190, 205, 220, 255});

    DrawCenteredText("ENTER  Restart", 445, 23, Color{110, 225, 175, 255});
    DrawCenteredText("H  Review teaching points", 485, 17, Color{150, 180, 205, 255});
}

static void DrawVictory() {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(Color{0, 12, 8, 255}, 0.92f));
    DrawCenteredText("SIMULATION COMPLETE", 175, 44, Color{120, 230, 170, 255});
    DrawCenteredText("You survived the full training sequence.", 250, 21, WHITE);

    string s = "Score: " + std::to_string(G.score);
    DrawCenteredText(s.c_str(), 310, 22, Color{195, 210, 220, 255});

    DrawCenteredText("The goal was not a single magic drug.", 370, 20,
                     Color{180, 205, 220, 255});
    DrawCenteredText("It was continuous monitoring and balanced control of competing endpoints.",
                     405, 17, Color{165, 185, 200, 255});

    DrawCenteredText("ENTER  Play again", 490, 22, Color{110, 225, 175, 255});
}

static void DrawPlaying() {
    ClearBackground(Color{7, 10, 16, 255});

    DrawTopBar();
    DrawArenaBackground();
    DrawThreatLegend();
    DrawEnemies();
    DrawProjectiles();
    DrawPatient();
    DrawPlayer();
    DrawParticles();
    DrawVitalPanel();
    DrawAbilityBar();

    string waveTimer = "Next wave: " +
        std::to_string((int)std::max(0.0f, G.waveDuration - G.waveTime)) + "s";
    DrawText(waveTimer.c_str(), 650, 104, 14, Color{170, 190, 205, 255});

    if (G.combo > 1) {
        string combo = "COMBO x" + std::to_string(G.combo);
        DrawText(combo.c_str(), 315, 110, 18, Color{250, 220, 90, 255});
    }

    if (G.drugs.midazolam > 35 && G.vitals.ventilation < 65) {
        DrawRectangle(278, 610, 330, 30, Fade(RED, 0.18f));
        DrawText("VENTILATORY RISK: monitor closely",
                 292, 617, 14, Color{255, 165, 165, 255});
    }

    if (G.vitals.sterileIntegrity < 45) {
        DrawText("STERILE INTEGRITY LOW", 55, 620, 14, ORANGE);
    }

    if (G.educationOverlay) DrawTeachingOverlay();
}

static void HandleScreens() {
    if (G.screen == GameScreen::TITLE) {
        if (IsKeyPressed(KEY_ENTER)) ResetGameToPlay();
        if (IsKeyPressed(KEY_H)) {
            G.educationOverlay = true;
            G.educationPage = 0;
        }
        if (G.educationOverlay) {
            if (IsKeyPressed(KEY_RIGHT)) G.educationPage = (G.educationPage + 1) % 7;
            if (IsKeyPressed(KEY_LEFT)) G.educationPage = (G.educationPage + 6) % 7;
            if (IsKeyPressed(KEY_H)) G.educationOverlay = false;
        }
        return;
    }

    if (G.screen == GameScreen::GAME_OVER) {
        if (IsKeyPressed(KEY_ENTER)) ResetGameToPlay();
        if (IsKeyPressed(KEY_H)) {
            G.educationOverlay = true;
            G.educationPage = 0;
        }
        return;
    }

    if (G.screen == GameScreen::VICTORY) {
        if (IsKeyPressed(KEY_ENTER)) ResetGameToPlay();
        if (IsKeyPressed(KEY_H)) {
            G.educationOverlay = true;
            G.educationPage = 0;
        }
        return;
    }

    if (G.screen == GameScreen::PAUSED) {
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
            G.screen = GameScreen::PLAYING;
        }
        if (IsKeyPressed(KEY_H)) {
            G.educationOverlay = !G.educationOverlay;
        }
        return;
    }
}

int main() {
    std::srand((unsigned int)std::time(nullptr));

    InitWindow(SCREEN_W, SCREEN_H, "Anesthesia Area Defense - Raylib C++");
    SetTargetFPS(60);

    G.Reset();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        dt = ClampF(dt, 0.0f, 0.033f);

        HandleScreens();

        if (G.screen == GameScreen::PLAYING && !G.educationOverlay) {
            UpdateGame(dt);
        }

        BeginDrawing();

        if (G.screen == GameScreen::TITLE) {
            DrawTitle();
            if (G.educationOverlay) DrawTeachingOverlay();
        } else if (G.screen == GameScreen::PLAYING) {
            DrawPlaying();
        } else if (G.screen == GameScreen::PAUSED) {
            DrawPlaying();
            DrawPause();
            if (G.educationOverlay) DrawTeachingOverlay();
        } else if (G.screen == GameScreen::GAME_OVER) {
            DrawGameOver();
            if (G.educationOverlay) DrawTeachingOverlay();
        } else if (G.screen == GameScreen::VICTORY) {
            DrawVictory();
            if (G.educationOverlay) DrawTeachingOverlay();
        }

        if (G.damageFlash > 0) {
            DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(RED, G.damageFlash * 0.22f));
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}


/* ===== GAMEPLAY NOTES =====
   The patient is a protected simulation target.
   Threats are physiologic concepts, not literal diseases or monsters.
   Interventions are abstract actions and do not correspond to prescriptions.
   The player learns that anesthesia has multiple endpoints.
   Monitoring is continuous rather than a one-time drug-selection event.
   Educational design note 6: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 7: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 8: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 9: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 10: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 11: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 12: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 13: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 14: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 15: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 16: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 17: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 18: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 19: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 20: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 21: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 22: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 23: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 24: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 25: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 26: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 27: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 28: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 29: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 30: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 31: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 32: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 33: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 34: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 35: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 36: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 37: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 38: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 39: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 40: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 41: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 42: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 43: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 44: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 45: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 46: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 47: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 48: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 49: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 50: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 51: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 52: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 53: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 54: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 55: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 56: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 57: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 58: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 59: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 60: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 61: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 62: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 63: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 64: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 65: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 66: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 67: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 68: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 69: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 70: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 71: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 72: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 73: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 74: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 75: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 76: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 77: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 78: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 79: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 80: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 81: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 82: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 83: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 84: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 85: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 86: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 87: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 88: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 89: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 90: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 91: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 92: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 93: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 94: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 95: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 96: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 97: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 98: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 99: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 100: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 101: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 102: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 103: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 104: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 105: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 106: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 107: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 108: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 109: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 110: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 111: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 112: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 113: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 114: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 115: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 116: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 117: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 118: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 119: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 120: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 121: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 122: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 123: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 124: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 125: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 126: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 127: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 128: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 129: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 130: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 131: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 132: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 133: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 134: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 135: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 136: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 137: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 138: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 139: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 140: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 141: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 142: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 143: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 144: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 145: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 146: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 147: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 148: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 149: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 150: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 151: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 152: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 153: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 154: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 155: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 156: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 157: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 158: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 159: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 160: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 161: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 162: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 163: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 164: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 165: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 166: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 167: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 168: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 169: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 170: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 171: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 172: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 173: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 174: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 175: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 176: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 177: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 178: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 179: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 180: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
*/

/* ===== PHARMACOLOGY NOTES =====
   Propofol is represented as a rapid hypnotic with hemodynamic/airway costs.
   Redistribution is represented as a simple declining effect after bolus-like actions.
   Midazolam is represented with prominent amnesia and sedation.
   Ventilatory monitoring is emphasized with benzodiazepine exposure.
   Ketamine is represented with analgesic emphasis.
   Etomidate is represented as a distinct hypnotic option.
   Dexmedetomidine is represented with sedation and bradycardia/hypotension tendencies.
   Flumazenil is intentionally short-lived in the game model.
   No numerical clinical dosing is encoded.
   Educational design note 10: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 11: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 12: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 13: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 14: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 15: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 16: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 17: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 18: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 19: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 20: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 21: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 22: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 23: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 24: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 25: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 26: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 27: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 28: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 29: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 30: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 31: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 32: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 33: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 34: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 35: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 36: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 37: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 38: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 39: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 40: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 41: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 42: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 43: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 44: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 45: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 46: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 47: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 48: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 49: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 50: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 51: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 52: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 53: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 54: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 55: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 56: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 57: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 58: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 59: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 60: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 61: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 62: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 63: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 64: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 65: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 66: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 67: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 68: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 69: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 70: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 71: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 72: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 73: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 74: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 75: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 76: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 77: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 78: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 79: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 80: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 81: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 82: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 83: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 84: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 85: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 86: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 87: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 88: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 89: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 90: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 91: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 92: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 93: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 94: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 95: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 96: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 97: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 98: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 99: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 100: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 101: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 102: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 103: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 104: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 105: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 106: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 107: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 108: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 109: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 110: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 111: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 112: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 113: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 114: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 115: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 116: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 117: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 118: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 119: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 120: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 121: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 122: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 123: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 124: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 125: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 126: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 127: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 128: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 129: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 130: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 131: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 132: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 133: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 134: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 135: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 136: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 137: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 138: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 139: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 140: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 141: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 142: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 143: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 144: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 145: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 146: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 147: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 148: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 149: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 150: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 151: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 152: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 153: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 154: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 155: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 156: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 157: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 158: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 159: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 160: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 161: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 162: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 163: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 164: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 165: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 166: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 167: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 168: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 169: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 170: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 171: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 172: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 173: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 174: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 175: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 176: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 177: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 178: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 179: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 180: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
*/

/* ===== EXTENSION IDEAS =====
   Add an airway-management minigame.
   Add capnography waveforms.
   Add ECG rhythm recognition as a separate educational mode.
   Add simulated emergence and recovery-room phases.
   Add case-based scenarios with different physiologic priorities.
   Add quiz questions after each wave.
   Add a replay screen explaining why each threat was dangerous.
   Add a pharmacokinetics graph comparing onset and offset conceptually.
   Add an adverse-effect codex.
   Add local-language educational text if desired.
   Educational design note 11: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 12: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 13: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 14: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 15: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 16: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 17: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 18: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 19: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 20: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 21: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 22: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 23: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 24: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 25: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 26: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 27: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 28: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 29: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 30: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 31: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 32: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 33: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 34: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 35: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 36: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 37: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 38: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 39: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 40: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 41: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 42: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 43: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 44: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 45: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 46: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 47: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 48: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 49: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 50: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 51: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 52: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 53: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 54: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 55: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 56: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 57: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 58: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 59: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 60: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 61: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 62: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 63: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 64: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 65: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 66: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 67: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 68: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 69: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 70: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 71: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 72: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 73: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 74: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 75: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 76: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 77: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 78: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 79: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 80: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 81: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 82: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 83: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 84: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 85: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 86: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 87: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 88: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 89: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 90: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 91: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 92: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 93: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 94: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 95: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 96: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 97: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 98: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 99: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 100: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 101: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 102: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 103: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 104: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 105: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 106: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 107: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 108: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 109: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 110: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 111: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 112: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 113: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 114: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 115: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 116: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 117: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 118: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 119: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 120: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 121: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 122: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 123: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 124: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 125: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 126: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 127: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 128: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 129: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 130: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 131: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 132: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 133: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 134: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 135: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 136: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 137: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 138: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 139: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 140: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 141: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 142: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 143: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 144: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 145: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 146: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 147: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 148: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 149: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 150: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 151: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 152: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 153: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 154: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 155: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 156: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 157: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 158: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 159: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 160: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 161: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 162: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 163: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 164: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 165: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 166: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 167: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 168: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 169: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 170: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 171: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 172: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 173: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 174: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 175: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 176: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 177: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 178: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 179: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
   Educational design note 180: keep clinical concepts abstract, monitor physiology continuously, and avoid turning the simulation into patient-specific prescribing guidance.
*/

/* ===== PROJECT STUDY LOG =====
   Study note 1: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 3: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 4: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 5: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 6: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 7: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 8: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 9: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 10: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 11: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 12: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 13: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 14: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 15: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 16: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 17: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 18: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 19: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 20: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 21: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 22: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 23: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 24: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 25: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 26: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 27: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 28: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 29: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 30: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 31: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 32: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 33: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 34: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 35: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 36: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 37: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 38: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 39: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 40: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 41: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 42: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 43: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 44: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 45: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 46: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 47: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 48: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 49: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 50: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 51: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 52: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 53: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 54: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 55: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 56: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 57: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 58: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 59: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 60: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 61: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 62: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 63: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 64: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 65: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 66: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 67: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 68: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 69: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 70: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 71: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 72: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 73: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 74: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 75: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 76: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 77: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 78: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 79: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 80: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 81: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 82: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 83: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 84: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 85: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 86: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 87: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 88: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 89: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 90: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 91: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 92: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 93: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 94: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 95: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 96: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 97: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 98: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 99: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 100: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 101: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 102: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 103: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 104: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 105: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 106: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 107: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 108: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 109: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 110: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 111: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 112: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 113: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 114: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 115: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 116: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 117: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 118: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 119: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 120: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 121: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 122: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 123: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 124: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 125: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 126: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 127: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 128: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 129: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 130: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 131: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 132: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 133: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 134: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 135: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 136: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 137: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 138: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 139: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 140: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 141: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 142: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 143: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 144: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 145: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 146: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 147: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 148: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 149: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 150: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 151: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 152: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 153: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 154: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 155: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 156: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 157: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 158: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 159: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 160: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 161: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 162: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 163: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 164: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 165: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 166: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 167: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 168: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 169: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 170: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 171: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 172: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 173: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 174: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 175: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 176: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 177: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 178: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 179: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 180: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 181: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 182: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 183: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 184: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 185: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 186: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 187: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 188: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 189: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 190: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 191: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 192: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 193: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 194: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 195: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 196: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 197: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 198: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 199: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 200: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 201: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 202: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 203: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 204: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 205: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 206: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 207: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 208: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 209: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 210: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 211: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 212: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 213: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 214: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 215: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 216: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 217: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 218: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 219: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 220: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 221: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 222: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 223: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 224: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 225: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 226: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 227: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 228: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 229: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 230: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 231: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 232: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 233: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 234: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 235: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 236: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 237: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 238: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 239: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 240: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 241: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 242: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 243: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 244: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 245: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 246: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 247: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 248: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 249: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 250: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 251: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 252: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 253: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 254: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 255: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 256: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 257: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 258: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 259: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 260: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 261: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 262: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 263: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 264: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 265: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 266: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 267: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 268: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 269: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 270: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 271: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 272: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 273: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 274: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 275: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 276: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 277: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 278: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 279: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 280: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 281: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 282: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 283: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 284: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 285: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 286: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 287: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 288: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 289: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 290: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 291: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 292: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 293: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 294: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 295: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 296: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 297: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 298: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 299: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 300: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 301: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 302: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 303: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 304: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 305: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 306: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 307: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 308: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 309: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 310: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 311: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 312: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 313: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 314: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 315: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 316: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 317: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 318: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 319: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 320: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 321: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 322: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 323: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 324: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 325: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 326: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 327: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 328: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 329: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 330: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 331: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 332: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 333: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 334: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 335: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 336: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 337: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 338: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 339: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 340: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 341: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 342: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 343: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 344: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 345: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 346: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 347: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 348: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 349: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 350: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 351: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 352: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 353: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 354: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 355: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 356: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 357: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 358: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 359: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 360: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 361: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 362: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 363: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 364: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 365: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 366: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 367: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 368: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 369: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 370: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 371: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 372: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 373: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 374: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 375: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 376: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 377: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 378: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 379: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 380: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 381: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 382: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 383: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 384: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 385: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 386: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 387: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 388: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 389: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 390: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 391: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 392: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 393: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 394: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 395: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 396: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 397: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 398: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 399: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 400: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 401: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 402: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 403: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 404: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 405: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 406: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 407: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 408: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 409: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 410: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 411: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 412: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 413: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 414: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 415: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 416: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 417: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 418: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 419: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 420: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 421: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 422: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 423: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 424: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 425: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 426: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 427: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 428: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 429: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 430: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 431: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 432: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 433: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 434: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 435: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 436: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 437: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 438: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 439: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 440: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 441: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 442: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 443: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 444: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 445: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 446: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 447: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 448: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 449: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 450: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 451: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 452: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 453: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 454: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 455: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 456: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 457: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 458: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 459: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 460: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 461: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 462: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 463: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 464: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 465: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 466: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 467: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 468: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 469: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 470: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 471: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 472: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 473: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 474: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 475: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 476: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 477: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 478: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 479: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 480: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 481: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 482: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 483: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 484: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 485: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 486: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 487: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 488: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 489: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 490: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 491: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 492: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 493: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 494: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 495: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 496: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 497: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 498: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 499: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 500: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 501: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 502: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 503: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 504: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 505: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 506: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 507: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 508: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 509: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 510: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 511: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 512: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 513: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 514: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 515: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 516: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 517: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 518: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 519: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 520: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 521: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 522: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 523: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 524: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 525: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 526: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 527: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 528: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 529: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 530: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 531: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 532: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 533: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 534: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 535: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 536: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 537: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 538: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 539: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 540: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 541: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 542: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 543: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 544: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 545: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 546: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 547: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 548: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 549: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 550: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 551: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 552: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 553: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 554: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 555: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 556: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 557: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 558: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 559: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 560: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 561: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 562: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 563: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 564: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 565: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 566: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 567: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 568: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 569: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 570: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 571: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 572: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 573: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 574: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 575: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 576: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 577: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 578: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 579: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 580: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 581: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 582: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 583: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 584: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 585: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 586: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 587: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 588: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 589: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 590: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 591: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 592: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 593: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 594: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 595: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 596: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 597: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 598: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 599: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 600: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 601: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 602: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 603: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 604: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 605: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 606: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 607: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 608: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 609: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 610: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 611: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 612: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 613: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 614: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 615: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 616: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 617: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 618: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 619: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 620: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 621: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 622: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 623: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 624: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 625: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 626: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 627: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 628: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 629: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 630: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 631: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 632: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 633: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 634: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 635: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 636: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 637: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 638: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 639: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 640: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 641: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 642: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 643: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 644: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 645: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 646: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 647: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 648: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 649: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 650: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 651: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 652: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 653: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 654: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 655: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 656: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 657: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 658: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 659: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 660: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 661: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 662: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 663: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 664: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 665: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 666: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 667: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 668: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 669: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 670: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 671: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 672: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 673: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 674: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 675: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 676: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 677: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 678: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 679: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 680: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 681: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 682: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 683: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 684: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 685: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 686: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 687: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 688: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 689: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 690: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 691: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 692: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 693: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 694: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 695: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 696: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 697: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 698: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 699: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 700: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 701: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 702: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 703: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 704: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 705: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 706: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 707: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 708: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 709: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 710: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 711: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 712: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 713: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 714: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 715: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 716: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 717: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 718: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 719: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 720: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 721: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 722: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 723: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 724: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 725: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 726: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 727: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 728: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 729: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 730: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 731: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 732: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 733: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 734: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 735: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 736: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 737: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 738: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 739: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 740: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 741: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 742: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 743: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 744: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 745: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 746: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 747: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 748: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 749: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 750: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 751: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 752: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 753: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 754: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 755: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 756: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 757: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 758: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 759: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 760: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 761: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 762: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 763: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 764: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 765: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 766: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 767: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 768: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 769: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 770: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 771: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 772: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 773: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 774: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 775: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 776: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 777: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 778: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 779: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 780: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 781: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 782: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 783: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 784: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 785: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 786: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 787: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 788: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 789: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 790: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 791: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 792: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 793: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 794: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 795: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 796: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 797: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 798: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 799: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 800: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 801: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 802: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 803: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 804: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 805: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 806: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 807: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 808: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 809: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 810: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 811: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 812: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 813: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 814: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 815: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 816: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 817: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 818: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 819: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 820: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 821: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 822: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 823: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 824: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 825: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 826: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 827: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 828: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 829: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 830: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 831: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 832: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 833: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 834: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 835: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 836: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 837: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 838: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 839: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 840: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 841: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 842: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 843: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 844: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 845: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 846: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 847: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 848: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 849: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 850: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 851: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 852: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 853: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 854: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 855: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 856: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 857: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 858: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 859: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 860: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 861: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 862: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 863: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 864: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 865: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 866: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 867: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 868: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 869: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 870: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 871: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 872: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 873: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 874: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 875: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 876: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 877: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 878: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 879: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 880: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 881: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 882: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 883: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 884: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 885: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 886: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 887: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 888: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 889: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 890: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 891: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 892: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 893: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 894: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 895: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 896: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 897: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 898: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 899: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 900: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 901: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 902: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 903: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 904: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 905: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 906: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 907: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 908: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 909: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 910: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 911: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 912: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 913: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 914: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 915: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 916: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 917: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 918: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 919: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 920: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 921: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 922: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 923: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 924: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 925: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 926: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 927: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 928: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 929: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 930: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 931: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 932: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 933: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 934: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 935: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 936: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 937: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 938: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 939: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 940: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 941: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 942: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 943: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 944: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 945: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 946: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 947: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 948: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 949: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 950: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 951: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 952: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 953: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 954: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 955: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 956: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 957: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 958: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 959: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 960: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 961: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 962: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 963: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 964: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 965: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 966: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 967: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 968: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 969: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 970: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 971: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 972: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 973: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 974: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 975: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 976: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 977: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 978: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 979: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 980: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 981: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 982: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 983: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 984: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 985: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 986: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 987: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 988: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 989: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 990: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 991: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 992: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 993: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 994: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 995: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 996: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 997: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 998: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 999: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1000: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1001: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1002: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1003: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1004: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1005: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1006: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1007: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1008: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1009: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1010: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1011: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1012: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1013: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1014: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1015: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1016: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1017: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1018: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1019: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1020: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1021: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1022: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1023: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1024: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1025: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1026: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1027: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1028: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1029: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1030: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1031: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1032: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1033: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1034: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1035: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1036: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1037: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1038: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1039: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1040: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1041: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1042: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1043: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1044: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1045: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1046: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1047: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1048: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1049: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1050: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1051: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1052: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1053: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1054: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1055: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1056: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1057: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1058: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1059: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1060: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1061: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1062: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1063: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1064: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1065: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1066: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1067: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1068: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1069: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1070: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1071: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1072: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1073: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1074: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1075: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1076: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1077: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1078: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1079: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1080: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1081: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1082: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1083: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1084: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1085: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1086: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1087: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1088: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1089: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1090: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1091: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1092: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1093: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1094: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1095: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1096: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1097: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1098: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1099: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1100: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1101: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1102: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1103: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1104: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1105: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1106: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1107: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1108: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1109: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1110: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1111: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1112: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1113: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1114: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1115: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1116: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1117: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1118: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1119: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1120: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1121: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1122: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1123: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1124: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1125: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1126: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1127: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1128: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1129: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1130: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1131: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1132: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1133: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1134: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1135: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1136: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1137: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1138: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1139: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1140: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1141: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1142: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1143: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1144: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1145: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1146: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1147: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1148: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1149: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1150: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1151: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1152: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1153: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1154: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1155: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1156: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1157: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1158: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1159: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1160: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1161: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1162: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1163: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1164: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1165: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1166: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1167: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1168: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1169: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1170: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1171: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1172: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1173: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1174: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1175: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1176: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1177: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1178: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1179: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1180: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1181: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1182: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1183: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1184: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1185: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1186: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1187: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1188: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1189: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1190: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1191: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1192: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1193: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1194: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1195: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1196: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1197: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1198: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1199: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1200: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1201: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1202: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1203: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1204: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1205: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1206: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1207: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1208: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1209: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1210: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1211: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1212: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1213: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1214: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1215: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1216: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1217: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1218: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1219: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1220: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1221: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1222: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1223: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1224: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1225: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1226: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1227: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1228: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1229: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1230: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1231: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1232: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1233: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1234: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1235: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1236: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1237: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1238: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1239: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1240: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1241: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1242: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1243: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1244: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1245: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1246: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1247: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1248: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1249: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1250: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1251: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1252: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1253: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1254: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1255: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1256: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1257: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1258: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1259: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1260: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1261: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1262: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1263: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1264: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1265: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1266: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1267: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1268: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1269: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1270: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1271: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1272: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1273: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1274: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1275: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1276: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1277: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1278: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1279: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1280: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1281: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1282: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1283: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1284: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1285: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1286: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1287: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1288: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1289: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1290: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1291: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1292: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1293: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1294: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1295: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1296: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1297: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1298: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1299: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1300: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1301: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1302: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1303: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1304: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1305: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1306: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1307: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1308: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1309: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1310: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1311: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1312: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1313: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1314: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1315: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1316: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1317: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1318: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1319: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1320: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1321: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1322: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1323: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1324: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1325: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1326: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1327: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1328: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1329: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1330: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1331: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1332: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1333: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1334: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1335: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1336: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1337: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1338: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1339: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1340: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1341: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1342: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1343: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1344: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1345: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1346: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1347: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1348: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1349: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1350: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1351: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1352: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1353: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1354: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1355: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1356: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1357: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1358: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1359: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1360: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1361: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1362: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1363: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1364: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1365: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1366: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1367: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1368: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1369: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1370: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1371: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1372: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1373: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1374: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1375: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1376: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1377: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1378: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1379: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1380: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1381: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1382: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1383: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1384: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1385: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1386: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1387: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1388: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1389: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1390: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1391: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1392: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1393: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1394: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1395: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1396: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1397: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1398: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1399: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1400: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1401: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1402: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1403: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1404: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1405: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1406: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1407: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1408: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1409: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1410: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1411: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1412: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1413: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1414: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1415: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1416: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1417: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1418: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1419: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1420: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1421: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1422: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1423: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1424: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1425: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1426: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1427: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1428: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1429: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1430: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1431: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1432: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1433: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1434: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1435: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1436: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1437: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1438: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1439: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1440: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1441: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1442: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1443: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1444: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1445: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1446: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1447: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1448: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1449: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1450: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1451: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1452: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1453: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1454: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1455: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1456: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1457: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1458: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1459: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1460: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1461: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1462: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1463: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1464: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1465: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1466: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1467: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1468: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1469: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1470: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1471: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1472: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1473: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1474: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1475: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1476: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1477: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1478: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1479: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1480: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1481: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1482: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1483: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1484: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1485: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1486: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1487: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1488: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1489: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1490: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1491: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1492: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1493: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1494: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1495: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1496: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1497: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1498: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1499: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1500: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1501: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1502: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1503: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1504: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1505: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1506: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1507: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1508: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1509: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1510: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1511: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1512: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1513: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1514: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1515: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1516: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1517: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1518: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1519: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1520: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1521: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1522: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1523: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1524: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1525: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1526: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1527: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1528: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1529: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1530: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1531: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1532: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1533: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1534: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1535: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1536: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1537: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1538: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1539: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1540: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1541: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1542: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1543: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1544: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1545: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1546: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1547: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1548: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1549: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1550: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1551: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1552: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1553: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1554: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1555: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1556: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1557: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1558: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1559: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1560: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1561: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1562: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1563: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1564: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1565: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1566: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1567: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1568: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1569: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1570: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1571: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1572: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1573: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1574: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1575: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1576: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1577: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1578: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1579: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1580: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1581: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1582: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1583: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1584: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1585: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1586: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1587: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1588: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1589: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1590: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1591: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1592: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1593: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1594: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1595: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1596: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1597: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1598: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1599: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1600: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1601: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1602: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1603: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1604: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1605: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1606: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1607: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1608: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1609: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1610: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1611: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1612: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1613: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1614: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1615: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1616: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1617: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1618: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1619: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1620: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1621: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1622: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1623: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1624: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1625: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1626: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1627: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1628: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1629: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1630: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1631: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1632: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1633: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1634: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1635: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1636: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1637: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1638: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1639: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1640: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1641: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1642: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1643: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1644: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1645: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1646: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1647: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1648: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1649: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1650: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1651: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1652: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1653: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1654: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1655: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1656: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1657: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1658: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1659: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1660: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1661: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1662: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1663: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1664: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1665: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1666: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1667: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1668: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1669: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1670: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1671: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1672: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1673: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1674: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1675: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1676: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1677: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1678: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1679: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1680: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1681: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1682: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1683: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1684: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1685: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1686: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1687: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1688: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1689: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1690: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1691: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1692: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1693: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1694: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1695: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1696: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1697: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1698: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1699: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1700: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1701: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1702: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1703: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1704: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1705: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1706: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1707: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1708: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1709: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1710: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1711: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1712: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1713: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1714: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1715: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1716: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1717: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1718: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1719: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1720: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1721: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1722: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1723: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1724: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1725: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1726: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1727: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1728: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1729: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1730: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1731: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1732: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1733: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1734: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1735: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1736: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1737: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1738: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1739: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1740: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1741: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1742: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1743: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1744: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1745: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1746: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1747: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1748: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1749: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1750: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1751: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1752: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1753: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1754: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1755: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1756: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1757: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1758: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1759: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1760: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1761: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1762: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1763: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1764: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1765: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1766: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1767: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1768: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1769: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1770: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1771: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1772: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1773: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1774: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1775: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1776: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1777: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1778: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1779: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1780: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1781: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1782: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1783: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1784: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1785: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1786: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1787: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1788: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1789: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1790: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1791: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1792: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1793: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1794: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1795: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1796: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1797: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1798: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1799: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1800: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1801: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1802: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1803: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1804: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1805: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1806: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1807: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1808: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1809: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1810: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1811: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1812: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1813: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1814: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1815: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1816: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1817: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1818: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1819: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1820: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1821: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1822: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1823: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1824: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1825: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1826: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1827: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1828: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1829: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1830: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1831: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1832: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1833: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1834: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1835: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1836: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1837: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1838: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1839: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1840: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1841: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1842: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1843: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1844: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1845: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1846: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1847: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1848: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1849: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1850: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1851: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1852: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1853: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1854: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1855: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1856: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1857: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1858: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1859: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1860: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1861: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1862: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1863: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1864: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1865: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1866: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1867: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1868: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1869: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1870: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1871: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1872: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1873: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1874: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1875: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1876: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1877: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1878: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1879: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1880: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1881: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1882: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1883: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1884: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1885: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1886: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1887: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1888: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1889: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1890: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1891: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1892: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1893: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1894: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1895: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1896: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1897: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1898: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1899: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1900: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1901: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1902: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1903: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1904: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1905: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1906: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1907: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1908: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1909: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1910: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1911: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1912: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1913: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1914: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1915: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1916: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1917: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1918: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1919: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1920: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1921: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1922: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1923: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1924: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1925: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1926: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1927: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1928: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1929: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1930: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1931: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1932: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1933: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1934: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1935: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1936: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1937: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1938: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1939: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1940: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1941: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1942: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1943: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1944: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1945: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1946: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1947: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1948: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1949: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1950: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1951: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1952: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1953: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1954: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1955: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1956: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1957: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1958: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1959: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1960: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1961: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1962: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1963: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1964: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1965: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1966: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1967: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1968: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1969: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1970: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1971: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1972: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1973: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1974: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1975: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1976: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1977: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1978: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1979: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1980: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1981: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1982: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1983: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1984: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1985: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1986: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1987: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1988: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1989: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1990: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1991: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1992: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1993: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1994: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1995: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1996: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1997: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1998: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 1999: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2000: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2001: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2002: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2003: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2004: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2005: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2006: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2007: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2008: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2009: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2010: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2011: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2012: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2013: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2014: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2015: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2016: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2017: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2018: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2019: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2020: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2021: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2022: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2023: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2024: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2025: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2026: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2027: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2028: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2029: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2030: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2031: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2032: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2033: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2034: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2035: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2036: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2037: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
   Study note 2038: review the distinction between hypnosis, amnesia, analgesia, immobility, airway function, ventilation, and hemodynamic stability when extending this simulation.
*/
