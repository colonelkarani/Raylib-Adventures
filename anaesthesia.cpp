/* ============================================================================
 *  BALANCED ANESTHESIA
 *  ---------------------------------------------------------------------------
 *  A 2D top-down area-defense game about intravenous non-opioid anesthetics.
 *
 *  Source material: Chapter 8, "Intravenous Anesthetics"
 *                   Bokoch MP, Eilers H.  (Miller's Anesthesia)
 *
 *  Every syringe you fire enters the patient's circulation. The game's central
 *  tension is the same tension that governs real practice:
 *
 *      TOO LITTLE  ->  awareness, pain, sympathetic surge, injury.
 *      TOO MUCH    ->  vasodilation, apnoea, bradycardia, injury.
 *
 *  Build:  g++ anaesthesia.cpp -o anaesthesia.exe -Wall -std=c++17 \
 *              -Iinclude "my_random_engine.cpp" -Llib -lraylib \
 *              -lopengl32 -lgdi32 -lwinmm \
 *              -Wl,--defsym,stat64i32=_stat64
 * ==========================================================================*/

#include "raylib.h"
#include "raymath.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstddef>

/* ============================================================================
 *  SECTION 1 -- CONSTANTS
 * ==========================================================================*/

static const int SCREEN_W = 1600;
static const int SCREEN_H =  900;

static const float ARENA_CX = 800.0f;
static const float ARENA_CY = 470.0f;

static const int MAX_PROJECTILES =  512;
static const int MAX_ENEMIES     =  160;
static const int MAX_PARTICLES   = 2048;
static const int MAX_FLOATERS    =  256;
static const int WAVE_QUEUE_MAX  =   96;

static const float PATIENT_RADIUS = 118.0f;

static const float MONITOR_W = 560.0f;
static const float MONITOR_H = 208.0f;
static const float MONITOR_X =  16.0f;
static const float MONITOR_Y =  14.0f;

static const int ECG_BUF = 700;

static const int TARGET_FPS = 60;

/* ============================================================================
 *  SECTION 2 -- ENUMERATIONS
 * ==========================================================================*/

enum DrugId {
    DRUG_PROPOFOL = 0,
    DRUG_FOSPROPOFOL,
    DRUG_THIOPENTAL,
    DRUG_METHOHEXITAL,
    DRUG_MIDAZOLAM,
    DRUG_DIAZEPAM,
    DRUG_KETAMINE,
    DRUG_ETOMIDATE,
    DRUG_DEXMEDETOMIDINE,
    DRUG_FLUMAZENIL,
    DRUG_COUNT
};

enum EnemyType {
    ENEMY_ANXIETY = 0,
    ENEMY_NOCICEPTION,
    ENEMY_AWARENESS,
    ENEMY_LARYNGOSPASM,
    ENEMY_BRONCHOSPASM,
    ENEMY_SEIZURE,
    ENEMY_SYMPATHETIC_SURGE,
    ENEMY_PONV,
    ENEMY_EMERGENCE_DELIRIUM,
    ENEMY_AWARENESS_PARALYSIS,
    ENEMY_TYPE_COUNT
};

enum GameState {
    STATE_MENU = 0,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_CODEX,
    STATE_WAVE_INTRO,
    STATE_GAMEOVER,
    STATE_VICTORY
};

/* ============================================================================
 *  SECTION 3 -- DATA STRUCTURES
 * ==========================================================================*/

struct DrugDef {
    const char *name;
    const char *abbrev;
    const char *className;
    Color       color;

    float hypnotic;
    float analgesic;
    float anxiolytic;
    float anticonvulsant;
    float amnesia;
    float muscleRelaxant;

    float mapDelta;
    float hrDelta;
    float rrDelta;

    float ke;
    float ke0;
    float cshtGrowth;

    float fireCooldown;
    float projectileSpeed;
    float potencyScale;

    bool  isBenzodiazepine;
    bool  antiemetic;
    bool  bronchodilator;
    bool  raisesICP;
    bool  adrenalSuppress;
    bool  injectionPain;
    bool  reversible;
    bool  waterSoluble;

    float durationMin;
    const char *note;
};

struct EnemyDef {
    const char *name;
    const char *desc;
    float hp;
    float speed;
    float radius;
    float contactDamage;
    float hypnoticSens;
    float analgesicSens;
    float anxiolyticSens;
    float anticonvulsantSens;
    float sympathetic;
    float aggression;
    bool  isBoss;
    Color color;
};

struct DrugPK {
    float plasma;
    float effect;
    float exposure;
    float totalGiven;
};

struct Patient {
    float health;
    float healthMax;

    float map;
    float hr;
    float spo2;
    float etco2;
    float bis;
    float temp;

    float sympathetic;
    float sympatheticRaw;

    float adrenalReserve;
    float paralysis;

    float totalHypnosis;
    float totalAnalgesia;
    float totalAnxiolysis;

    float ecg[ECG_BUF];
    int   ecgHead;
    float ecgPhase;

    float pleth[ECG_BUF];
    int   plethHead;
    float plethPhase;

    float capno[ECG_BUF];
    int   capnoHead;
    float capnoPhase;

    float eeg[ECG_BUF];
    int   eegHead;

    float lastDamage;
    bool  alive;
};

struct Player {
    Vector2 pos;
    Vector2 vel;
    float   radius;
    float   speed;
    float   aimAngle;
    float   recoil;
    int     selectedDrug;
    float   cooldowns[DRUG_COUNT];
    float   bob;
    float   whiteCoatFlap;
    float   syringeGlow;
    int     shotsFired;
    int     hits;
};

struct Projectile {
    bool    active;
    Vector2 pos;
    Vector2 vel;
    float   radius;
    float   life;
    float   maxLife;
    DrugId  drug;
    float   spin;
    float   trailTimer;
};

struct Enemy {
    bool      active;
    EnemyType type;
    Vector2   pos;
    Vector2   vel;
    float     hp;
    float     hpMax;
    float     radius;
    float     speed;
    float     hitFlash;
    float     slowTimer;
    float     wobble;
    float     spawnAnim;
    float     pulse;
    bool      inContact;
    float     contactTimer;
    float     damageDealt;
    int       id;
};

struct Particle {
    bool    active;
    Vector2 pos;
    Vector2 vel;
    float   life;
    float   maxLife;
    float   size;
    Color   color;
    float   drag;
    int     kind;      /* 0 = dot, 1 = cross, 2 = ring */
};

struct FloatingText {
    bool    active;
    Vector2 pos;
    Vector2 vel;
    float   life;
    float   maxLife;
    char    text[64];
    Color   color;
    float   size;
};

struct WaveDef {
    const char *name;
    const char *subtitle;
    int   spawnCount;
    EnemyType pool[6];
    float hpMul;
    float speedMul;
    float spawnInterval;
    int   maxAlive;
    bool  boss;
};

/* ============================================================================
 *  SECTION 4 -- DRUG TABLE
 *
 *  Effect-site concentration 1.0 is roughly a deep surgical plane for that
 *  agent alone. Numbers reflect the relative potencies and haemodynamic
 *  profiles described in Chapter 8.
 * ==========================================================================*/

static const DrugDef DRUGS[DRUG_COUNT] = {

    /* PROPOFOL ----------------------------------------------------------- */
    {
        "Propofol", "PRO", "Alkylphenol",
        (Color){ 248, 244, 228, 255 },
        1.05f, 0.00f, 0.18f, 0.70f, 0.55f, 0.05f,
        -58.0f, -12.0f, 1.25f,
        0.100f, 1.30f, 0.14f,
        0.22f, 900.0f, 1.00f,
        false, true, false, false, false, true, false, false,
        5.0f,
        "Hypnotic only. Large fall in MAP via arterial and venous dilation "
        "plus a blunted baroreflex. Apnoea common. Antiemetic. Pain on "
        "injection (big antecubital vein, or lidocaine 20-40 mg IV)."
    },

    /* FOSPROPOFOL -------------------------------------------------------- */
    {
        "Fospropofol", "FOS", "Propofol prodrug",
        (Color){ 232, 236, 200, 255 },
        0.90f, 0.00f, 0.20f, 0.55f, 0.50f, 0.04f,
        -34.0f, -6.0f, 0.85f,
        0.070f, 0.26f, 0.28f,
        0.55f, 620.0f, 0.92f,
        false, true, false, false, false, false, false, true,
        8.0f,
        "Water-soluble prodrug. Slow onset (4-8 min), longer duration. "
        "Theoretically less hypotension and less respiratory depression. "
        "Perineal burning and pruritus are common. Airway compromise remains "
        "the major concern."
    },

    /* THIOPENTAL --------------------------------------------------------- */
    {
        "Thiopental", "THI", "Thiobarbiturate",
        (Color){ 255, 224, 130, 255 },
        1.00f, -0.05f, 0.25f, 1.00f, 0.45f, 0.05f,
        -36.0f, 8.0f, 1.05f,
        0.030f, 1.00f, 0.85f,
        0.28f, 860.0f, 1.02f,
        false, false, false, false, false, false, false, true,
        11.0f,
        "Potent cerebral vasoconstrictor: lowers CBF, CBV, ICP and CMRO2 up "
        "to an isoelectric EEG. Neuroprotective in FOCAL ischaemia only. "
        "Modest MAP fall. Long context-sensitive half-time. Avoid in acute "
        "intermittent porphyria."
    },

    /* METHOHEXITAL ------------------------------------------------------- */
    {
        "Methohexital", "MET", "Oxybarbiturate",
        (Color){ 255, 196, 120, 255 },
        0.98f, -0.05f, 0.20f, -0.85f, 0.40f, 0.05f,
        -32.0f, 8.0f, 0.95f,
        0.120f, 1.10f, 0.30f,
        0.24f, 880.0f, 0.98f,
        false, false, false, false, false, false, false, true,
        4.0f,
        "Faster clearance than thiopental -> faster, more complete recovery. "
        "PROCONVULSANT: activates seizure foci. Useful for ECT and epilepsy "
        "focus mapping. DO NOT use against seizure activity."
    },

    /* MIDAZOLAM ---------------------------------------------------------- */
    {
        "Midazolam", "MID", "Benzodiazepine",
        (Color){ 150, 220, 255, 255 },
        0.58f, 0.00f, 1.30f, 0.95f, 1.30f, 0.20f,
        -14.0f, -4.0f, 0.38f,
        0.062f, 0.50f, 0.22f,
        0.30f, 760.0f, 0.72f,
        true, true, false, false, false, false, true, true,
        2.0f,
        "Anxiolysis, sedation, anterograde amnesia, anticonvulsant. Minimal "
        "CV/respiratory depression alone, but SYNERGISTIC with opioids and "
        "propofol. Ceiling effect: cannot produce an isoelectric EEG. "
        "Reversible with flumazenil 8-15 mcg/kg IV."
    },

    /* DIAZEPAM ----------------------------------------------------------- */
    {
        "Diazepam", "DIA", "Benzodiazepine",
        (Color){ 120, 180, 240, 255 },
        0.42f, 0.00f, 1.10f, 1.10f, 0.85f, 0.35f,
        -9.0f, -2.0f, 0.32f,
        0.022f, 0.32f, 1.10f,
        0.42f, 700.0f, 0.68f,
        true, false, false, false, false, true, true, false,
        35.0f,
        "Long elimination half-time; active metabolites (desmethyldiazepam, "
        "oxazepam). Propylene glycol vehicle causes pain on injection and "
        "thrombophlebitis. First-line for local-anesthetic and "
        "alcohol-withdrawal seizures."
    },

    /* KETAMINE ----------------------------------------------------------- */
    {
        "Ketamine", "KET", "Phencyclidine / NMDA antagonist",
        (Color){ 210, 130, 255, 255 },
        0.62f, 1.55f, 0.10f, 0.30f, 0.35f, 0.00f,
        26.0f, 24.0f, -0.15f,
        0.115f, 1.05f, 0.18f,
        0.30f, 950.0f, 1.05f,
        false, false, true, true, false, false, false, true,
        3.0f,
        "The only IV agent here with real ANALGESIA (NMDA blockade). "
        "Sympathomimetic -- raises MAP, HR and cardiac output. "
        "Bronchodilator. Minimal respiratory depression. Cerebral "
        "VASODILATOR: raises CBF/CMRO2/ICP. Emergence reactions."
    },

    /* ETOMIDATE ---------------------------------------------------------- */
    {
        "Etomidate", "ETO", "Carboxylated imidazole",
        (Color){ 255, 160, 190, 255 },
        1.02f, 0.00f, 0.12f, -0.35f, 0.40f, 0.05f,
        -5.0f, -3.0f, 0.60f,
        0.095f, 1.00f, 0.16f,
        0.26f, 840.0f, 1.00f,
        false, false, false, false, true, true, false, false,
        4.0f,
        "Cardiovascular STABILITY: minimal change in MAP, HR or CO. Ideal "
        "when myocardial contractility is poor, in coronary disease or severe "
        "aortic stenosis. Inhibits 11-beta-hydroxylase -> adrenocortical "
        "suppression. Myoclonus >50%. Activates seizure foci."
    },

    /* DEXMEDETOMIDINE ---------------------------------------------------- */
    {
        "Dexmedetomidine", "DEX", "alpha-2 adrenergic agonist",
        (Color){ 140, 255, 210, 255 },
        0.55f, 0.85f, 0.60f, 0.05f, 0.30f, 0.00f,
        -20.0f, -22.0f, 0.14f,
        0.072f, 0.52f, 0.95f,
        0.34f, 780.0f, 0.80f,
        false, false, false, false, false, false, false, true,
        3.0f,
        "Sedation resembling natural sleep (locus ceruleus). Analgesia at "
        "the spinal cord. Minimal respiratory depression -- the key "
        "advantage. BRADYCARDIA and hypotension are typical. "
        "Context-sensitive half-time rises steeply with long infusions."
    },

    /* FLUMAZENIL --------------------------------------------------------- */
    {
        "Flumazenil", "FLU", "Benzodiazepine antagonist",
        (Color){ 255, 255, 255, 255 },
        0.00f, 0.00f, 0.00f, -0.30f, 0.00f, 0.00f,
        0.0f, 0.0f, 0.0f,
        0.160f, 1.40f, 0.00f,
        0.50f, 1000.0f, 0.00f,
        false, false, false, false, false, false, false, true,
        1.0f,
        "Antidote. Eliminates benzodiazepine effect-site concentration. "
        "Duration ~20 min, so RESEDATION is possible. May precipitate "
        "seizures in chronic benzodiazepine users or in benzodiazepine-"
        "treated status epilepticus."
    },
};

/* ============================================================================
 *  SECTION 5 -- ENEMY TABLE
 * ==========================================================================*/

static const EnemyDef ENEMIES[ENEMY_TYPE_COUNT] = {

    { "Anxiety",
      "Preoperative apprehension. Benzodiazepines are the specific answer.",
      34.0f, 92.0f, 15.0f, 3.0f,
      0.70f, 0.00f, 1.50f, 0.20f, 0.10f, 1.00f, false,
      (Color){ 198, 178, 255, 255 } },

    { "Nociception",
      "Surgical pain. Hypnotics alone DO NOT suppress it.",
      78.0f, 58.0f, 21.0f, 6.5f,
      0.22f, 1.60f, 0.10f, 0.00f, 0.32f, 1.00f, false,
      (Color){ 255, 92, 92, 255 } },

    { "Awareness",
      "Intraoperative consciousness with recall. Needs deep hypnosis.",
      62.0f, 118.0f, 17.0f, 9.0f,
      1.55f, 0.00f, 0.45f, 0.10f, 0.22f, 1.35f, false,
      (Color){ 255, 230, 80, 255 } },

    { "Laryngospasm",
      "Reflex laryngeal closure. Propofol suppresses airway reflexes best.",
      88.0f, 46.0f, 24.0f, 8.0f,
      1.10f, 0.10f, 0.15f, 0.05f, 0.30f, 0.85f, false,
      (Color){ 90, 210, 255, 255 } },

    { "Bronchospasm",
      "Reactive airway constriction. Ketamine is the agent of choice.",
      82.0f, 52.0f, 23.0f, 7.5f,
      0.45f, 0.55f, 0.10f, 0.00f, 0.26f, 0.95f, false,
      (Color){ 150, 255, 160, 255 } },

    { "Seizure Focus",
      "Epileptiform activity. Methohexital and etomidate make it WORSE.",
      96.0f, 66.0f, 22.0f, 10.0f,
      0.35f, 0.00f, 0.55f, 1.70f, 0.28f, 1.10f, false,
      (Color){ 255, 140, 255, 255 } },

    { "Sympathetic Surge",
      "Neuroendocrine stress response. Ketamine will make it worse.",
      130.0f, 42.0f, 30.0f, 7.0f,
      1.15f, 0.55f, 0.30f, 0.05f, 0.55f, 0.80f, false,
      (Color){ 255, 120, 40, 255 } },

    { "PONV",
      "Postoperative nausea and vomiting. Propofol is antiemetic.",
      40.0f, 74.0f, 16.0f, 4.5f,
      0.30f, 0.00f, 0.60f, 0.00f, 0.14f, 1.05f, false,
      (Color){ 190, 230, 120, 255 } },

    { "Emergence Delirium",
      "Agitation on emergence. Dexmedetomidine is the specific answer.",
      74.0f, 88.0f, 20.0f, 6.0f,
      0.55f, 0.45f, 0.65f, 0.00f, 0.24f, 1.15f, false,
      (Color){ 255, 200, 150, 255 } },

    { "Awareness Under Paralysis",
      "THE NIGHTMARE. Needs deep hypnosis AND adequate analgesia.",
      640.0f, 50.0f, 52.0f, 18.0f,
      1.35f, 1.35f, 0.30f, 0.10f, 0.80f, 0.85f, true,
      (Color){ 220, 30, 70, 255 } },
};

/* ============================================================================
 *  SECTION 6 -- WAVE TABLE
 * ==========================================================================*/

static const int WAVE_COUNT = 14;

static const WaveDef WAVES[WAVE_COUNT] = {
    { "PREOPERATIVE HOLDING",
      "Anxiolysis and amnesia are the goals. Midazolam 1-2 mg IV.",
      8, { ENEMY_ANXIETY, ENEMY_ANXIETY, ENEMY_ANXIETY, ENEMY_PONV, ENEMY_ANXIETY, ENEMY_ANXIETY },
      1.00f, 1.00f, 1.05f, 10, false },

    { "INDUCTION",
      "Loss of consciousness. Propofol 1-2.5 mg/kg, or etomidate if the patient cannot tolerate a fall in blood pressure.",
      10, { ENEMY_ANXIETY, ENEMY_AWARENESS, ENEMY_ANXIETY, ENEMY_AWARENESS, ENEMY_PONV, ENEMY_AWARENESS },
      1.00f, 1.00f, 0.90f, 11, false },

    { "LARYNGOSCOPY",
      "Direct laryngoscopy is a potent stimulus. Propofol suppresses upper airway reflexes more than thiopental.",
      9, { ENEMY_LARYNGOSPASM, ENEMY_ANXIETY, ENEMY_AWARENESS, ENEMY_LARYNGOSPASM, ENEMY_NOCICEPTION, ENEMY_AWARENESS },
      1.05f, 1.00f, 0.95f, 11, false },

    { "TRACHEAL INTUBATION",
      "The most intense airway stimulus of the case. Expect a sympathetic surge.",
      10, { ENEMY_LARYNGOSPASM, ENEMY_SYMPATHETIC_SURGE, ENEMY_NOCICEPTION, ENEMY_AWARENESS, ENEMY_ANXIETY, ENEMY_LARYNGOSPASM },
      1.10f, 1.02f, 0.92f, 12, false },

    { "SURGICAL INCISION",
      "The classic nociceptive stimulus. Hypnosis alone is not enough.",
      11, { ENEMY_NOCICEPTION, ENEMY_NOCICEPTION, ENEMY_SYMPATHETIC_SURGE, ENEMY_AWARENESS, ENEMY_NOCICEPTION, ENEMY_ANXIETY },
      1.10f, 1.02f, 0.88f, 12, false },

    { "MAINTENANCE -- PHASE I",
      "Steady state. Watch the context-sensitive half-time.",
      12, { ENEMY_AWARENESS, ENEMY_NOCICEPTION, ENEMY_SEIZURE, ENEMY_ANXIETY, ENEMY_SYMPATHETIC_SURGE, ENEMY_BRONCHOSPASM },
      1.18f, 1.05f, 0.80f, 13, false },

    { "PNEUMOPERITONEUM",
      "CO2 insufflation: sympathetic stimulation, hypercapnia, raised airway pressures.",
      12, { ENEMY_SYMPATHETIC_SURGE, ENEMY_BRONCHOSPASM, ENEMY_NOCICEPTION, ENEMY_SYMPATHETIC_SURGE, ENEMY_AWARENESS, ENEMY_PONV },
      1.22f, 1.06f, 0.80f, 13, false },

    { "EPILEPTIFORM ACTIVITY",
      "Benzodiazepines, propofol and thiopental are anticonvulsant. Methohexital and etomidate are PROCONVULSANT.",
      11, { ENEMY_SEIZURE, ENEMY_SEIZURE, ENEMY_AWARENESS, ENEMY_SEIZURE, ENEMY_NOCICEPTION, ENEMY_ANXIETY },
      1.25f, 1.06f, 0.90f, 13, false },

    { "REACTIVE AIRWAY",
      "Bronchospasm. Ketamine is the specific answer.",
      11, { ENEMY_BRONCHOSPASM, ENEMY_BRONCHOSPASM, ENEMY_LARYNGOSPASM, ENEMY_NOCICEPTION, ENEMY_BRONCHOSPASM, ENEMY_ANXIETY },
      1.28f, 1.08f, 0.85f, 13, false },

    { "DEEP STIMULATION",
      "Major surgical stimulation. Do not let the patient lighten.",
      13, { ENEMY_SYMPATHETIC_SURGE, ENEMY_NOCICEPTION, ENEMY_SYMPATHETIC_SURGE, ENEMY_AWARENESS, ENEMY_SEIZURE, ENEMY_NOCICEPTION },
      1.32f, 1.08f, 0.75f, 14, false },

    { "HAEMODYNAMIC INSTABILITY",
      "The therapeutic window is narrow. Watch the MAP.",
      14, { ENEMY_AWARENESS, ENEMY_SYMPATHETIC_SURGE, ENEMY_NOCICEPTION, ENEMY_AWARENESS, ENEMY_BRONCHOSPASM, ENEMY_SEIZURE },
      1.36f, 1.10f, 0.72f, 14, false },

    { "EMERGENCE",
      "The volatile agent is off. Expect emergence delirium and PONV.",
      14, { ENEMY_EMERGENCE_DELIRIUM, ENEMY_PONV, ENEMY_PONV, ENEMY_EMERGENCE_DELIRIUM, ENEMY_AWARENESS, ENEMY_ANXIETY },
      1.38f, 1.10f, 0.70f, 15, false },

    { "RECOVERY ROOM",
      "The case is over but the patient is not safe yet.",
      15, { ENEMY_PONV, ENEMY_EMERGENCE_DELIRIUM, ENEMY_NOCICEPTION, ENEMY_AWARENESS, ENEMY_EMERGENCE_DELIRIUM, ENEMY_SEIZURE },
      1.42f, 1.12f, 0.66f, 15, false },

    { "AWARENESS UNDER PARALYSIS",
      "A paralysed patient who is fully conscious. Deep hypnosis AND adequate analgesia simultaneously.",
      1, { ENEMY_AWARENESS_PARALYSIS, ENEMY_NOCICEPTION, ENEMY_AWARENESS, ENEMY_SYMPATHETIC_SURGE, ENEMY_SEIZURE, ENEMY_BRONCHOSPASM },
      1.00f, 1.00f, 3.20f, 16, true },
};

/* ============================================================================
 *  SECTION 7 -- GLOBAL STATE
 * ==========================================================================*/

static GameState     gState        = STATE_MENU;
static Patient       gPatient;
static Player        gPlayer;
static DrugPK        gDrugPK[DRUG_COUNT];

static Projectile    gProjectiles[MAX_PROJECTILES];
static Enemy         gEnemies[MAX_ENEMIES];
static Particle      gParticles[MAX_PARTICLES];
static FloatingText  gFloaters[MAX_FLOATERS];

static int           gCurrentWave       = 0;
static float         gWaveIntroTimer    = 0.0f;
static int           gWaveQueue[WAVE_QUEUE_MAX];
static int           gWaveQueueCount    = 0;
static int           gWaveQueueHead     = 0;
static float         gSpawnTimer        = 0.0f;

static float         gGameTime          = 0.0f;
static float         gScore             = 0.0f;
static int           gKills             = 0;
static int           gEnemiesKilledThisWave = 0;
static int           gTotalEnemiesThisWave  = 0;

static float         gShake        = 0.0f;
static float         gFlashRed     = 0.0f;
static float         gFlashWhite   = 0.0f;

static int           gNextEnemyId  = 0;
static bool          gShowHelp     = false;
static float         gCodexScroll  = 0.0f;

static float         gStatTotalDrug      = 0.0f;
static float         gStatPropofol       = 0.0f;
static float         gStatKetamine       = 0.0f;
static float         gStatEtomidate      = 0.0f;
static float         gStatHypotensionTime = 0.0f;
static float         gStatHypoxiaTime     = 0.0f;
static float         gStatAwarenessTime   = 0.0f;

/* ============================================================================
 *  SECTION 7b -- FORWARD DECLARATIONS
 *
 *  Everything that is referenced before its definition lives here.
 * ==========================================================================*/

/* Utilities */
static float  Clampf(float v, float lo, float hi);
static float  Lerpf(float a, float b, float t);
static float  Randf(float lo, float hi);
static int    Randi(int lo, int hi);
static float  SmoothTowards(float current, float target, float rate, float dt);
static float  VecLenSqr(Vector2 v);
static Vector2 VecFromAngle(float a);
static float  AngleTo(Vector2 from, Vector2 to);
static Color  ColLerp(Color a, Color b, float t);
static void   DrawTxt(const char *text, float x, float y, float size, Color c);
static void   DrawTxtCentered(const char *text, float cx, float y, float size, Color c);
static void   DrawTxtRight(const char *text, float rx, float y, float size, Color c);

/* Particles */
static void ParticlesClear(void);
static void SpawnParticle(Vector2 pos, Vector2 vel, float life, float size,
                          Color col, int kind, float drag);
static void Burst(Vector2 pos, int count, Color col, float speed, float size, int kind);
static void UpdateParticles(float dt);
static void DrawParticles(void);

/* Floating text */
static void FloatersClear(void);
static void SpawnFloater(Vector2 pos, const char *text, Color col, float size);
static void UpdateFloaters(float dt);
static void DrawFloaters(void);

/* Patient */
static void PatientInit(void);
static void AdministerDrug(DrugId id, float units);
static void ComputeDrugEffects(float *outHyp, float *outAnalg,
                               float *outAnx, float *outAntiConv,
                               float *outMap, float *outHR, float *outRR,
                               float *outAdrenal, float *outBronch,
                               float *outICP);
static void UpdatePharmacokinetics(float dt);
static void UpdateSympatheticTone(float dt);
static void UpdatePatient(float dt);

/* Player */
static void PlayerInit(void);
static void UpdatePlayer(float dt);

/* Projectiles */
static void ProjectilesClear(void);
static void UpdateProjectiles(float dt);
static void DrawProjectiles(void);

/* Enemies */
static void EnemiesClear(void);
static void SpawnEnemy(EnemyType type, float hpMul, float speedMul);
static void DamageEnemy(Enemy *e, float amount, Vector2 hitPos, Color color);
static void UpdateEnemies(float dt);
static int  CountAliveEnemies(void);

/* Pharmacology */
static float DrugDamageVsEnemy(DrugId id, const Enemy *e);
static void  DrugHitEnemy(DrugId id, Enemy *e, Vector2 hitPos);
static void  FireSyringe(void);

/* Wave director */
static void WaveBegin(int index);
static void UpdateWaveDirector(float dt);

/* HUD / draw */
static void DrawMonitorPanel(void);
static void DrawDrugBar(void);
static void DrawWaveBanner(void);
static void DrawAlarms(void);
static void DrawOperatingRoomFloor(void);
static void DrawPatient(void);
static void DrawEnemyShape(Enemy *e);
static void DrawEnemies(void);
static void DrawPlayer(void);
static void DrawGameplayOverlay(void);
static void DrawTitleScreen(void);
static void DrawCodexScreen(void);
static void DrawEndScreen(bool victory);

/* Flow */
static void ResetGame(void);
static void UpdateGameplay(float dt);
static void DrawGameplay(void);

/* ============================================================================
 *  SECTION 8 -- UTILITIES
 * ==========================================================================*/

static inline float Clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline float Lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

static inline float Randf(float lo, float hi)
{
    return lo + (float)GetRandomValue(0, 10000) / 10000.0f * (hi - lo);
}

static inline int Randi(int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + GetRandomValue(0, hi - lo - 1);
}

static inline float SmoothTowards(float current, float target, float rate, float dt)
{
    return current + (target - current) * Clampf(rate * dt, 0.0f, 1.0f);
}

static inline float VecLenSqr(Vector2 v)
{
    return v.x * v.x + v.y * v.y;
}

static inline Vector2 VecFromAngle(float a)
{
    return (Vector2){ cosf(a), sinf(a) };
}

static inline float AngleTo(Vector2 from, Vector2 to)
{
    return atan2f(to.y - from.y, to.x - from.x);
}

static Color ColLerp(Color a, Color b, float t)
{
    t = Clampf(t, 0.0f, 1.0f);
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static void DrawTxt(const char *text, float x, float y, float size, Color c)
{
    DrawTextEx(GetFontDefault(), text, (Vector2){ x, y }, size, size * 0.08f, c);
}

static void DrawTxtCentered(const char *text, float cx, float y, float size, Color c)
{
    Vector2 m = MeasureTextEx(GetFontDefault(), text, size, size * 0.08f);
    DrawTextEx(GetFontDefault(), text, (Vector2){ cx - m.x * 0.5f, y }, size, size * 0.08f, c);
}

static void DrawTxtRight(const char *text, float rx, float y, float size, Color c)
{
    Vector2 m = MeasureTextEx(GetFontDefault(), text, size, size * 0.08f);
    DrawTextEx(GetFontDefault(), text, (Vector2){ rx - m.x, y }, size, size * 0.08f, c);
}

/* ============================================================================
 *  SECTION 9 -- PARTICLE & FLOATING TEXT SYSTEM
 * ==========================================================================*/

static void ParticlesClear(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) gParticles[i].active = false;
}

static void SpawnParticle(Vector2 pos, Vector2 vel, float life, float size,
                          Color col, int kind, float drag)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!gParticles[i].active) {
            gParticles[i].active  = true;
            gParticles[i].pos     = pos;
            gParticles[i].vel     = vel;
            gParticles[i].life    = life;
            gParticles[i].maxLife = life;
            gParticles[i].size    = size;
            gParticles[i].color   = col;
            gParticles[i].kind    = kind;
            gParticles[i].drag    = drag;
            return;
        }
    }
}

static void Burst(Vector2 pos, int count, Color col, float speed, float size, int kind)
{
    for (int i = 0; i < count; i++) {
        float a = Randf(0.0f, 2.0f * PI);
        float s = Randf(speed * 0.35f, speed);
        Vector2 v = { cosf(a) * s, sinf(a) * s };
        SpawnParticle(pos, v, Randf(0.25f, 0.65f), size * Randf(0.6f, 1.4f),
                      col, kind, 2.4f);
    }
}

static void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &gParticles[i];
        if (!p->active) continue;
        p->life -= dt;
        if (p->life <= 0.0f) { p->active = false; continue; }
        p->pos = Vector2Add(p->pos, Vector2Scale(p->vel, dt));
        float damp = 1.0f - Clampf(p->drag * dt, 0.0f, 1.0f);
        p->vel = Vector2Scale(p->vel, damp);
    }
}

static void DrawParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &gParticles[i];
        if (!p->active) continue;
        float t = p->life / p->maxLife;
        Color c = p->color;
        c.a = (unsigned char)(255.0f * Clampf(t, 0.0f, 1.0f));
        float sz = p->size * (0.4f + 0.6f * t);
        if (p->kind == 0) {
            DrawCircleV(p->pos, sz, c);
        } else if (p->kind == 1) {
            DrawLineEx((Vector2){ p->pos.x - sz, p->pos.y },
                       (Vector2){ p->pos.x + sz, p->pos.y }, 2.0f, c);
            DrawLineEx((Vector2){ p->pos.x, p->pos.y - sz },
                       (Vector2){ p->pos.x, p->pos.y + sz }, 2.0f, c);
        } else {
            DrawRing(p->pos, sz, sz + 1.6f, 0.0f, 360.0f, 24, c);
        }
    }
}

static void FloatersClear(void)
{
    for (int i = 0; i < MAX_FLOATERS; i++) gFloaters[i].active = false;
}

static void SpawnFloater(Vector2 pos, const char *text, Color col, float size)
{
    for (int i = 0; i < MAX_FLOATERS; i++) {
        if (!gFloaters[i].active) {
            gFloaters[i].active  = true;
            gFloaters[i].pos     = pos;
            gFloaters[i].vel     = (Vector2){ Randf(-18.0f, 18.0f), -46.0f };
            gFloaters[i].life    = 1.1f;
            gFloaters[i].maxLife = 1.1f;
            gFloaters[i].color   = col;
            gFloaters[i].size    = size;
            std::snprintf(gFloaters[i].text, sizeof(gFloaters[i].text),
                          "%s", text);
            return;
        }
    }
}

static void UpdateFloaters(float dt)
{
    for (int i = 0; i < MAX_FLOATERS; i++) {
        FloatingText *f = &gFloaters[i];
        if (!f->active) continue;
        f->life -= dt;
        if (f->life <= 0.0f) { f->active = false; continue; }
        f->pos = Vector2Add(f->pos, Vector2Scale(f->vel, dt));
        f->vel.y += 42.0f * dt;
        f->vel = Vector2Scale(f->vel, 1.0f - Clampf(1.6f * dt, 0.0f, 1.0f));
    }
}

static void DrawFloaters(void)
{
    for (int i = 0; i < MAX_FLOATERS; i++) {
        FloatingText *f = &gFloaters[i];
        if (!f->active) continue;
        float t = f->life / f->maxLife;
        Color c = f->color;
        c.a = (unsigned char)(255.0f * Clampf(t * 1.5f, 0.0f, 1.0f));
        DrawTxtCentered(f->text, f->pos.x, f->pos.y, f->size, c);
    }
}

/* ============================================================================
 *  SECTION 10 -- PATIENT PHYSIOLOGY
 * ==========================================================================*/

static void PatientInit(void)
{
    std::memset(&gPatient, 0, sizeof(Patient));

    gPatient.healthMax      = 100.0f;
    gPatient.health         = 100.0f;
    gPatient.map            = 88.0f;
    gPatient.hr             = 76.0f;
    gPatient.spo2           = 99.0f;
    gPatient.etco2          = 38.0f;
    gPatient.bis            = 96.0f;
    gPatient.temp           = 36.6f;
    gPatient.sympathetic    = 0.0f;
    gPatient.sympatheticRaw = 0.0f;
    gPatient.adrenalReserve = 1.0f;
    gPatient.paralysis      = 0.0f;
    gPatient.alive          = true;

    for (int i = 0; i < ECG_BUF; i++) {
        gPatient.ecg[i]   = 0.0f;
        gPatient.pleth[i] = 0.0f;
        gPatient.capno[i] = 0.0f;
        gPatient.eeg[i]   = 0.0f;
    }

    for (int i = 0; i < DRUG_COUNT; i++) {
        gDrugPK[i].plasma     = 0.0f;
        gDrugPK[i].effect     = 0.0f;
        gDrugPK[i].exposure   = 0.0f;
        gDrugPK[i].totalGiven = 0.0f;
    }

    gStatTotalDrug         = 0.0f;
    gStatPropofol          = 0.0f;
    gStatKetamine          = 0.0f;
    gStatEtomidate         = 0.0f;
    gStatHypotensionTime   = 0.0f;
    gStatHypoxiaTime       = 0.0f;
    gStatAwarenessTime     = 0.0f;
}

/* Crude PQRST morphology. Not a diagnostic tracing. */
static float ECGSample(float phase)
{
    float v = 0.0f;
    v += 0.13f * expf(-powf((phase - 0.16f) / 0.028f, 2.0f));
    v -= 0.09f * expf(-powf((phase - 0.27f) / 0.009f, 2.0f));
    v += 1.00f * expf(-powf((phase - 0.295f) / 0.010f, 2.0f));
    v -= 0.24f * expf(-powf((phase - 0.325f) / 0.013f, 2.0f));
    v += 0.30f * expf(-powf((phase - 0.46f) / 0.055f, 2.0f));
    return v;
}

static void ComputeDrugEffects(float *outHyp, float *outAnalg,
                               float *outAnx, float *outAntiConv,
                               float *outMap, float *outHR, float *outRR,
                               float *outAdrenal, float *outBronch,
                               float *outICP)
{
    float hyp = 0.0f, analg = 0.0f, anx = 0.0f, ac = 0.0f;
    float mapD = 0.0f, hrD = 0.0f, rr = 0.0f;
    float adr = 0.0f, br = 0.0f, icp = 0.0f;

    float benzoHyp = 0.0f;

    for (int i = 0; i < DRUG_COUNT; i++) {
        float e = gDrugPK[i].effect;
        if (e <= 0.0f) continue;
        const DrugDef *d = &DRUGS[i];

        if (d->isBenzodiazepine) benzoHyp += e * d->hypnotic;
        else                     hyp      += e * d->hypnotic;

        analg += e * d->analgesic;
        anx   += e * d->anxiolytic;
        ac    += e * d->anticonvulsant;
        mapD  += e * d->mapDelta;
        hrD   += e * d->hrDelta;
        rr    += e * d->rrDelta;

        if (d->adrenalSuppress) adr += e;
        if (d->bronchodilator)  br  += e;
        if (d->raisesICP)       icp += e;
    }

    /* Benzodiazepines have a ceiling effect on hypnosis. */
    benzoHyp = fminf(benzoHyp, 0.34f);
    hyp += benzoHyp;

    *outHyp      = hyp;
    *outAnalg    = analg;
    *outAnx      = anx;
    *outAntiConv = ac;
    *outMap      = mapD;
    *outHR       = hrD;
    *outRR       = rr;
    *outAdrenal  = adr;
    *outBronch   = br;
    *outICP      = icp;
}

static void AdministerDrug(DrugId id, float units)
{
    if (id == DRUG_FLUMAZENIL) {
        gDrugPK[DRUG_MIDAZOLAM].plasma *= 0.18f;
        gDrugPK[DRUG_MIDAZOLAM].effect *= 0.10f;
        gDrugPK[DRUG_DIAZEPAM].plasma  *= 0.22f;
        gDrugPK[DRUG_DIAZEPAM].effect  *= 0.12f;
        gDrugPK[DRUG_MIDAZOLAM].exposure *= 0.6f;
        gDrugPK[DRUG_DIAZEPAM].exposure  *= 0.6f;
        SpawnFloater((Vector2){ ARENA_CX, ARENA_CY - 90.0f },
                     "BENZO REVERSED", (Color){ 255, 255, 255, 255 }, 20.0f);
        return;
    }

    gDrugPK[id].plasma += units;
    if (gDrugPK[id].plasma > 4.0f) gDrugPK[id].plasma = 4.0f;
    gDrugPK[id].totalGiven += units;

    gStatTotalDrug += units;
    if (id == DRUG_PROPOFOL)  gStatPropofol  += units;
    if (id == DRUG_KETAMINE)  gStatKetamine  += units;
    if (id == DRUG_ETOMIDATE) gStatEtomidate += units;
}

static void UpdatePharmacokinetics(float dt)
{
    for (int i = 0; i < DRUG_COUNT; i++) {
        DrugPK *pk = &gDrugPK[i];
        const DrugDef *d = &DRUGS[i];

        float effKe = d->ke / (1.0f + d->cshtGrowth * pk->exposure);

        pk->plasma -= pk->plasma * effKe * dt;
        if (pk->plasma < 0.0f) pk->plasma = 0.0f;

        if (pk->plasma > 0.04f) {
            pk->exposure += dt * 0.020f;
            if (pk->exposure > 9.0f) pk->exposure = 9.0f;
        } else {
            pk->exposure -= dt * 0.045f;
            if (pk->exposure < 0.0f) pk->exposure = 0.0f;
        }

        pk->effect += (pk->plasma - pk->effect) * d->ke0 * dt;
        if (pk->effect < 0.0f) pk->effect = 0.0f;
    }
}

static void UpdateSympatheticTone(float dt)
{
    float raw = 0.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &gEnemies[i];
        if (!e->active) continue;
        const EnemyDef *def = &ENEMIES[e->type];
        float d = Vector2Distance(e->pos, (Vector2){ ARENA_CX, ARENA_CY });
        float prox = Clampf(1.0f - d / 620.0f, 0.0f, 1.0f);
        prox = prox * prox;
        raw += def->sympathetic * prox;
    }
    raw = Clampf(raw, 0.0f, 2.2f);
    gPatient.sympatheticRaw = raw;
    gPatient.sympathetic = SmoothTowards(gPatient.sympathetic, raw, 1.6f, dt);
}

static void UpdatePatient(float dt)
{
    UpdatePharmacokinetics(dt);
    UpdateSympatheticTone(dt);

    float hyp, analg, anx, anticonv, mapD, hrD, rrDep, adrSupp, bronch, icp;
    ComputeDrugEffects(&hyp, &analg, &anx, &anticonv,
                       &mapD, &hrD, &rrDep, &adrSupp, &bronch, &icp);

    gPatient.totalHypnosis   = hyp;
    gPatient.totalAnalgesia  = analg;
    gPatient.totalAnxiolysis = anx;

    float S = gPatient.sympathetic;

    /* Adrenocortical suppression from etomidate. */
    gPatient.adrenalReserve -= adrSupp * dt * 0.055f;
    gPatient.adrenalReserve += dt * 0.0060f * (1.0f - gPatient.adrenalReserve);
    gPatient.adrenalReserve = Clampf(gPatient.adrenalReserve, 0.0f, 1.0f);

    float adrenalPenalty = 0.0f;
    if (gPatient.adrenalReserve < 0.55f) {
        adrenalPenalty = (0.55f - gPatient.adrenalReserve) * 62.0f;
    }

    /* BIS */
    float targetBIS = 100.0f - 108.0f * hyp + 58.0f * S;
    if (targetBIS > 100.0f) targetBIS = 100.0f;
    if (targetBIS < 0.0f)   targetBIS = 0.0f;
    gPatient.bis = SmoothTowards(gPatient.bis, targetBIS, 2.2f, dt);

    /* MAP */
    float targetMAP = 88.0f;
    targetMAP += 34.0f * S;
    targetMAP += mapD;
    targetMAP -= adrenalPenalty;
    targetMAP += 14.0f * (analg * 0.25f);
    targetMAP = Clampf(targetMAP, 12.0f, 210.0f);
    gPatient.map = SmoothTowards(gPatient.map, targetMAP, 1.5f, dt);

    /* HR */
    float targetHR = 76.0f;
    targetHR += 34.0f * S;
    targetHR += hrD;
    targetHR -= 8.0f * hyp;
    targetHR = Clampf(targetHR, 18.0f, 190.0f);
    gPatient.hr = SmoothTowards(gPatient.hr, targetHR, 1.4f, dt);

    /* Ventilation */
    float ventDepression = Clampf(rrDep, 0.0f, 1.9f);
    float ventilationFactor = 1.0f - ventDepression;
    if (ventilationFactor < 0.05f) ventilationFactor = 0.05f;

    gPatient.etco2 += (1.0f - ventilationFactor) * 22.0f * dt;
    gPatient.etco2 -= (ventilationFactor - 1.0f) * 6.0f * dt;
    gPatient.etco2 = Clampf(gPatient.etco2, 8.0f, 130.0f);

    float spo2Target = 99.0f;
    if (ventilationFactor < 0.75f) {
        spo2Target = 99.0f - (0.75f - ventilationFactor) * 128.0f;
    }
    if (gPatient.etco2 > 60.0f) {
        spo2Target -= (gPatient.etco2 - 60.0f) * 0.75f;
    }
    spo2Target = Clampf(spo2Target, 22.0f, 100.0f);
    gPatient.spo2 = SmoothTowards(gPatient.spo2, spo2Target, 0.55f, dt);

    /* ECG */
    gPatient.ecgPhase += (gPatient.hr / 60.0f) * dt;
    if (gPatient.ecgPhase >= 1.0f) gPatient.ecgPhase -= 1.0f;

    float ecgVal = ECGSample(gPatient.ecgPhase);
    ecgVal *= (0.65f + 0.35f * (1.0f - Clampf(hyp * 0.7f, 0.0f, 0.8f)));
    if (gPatient.hr > 150.0f) ecgVal *= Randf(0.55f, 1.10f);
    gPatient.ecgHead = (gPatient.ecgHead + 1) % ECG_BUF;
    gPatient.ecg[gPatient.ecgHead] = ecgVal;

    /* Pleth */
    gPatient.plethPhase += (gPatient.hr / 60.0f) * dt;
    if (gPatient.plethPhase >= 1.0f) gPatient.plethPhase -= 1.0f;
    float plethVal = 0.0f;
    {
        float p = gPatient.plethPhase;
        if (p < 0.16f)      plethVal = sinf(p / 0.16f * PI);
        else if (p < 0.36f) plethVal = 0.62f - (p - 0.16f) * 1.6f;
        else if (p < 0.52f) plethVal = 0.30f * sinf((p - 0.36f) / 0.16f * PI);
        else                plethVal = 0.0f;
    }
    plethVal *= Clampf((gPatient.spo2 - 60.0f) / 40.0f, 0.15f, 1.0f);
    gPatient.plethHead = (gPatient.plethHead + 1) % ECG_BUF;
    gPatient.pleth[gPatient.plethHead] = plethVal;

    /* Capnograph */
    gPatient.capnoPhase += (gPatient.hr / 60.0f) * dt;
    if (gPatient.capnoPhase >= 1.0f) gPatient.capnoPhase -= 1.0f;
    float capnoVal = 0.0f;
    {
        float p = gPatient.capnoPhase;
        if (p < 0.10f)      capnoVal = p / 0.10f;
        else if (p < 0.62f) capnoVal = 1.0f - (p - 0.10f) * 0.10f;
        else                capnoVal = 0.94f * (1.0f - (p - 0.62f) / 0.38f);
        capnoVal *= Clampf((gPatient.etco2 - 5.0f) / 60.0f, 0.0f, 1.3f);
    }
    gPatient.capnoHead = (gPatient.capnoHead + 1) % ECG_BUF;
    gPatient.capno[gPatient.capnoHead] = capnoVal;

    /* EEG / BIS proxy */
    float eegVal;
    float awake = Clampf(gPatient.bis / 100.0f, 0.0f, 1.0f);
    if (gPatient.bis < 8.0f) {
        eegVal = 0.0f;
    } else if (gPatient.bis < 25.0f) {
        /* Burst suppression: short bursts on a fast carrier. */
        float burstCycle = fmodf(gGameTime * 1.5f, 1.0f);
        float burst = (burstCycle < 0.14f) ? 1.0f : 0.0f;
        eegVal = burst * Randf(-0.75f, 0.75f);
    } else {
        float amp = 0.22f + 0.78f * awake;
        eegVal = amp * sinf(gGameTime * (14.0f + 20.0f * awake)
                            + (float)gPatient.eegHead * 0.7f);
        eegVal += 0.22f * Randf(-1.0f, 1.0f) * awake;
        eegVal *= 0.35f + 0.65f * awake;
    }
    gPatient.eegHead = (gPatient.eegHead + 1) % ECG_BUF;
    gPatient.eeg[gPatient.eegHead] = eegVal;

    /* Damage accumulation */
    float dmg = 0.0f;

    if (gPatient.map < 55.0f) {
        dmg += (55.0f - gPatient.map) * 0.030f;
        gStatHypotensionTime += dt;
    }
    if (gPatient.map > 145.0f) {
        dmg += (gPatient.map - 145.0f) * 0.024f;
    }
    if (gPatient.hr < 42.0f) {
        dmg += (42.0f - gPatient.hr) * 0.045f;
    }
    if (gPatient.hr > 145.0f) {
        dmg += (gPatient.hr - 145.0f) * 0.030f;
    }
    if (gPatient.spo2 < 90.0f) {
        dmg += (90.0f - gPatient.spo2) * 0.16f;
        gStatHypoxiaTime += dt;
    }
    if (gPatient.bis > 72.0f && gPatient.sympathetic > 0.18f) {
        dmg += (gPatient.bis - 72.0f) * 0.055f;
        gStatAwarenessTime += dt;
    }
    if (gPatient.etco2 > 75.0f) {
        dmg += (gPatient.etco2 - 75.0f) * 0.02f;
    }
    if (gPatient.adrenalReserve < 0.20f) {
        dmg += (0.20f - gPatient.adrenalReserve) * 6.0f;
    }

    gPatient.lastDamage = dmg;
    if (dmg > 0.0f) gPatient.health -= dmg * dt;
    else            gPatient.health += 0.55f * dt;

    gPatient.health = Clampf(gPatient.health, 0.0f, gPatient.healthMax);

    if (gPatient.health <= 0.0f) gPatient.alive = false;
}

/* ============================================================================
 *  SECTION 11 -- PLAYER
 * ==========================================================================*/

static void PlayerInit(void)
{
    gPlayer.pos          = (Vector2){ ARENA_CX, ARENA_CY - 190.0f };
    gPlayer.vel          = (Vector2){ 0.0f, 0.0f };
    gPlayer.radius       = 17.0f;
    gPlayer.speed        = 320.0f;
    gPlayer.aimAngle     = PI * 0.5f;
    gPlayer.recoil       = 0.0f;
    gPlayer.selectedDrug = DRUG_PROPOFOL;
    gPlayer.bob          = 0.0f;
    gPlayer.whiteCoatFlap= 0.0f;
    gPlayer.syringeGlow  = 0.0f;
    gPlayer.shotsFired   = 0;
    gPlayer.hits         = 0;
    for (int i = 0; i < DRUG_COUNT; i++) gPlayer.cooldowns[i] = 0.0f;
}

static void UpdatePlayer(float dt)
{
    Vector2 dir = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dir.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dir.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1.0f;

    if (VecLenSqr(dir) > 0.0f) dir = Vector2Normalize(dir);

    gPlayer.vel = Vector2Lerp(gPlayer.vel,
                              Vector2Scale(dir, gPlayer.speed),
                              1.0f - Clampf(14.0f * dt, 0.0f, 1.0f));
    gPlayer.pos = Vector2Add(gPlayer.pos, Vector2Scale(gPlayer.vel, dt));

    gPlayer.pos.x = Clampf(gPlayer.pos.x, 40.0f, (float)SCREEN_W - 40.0f);
    gPlayer.pos.y = Clampf(gPlayer.pos.y, 210.0f, (float)SCREEN_H - 118.0f);

    /* Head-of-bed exclusion zone. */
    {
        float dx = (gPlayer.pos.x - ARENA_CX) / (PATIENT_RADIUS + 34.0f);
        float dy = (gPlayer.pos.y - ARENA_CY) / (PATIENT_RADIUS * 0.52f + 34.0f);
        float r = dx * dx + dy * dy;
        if (r < 1.0f && r > 0.0001f) {
            float inv = 1.0f / sqrtf(r);
            gPlayer.pos.x = ARENA_CX + dx * inv * (PATIENT_RADIUS + 34.0f);
            gPlayer.pos.y = ARENA_CY + dy * inv * (PATIENT_RADIUS * 0.52f + 34.0f);
        }
    }

    float speed = Vector2Length(gPlayer.vel);
    gPlayer.bob += dt * (4.0f + speed * 0.02f);
    gPlayer.whiteCoatFlap = Lerpf(gPlayer.whiteCoatFlap,
                                  Clampf(speed / 320.0f, 0.0f, 1.0f),
                                  8.0f * dt);

    Vector2 mouse = GetMousePosition();
    gPlayer.aimAngle = AngleTo(gPlayer.pos, mouse);

    gPlayer.recoil      = Lerpf(gPlayer.recoil, 0.0f, 14.0f * dt);
    gPlayer.syringeGlow = Lerpf(gPlayer.syringeGlow, 0.0f, 6.0f * dt);

    for (int i = 0; i < DRUG_COUNT; i++) {
        if (gPlayer.cooldowns[i] > 0.0f) {
            gPlayer.cooldowns[i] -= dt;
            if (gPlayer.cooldowns[i] < 0.0f) gPlayer.cooldowns[i] = 0.0f;
        }
    }

    for (int i = 0; i < 9; i++) {
        if (IsKeyPressed(KEY_ONE + i)) gPlayer.selectedDrug = i;
    }
    if (IsKeyPressed(KEY_ZERO)) gPlayer.selectedDrug = DRUG_FLUMAZENIL;
    if (IsKeyPressed(KEY_Q)) {
        gPlayer.selectedDrug = (gPlayer.selectedDrug + DRUG_COUNT - 1) % DRUG_COUNT;
    }
    if (IsKeyPressed(KEY_E)) {
        gPlayer.selectedDrug = (gPlayer.selectedDrug + 1) % DRUG_COUNT;
    }
    float wheel = GetMouseWheelMove();
    if (wheel > 0.1f) {
        gPlayer.selectedDrug = (gPlayer.selectedDrug + DRUG_COUNT - 1) % DRUG_COUNT;
    } else if (wheel < -0.1f) {
        gPlayer.selectedDrug = (gPlayer.selectedDrug + 1) % DRUG_COUNT;
    }

    bool wantFire = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE);
    if (wantFire && gPlayer.cooldowns[gPlayer.selectedDrug] <= 0.0f) {
        FireSyringe();
    }
}

/* ============================================================================
 *  SECTION 12 -- PROJECTILES
 * ==========================================================================*/

static void ProjectilesClear(void)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) gProjectiles[i].active = false;
}

static void UpdateProjectiles(float dt)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &gProjectiles[i];
        if (!p->active) continue;

        p->life -= dt;
        if (p->life <= 0.0f) { p->active = false; continue; }

        p->pos = Vector2Add(p->pos, Vector2Scale(p->vel, dt));
        p->vel = Vector2Scale(p->vel, 1.0f - Clampf(1.2f * dt, 0.0f, 1.0f));
        p->spin += dt * 14.0f;

        p->trailTimer -= dt;
        if (p->trailTimer <= 0.0f) {
            p->trailTimer = 0.018f;
            Color c = DRUGS[p->drug].color;
            c.a = 130;
            SpawnParticle(p->pos,
                          (Vector2){ Randf(-14.0f, 14.0f),
                                     Randf(-14.0f, 14.0f) },
                          0.28f, p->radius * 0.65f, c, 0, 3.0f);
        }

        if (p->pos.x < -60.0f || p->pos.x > (float)SCREEN_W + 60.0f ||
            p->pos.y < -60.0f || p->pos.y > (float)SCREEN_H + 60.0f) {
            p->active = false;
            continue;
        }

        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy *e = &gEnemies[j];
            if (!e->active) continue;
            if (CheckCollisionCircles(p->pos, p->radius, e->pos, e->radius)) {
                DrugHitEnemy(p->drug, e, p->pos);
                p->active = false;
                break;
            }
        }
    }
}

static void DrawProjectiles(void)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &gProjectiles[i];
        if (!p->active) continue;
        const DrugDef *d = &DRUGS[p->drug];

        Vector2 dir = Vector2Normalize(p->vel);
        if (VecLenSqr(p->vel) < 0.001f) dir = (Vector2){ 1.0f, 0.0f };
        Vector2 perp = (Vector2){ -dir.y, dir.x };

        Vector2 tail = Vector2Subtract(p->pos, Vector2Scale(dir, p->radius * 2.4f));
        Vector2 head = Vector2Add(p->pos, Vector2Scale(dir, p->radius * 1.1f));

        Color body = d->color;
        body.a = 235;

        DrawLineEx(tail, head, p->radius * 1.05f, body);

        Vector2 plunger = Vector2Subtract(tail, Vector2Scale(dir, p->radius * 0.9f));
        DrawLineEx(Vector2Add(plunger, Vector2Scale(perp, p->radius * 0.7f)),
                   Vector2Subtract(plunger, Vector2Scale(perp, p->radius * 0.7f)),
                   3.0f, body);

        Vector2 tip = Vector2Add(head, Vector2Scale(dir, p->radius * 1.4f));
        DrawLineEx(head, tip, 2.0f, (Color){ 220, 220, 230, 220 });

        Color glow = body;
        glow.a = 60;
        DrawCircleV(p->pos, p->radius * 3.0f, glow);
    }
}

/* ============================================================================
 *  SECTION 13 -- ENEMIES
 * ==========================================================================*/

static void EnemiesClear(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++) gEnemies[i].active = false;
}

static int CountAliveEnemies(void)
{
    int n = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) if (gEnemies[i].active) n++;
    return n;
}

static void SpawnEnemy(EnemyType type, float hpMul, float speedMul)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (gEnemies[i].active) continue;

        const EnemyDef *def = &ENEMIES[type];
        Enemy *e = &gEnemies[i];

        float side = (float)Randi(0, 4);
        Vector2 p;
        float pad = 60.0f;
        if (side < 1.0f) {
            p = (Vector2){ Randf(80.0f, (float)SCREEN_W - 80.0f), -pad };
        } else if (side < 2.0f) {
            p = (Vector2){ Randf(80.0f, (float)SCREEN_W - 80.0f),
                           (float)SCREEN_H + pad };
        } else if (side < 3.0f) {
            p = (Vector2){ -pad, Randf(220.0f, (float)SCREEN_H - 120.0f) };
        } else {
            p = (Vector2){ (float)SCREEN_W + pad,
                           Randf(220.0f, (float)SCREEN_H - 120.0f) };
        }

        e->active       = true;
        e->type         = type;
        e->pos          = p;
        e->vel          = (Vector2){ 0.0f, 0.0f };
        e->hpMax        = def->hp * hpMul;
        e->hp           = e->hpMax;
        e->radius       = def->radius;
        e->speed        = def->speed * speedMul;
        e->hitFlash     = 0.0f;
        e->slowTimer    = 0.0f;
        e->wobble       = Randf(0.0f, 10.0f);
        e->spawnAnim    = 0.0f;
        e->pulse        = 0.0f;
        e->inContact    = false;
        e->contactTimer = 0.0f;
        e->damageDealt  = 0.0f;
        e->id           = gNextEnemyId++;
        return;
    }
}

static void DamageEnemy(Enemy *e, float amount, Vector2 hitPos, Color color)
{
    const EnemyDef *def = &ENEMIES[e->type];

    if (amount > 0.0f) {
        e->hp -= amount;
        e->hitFlash = 0.22f;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "-%d", (int)roundf(amount));
        SpawnFloater((Vector2){ e->pos.x, e->pos.y - e->radius - 6.0f },
                     buf, color, 17.0f);
        Burst(hitPos, 6, color, 190.0f, 3.2f, 0);
        gPlayer.hits++;
    } else if (amount < 0.0f) {
        float heal = -amount * 0.75f;
        e->hp = fminf(e->hp + heal, e->hpMax);
        SpawnFloater((Vector2){ e->pos.x, e->pos.y - e->radius - 6.0f },
                     "WORSE", (Color){ 255, 90, 90, 255 }, 19.0f);
        Burst(hitPos, 8, (Color){ 255, 60, 60, 255 }, 220.0f, 4.0f, 1);
    }

    if (e->hp <= 0.0f && e->active) {
        e->active = false;
        gKills++;
        gEnemiesKilledThisWave++;
        gScore += def->isBoss ? 5000.0f : (40.0f + e->hpMax * 0.35f);

        Color c = def->color;
        Burst(e->pos, def->isBoss ? 90 : 20, c,
              def->isBoss ? 420.0f : 250.0f,
              def->isBoss ? 8.0f : 4.0f, 2);
        Burst(e->pos, def->isBoss ? 50 : 12,
              (Color){ 255, 255, 255, 200 },
              def->isBoss ? 300.0f : 180.0f, 3.0f, 0);

        if (def->isBoss) {
            gShake = 22.0f;
            gFlashWhite = 0.9f;
            SpawnFloater(e->pos, "SUPPRESSED",
                         (Color){ 120, 255, 160, 255 }, 30.0f);
        } else {
            gShake = fmaxf(gShake, 3.5f);
        }
    }
}

static void UpdateEnemies(float dt)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &gEnemies[i];
        if (!e->active) continue;

        const EnemyDef *def = &ENEMIES[e->type];

        e->spawnAnim += dt * 3.0f;
        e->pulse     += dt * 3.4f;
        if (e->hitFlash > 0.0f)  e->hitFlash -= dt;
        if (e->slowTimer > 0.0f) e->slowTimer -= dt;
        e->wobble += dt * 2.6f;

        Vector2 toPatient = Vector2Subtract((Vector2){ ARENA_CX, ARENA_CY },
                                            e->pos);
        float dist = Vector2Length(toPatient);
        Vector2 dir = (dist > 0.001f) ? Vector2Scale(toPatient, 1.0f / dist)
                                      : (Vector2){ 0.0f, 0.0f };

        Vector2 perp = (Vector2){ -dir.y, dir.x };
        float wob = sinf(e->wobble) * 0.35f;

        float contactRange = PATIENT_RADIUS + e->radius;

        if (dist > contactRange) {
            float spd = e->speed * def->aggression;
            float healthFrac = e->hp / e->hpMax;
            if (healthFrac < 0.55f) spd *= 0.62f;

            Vector2 move = Vector2Add(Vector2Scale(dir, 1.0f),
                                      Vector2Scale(perp, wob));
            move = Vector2Normalize(move);
            e->pos = Vector2Add(e->pos, Vector2Scale(move, spd * dt));
            e->inContact = false;
            e->contactTimer = 0.0f;
        } else {
            e->inContact = true;
            e->contactTimer += dt;

            if (e->type == ENEMY_AWARENESS_PARALYSIS) {
                if (e->contactTimer > 1.1f) {
                    e->contactTimer = 0.0f;
                    gPatient.health -= 14.0f;
                    gShake = 16.0f;
                    gFlashRed = 0.7f;
                    SpawnFloater((Vector2){ ARENA_CX, ARENA_CY },
                                 "RECALL!", (Color){ 255, 60, 60, 255 }, 30.0f);
                    Burst((Vector2){ ARENA_CX, ARENA_CY }, 40,
                          (Color){ 255, 40, 40, 255 }, 340.0f, 6.0f, 1);
                }
            }
        }

        if (dist < contactRange + 8.0f) {
            float dmg = def->contactDamage * dt;
            gPatient.health -= dmg;
            e->damageDealt += dmg;
        }
    }
}

/* ============================================================================
 *  SECTION 14 -- PHARMACOLOGY (damage model)
 * ==========================================================================*/

static float DrugDamageVsEnemy(DrugId id, const Enemy *e)
{
    const DrugDef  *d  = &DRUGS[id];
    const EnemyDef *ed = &ENEMIES[e->type];

    float h = d->hypnotic       * ed->hypnoticSens;
    float a = d->analgesic      * ed->analgesicSens;
    float x = d->anxiolytic     * ed->anxiolyticSens;
    float c = d->anticonvulsant * ed->anticonvulsantSens;

    float total = h + a + x + c;

    if (d->bronchodilator && e->type == ENEMY_BRONCHOSPASM)   total *= 1.55f;
    if (d->antiemetic     && e->type == ENEMY_PONV)           total *= 1.70f;
    if (id == DRUG_DEXMEDETOMIDINE &&
        e->type == ENEMY_EMERGENCE_DELIRIUM)                  total *= 1.75f;
    if (id == DRUG_PROPOFOL &&
        e->type == ENEMY_LARYNGOSPASM)                        total *= 1.45f;
    if (id == DRUG_KETAMINE &&
        e->type == ENEMY_NOCICEPTION)                         total *= 1.20f;
    if (d->raisesICP &&
        e->type == ENEMY_SYMPATHETIC_SURGE)                   total *= 0.55f;

    const float DAMAGE_SCALE = 30.0f;
    return total * d->potencyScale * DAMAGE_SCALE;
}

static void DrugHitEnemy(DrugId id, Enemy *e, Vector2 hitPos)
{
    if (id == DRUG_FLUMAZENIL) {
        const DrugDef *d = &DRUGS[id];
        float dmg = d->anticonvulsant * ENEMIES[e->type].anticonvulsantSens
                    * 30.0f;
        DamageEnemy(e, dmg, hitPos, (Color){ 255, 255, 255, 255 });
        return;
    }

    float dmg = DrugDamageVsEnemy(id, e);
    Color c = DRUGS[id].color;
    DamageEnemy(e, dmg, hitPos, c);
    Burst(hitPos, 4, c, 150.0f, 2.6f, 0);
}

/* ============================================================================
 *  SECTION 15 -- FIRING
 * ==========================================================================*/

static void FireSyringe(void)
{
    DrugId id = (DrugId)gPlayer.selectedDrug;
    const DrugDef *d = &DRUGS[id];

    if (gPlayer.cooldowns[id] > 0.0f) return;

    gPlayer.cooldowns[id] = d->fireCooldown;
    gPlayer.recoil        = 1.0f;
    gPlayer.syringeGlow   = 1.0f;
    gPlayer.shotsFired++;

    float units = 0.22f;
    if (id == DRUG_FLUMAZENIL)        units = 0.00f;
    if (id == DRUG_DEXMEDETOMIDINE)   units = 0.17f;
    if (id == DRUG_ETOMIDATE)         units = 0.24f;
    if (id == DRUG_THIOPENTAL)        units = 0.20f;
    AdministerDrug(id, units);

    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (gProjectiles[i].active) continue;

        Vector2 muzzle = Vector2Add(gPlayer.pos,
                                    Vector2Scale(VecFromAngle(gPlayer.aimAngle),
                                                 gPlayer.radius + 14.0f));
        float spread = Randf(-0.030f, 0.030f);
        float angle = gPlayer.aimAngle + spread;

        gProjectiles[i].active     = true;
        gProjectiles[i].pos        = muzzle;
        gProjectiles[i].vel        = Vector2Scale(VecFromAngle(angle),
                                                  d->projectileSpeed);
        gProjectiles[i].radius     = 7.0f;
        gProjectiles[i].life       = 1.4f;
        gProjectiles[i].maxLife    = 1.4f;
        gProjectiles[i].drug       = id;
        gProjectiles[i].spin       = 0.0f;
        gProjectiles[i].trailTimer = 0.0f;
        break;
    }

    Color c = d->color;
    Burst(Vector2Add(gPlayer.pos,
                     Vector2Scale(VecFromAngle(gPlayer.aimAngle),
                                  gPlayer.radius + 10.0f)),
          5, c, 130.0f, 2.4f, 0);

    gPlayer.vel = Vector2Subtract(gPlayer.vel,
                                  Vector2Scale(VecFromAngle(gPlayer.aimAngle),
                                               42.0f));
}

/* ============================================================================
 *  SECTION 16 -- WAVE DIRECTOR
 * ==========================================================================*/

static void WaveBegin(int index)
{
    if (index >= WAVE_COUNT) {
        gState = STATE_VICTORY;
        return;
    }

    gCurrentWave = index;
    const WaveDef *w = &WAVES[index];

    gWaveQueueCount = 0;
    gWaveQueueHead  = 0;
    gSpawnTimer     = 0.0f;
    gEnemiesKilledThisWave = 0;
    gTotalEnemiesThisWave  = w->spawnCount;

    for (int i = 0; i < w->spawnCount && i < WAVE_QUEUE_MAX; i++) {
        gWaveQueue[gWaveQueueCount++] = w->pool[i % 6];
    }

    gState = STATE_WAVE_INTRO;
    gWaveIntroTimer = w->boss ? 4.4f : 3.0f;
}

static void UpdateWaveDirector(float dt)
{
    if (gState == STATE_WAVE_INTRO) {
        gWaveIntroTimer -= dt;
        if (gWaveIntroTimer <= 0.0f) gState = STATE_PLAYING;
        return;
    }

    if (gState != STATE_PLAYING) return;

    const WaveDef *w = &WAVES[gCurrentWave];

    if (gWaveQueueHead < gWaveQueueCount) {
        gSpawnTimer -= dt;
        if (gSpawnTimer <= 0.0f) {
            if (CountAliveEnemies() < w->maxAlive) {
                SpawnEnemy((EnemyType)gWaveQueue[gWaveQueueHead],
                           w->hpMul, w->speedMul);
                gWaveQueueHead++;
                gSpawnTimer = w->spawnInterval * Randf(0.75f, 1.25f);
            } else {
                gSpawnTimer = 0.25f;
            }
        }
    }

    if (gWaveQueueHead >= gWaveQueueCount && CountAliveEnemies() == 0) {
        if (gCurrentWave + 1 >= WAVE_COUNT) {
            gState = STATE_VICTORY;
        } else {
            gScore += 500.0f + gPatient.health * 12.0f;
            WaveBegin(gCurrentWave + 1);
        }
    }
}

/* ============================================================================
 *  SECTION 17 -- HUD / MONITOR
 * ==========================================================================*/

static void DrawWaveform(const float *buf, int head, int count,
                         Rectangle rect, Color color, float gain, float baseline)
{
    if (count > ECG_BUF) count = ECG_BUF;
    if (count < 2) return;

    float stepX = rect.width / (float)(count - 1);
    Vector2 prev = { 0.0f, 0.0f };
    bool havePrev = false;

    for (int i = 0; i < count; i++) {
        int idx = (head + 1 + i) % ECG_BUF;
        float v = buf[idx];
        float y = rect.y + rect.height * baseline - v * rect.height * gain;
        y = Clampf(y, rect.y + 1.0f, rect.y + rect.height - 1.0f);
        Vector2 p = { rect.x + i * stepX, y };

        if (havePrev) DrawLineEx(prev, p, 1.6f, color);
        prev = p;
        havePrev = true;
    }
}

static Color VitalsColor(float value, float lo, float hi)
{
    if (value < lo || value > hi) return (Color){ 255, 70, 70, 255 };
    return (Color){ 120, 255, 140, 255 };
}

static void DrawMonitorPanel(void)
{
    Rectangle panel = { MONITOR_X, MONITOR_Y, MONITOR_W, MONITOR_H };

    DrawRectangleRounded(panel, 0.06f, 6, (Color){ 8, 12, 16, 225 });
    DrawRectangleRoundedLines(panel, 0.06f, 6, (Color){ 60, 78, 92, 255 });

    float x = panel.x + 10.0f;
    float y = panel.y + 8.0f;
    float w = panel.width - 20.0f;

    DrawTxt("PATIENT MONITOR", x, y, 15.0f, (Color){ 130, 160, 180, 255 });

    char buf[128];

    float healthFrac = gPatient.health / gPatient.healthMax;
    Color hcol = (healthFrac > 0.6f) ? (Color){ 120, 255, 140, 255 }
               : (healthFrac > 0.3f) ? (Color){ 255, 210, 90, 255 }
                                     : (Color){ 255, 70, 70, 255 };
    std::snprintf(buf, sizeof(buf), "VIABILITY %d%%",
                  (int)roundf(gPatient.health));
    DrawTxtRight(buf, panel.x + panel.width - 12.0f, y, 15.0f, hcol);

    y += 22.0f;

    Rectangle ecgRect = { x, y, w, 44.0f };
    DrawRectangleRec(ecgRect, (Color){ 4, 8, 6, 255 });
    DrawRectangleLinesEx(ecgRect, 1.0f, (Color){ 30, 60, 40, 255 });

    float hrGain = (gPatient.hr > 150.0f) ? 0.72f : 1.0f;
    DrawWaveform(gPatient.ecg, gPatient.ecgHead, ECG_BUF - 40,
                 ecgRect, (Color){ 90, 255, 120, 255 }, hrGain, 0.66f);

    std::snprintf(buf, sizeof(buf), "%3d", (int)roundf(gPatient.hr));
    DrawTxt(buf, ecgRect.x + 6.0f, ecgRect.y + 3.0f, 24.0f,
            VitalsColor(gPatient.hr, 45.0f, 140.0f));
    DrawTxt("HR", ecgRect.x + 6.0f, ecgRect.y + 28.0f, 12.0f,
            (Color){ 90, 130, 100, 255 });

    y += 48.0f;

    Rectangle plethRect = { x, y, w * 0.5f - 4.0f, 36.0f };
    DrawRectangleRec(plethRect, (Color){ 4, 6, 10, 255 });
    DrawRectangleLinesEx(plethRect, 1.0f, (Color){ 30, 45, 70, 255 });
    DrawWaveform(gPatient.pleth, gPatient.plethHead, 220,
                 plethRect, (Color){ 90, 190, 255, 255 }, 0.9f, 0.94f);

    std::snprintf(buf, sizeof(buf), "%3d", (int)roundf(gPatient.spo2));
    DrawTxt(buf, plethRect.x + 5.0f, plethRect.y + 2.0f, 20.0f,
            VitalsColor(gPatient.spo2, 92.0f, 101.0f));
    DrawTxt("SpO2", plethRect.x + 5.0f, plethRect.y + 23.0f, 10.0f,
            (Color){ 80, 110, 150, 255 });

    Rectangle capnoRect = { x + w * 0.5f + 4.0f, y, w * 0.5f - 4.0f, 36.0f };
    DrawRectangleRec(capnoRect, (Color){ 10, 8, 4, 255 });
    DrawRectangleLinesEx(capnoRect, 1.0f, (Color){ 70, 60, 30, 255 });
    DrawWaveform(gPatient.capno, gPatient.capnoHead, 220,
                 capnoRect, (Color){ 255, 210, 80, 255 }, 0.72f, 0.96f);

    std::snprintf(buf, sizeof(buf), "%3d", (int)roundf(gPatient.etco2));
    DrawTxt(buf, capnoRect.x + 5.0f, capnoRect.y + 2.0f, 20.0f,
            VitalsColor(gPatient.etco2, 30.0f, 50.0f));
    DrawTxt("EtCO2", capnoRect.x + 5.0f, capnoRect.y + 23.0f, 10.0f,
            (Color){ 150, 125, 60, 255 });

    y += 40.0f;

    {
        float barW = w * 0.5f - 6.0f;
        float barH = 12.0f;

        Rectangle mapBar = { x, y + 4.0f, barW, barH };
        DrawRectangleRec(mapBar, (Color){ 18, 20, 24, 255 });
        float mapFrac = Clampf((gPatient.map - 20.0f) / 150.0f, 0.0f, 1.0f);
        Color mapCol = VitalsColor(gPatient.map, 60.0f, 130.0f);
        DrawRectangleRec((Rectangle){ mapBar.x, mapBar.y, barW * mapFrac, barH },
                         mapCol);
        DrawRectangleLinesEx(mapBar, 1.0f, (Color){ 50, 60, 70, 255 });

        float lo = (55.0f - 20.0f) / 150.0f * barW;
        float hi = (130.0f - 20.0f) / 150.0f * barW;
        DrawLineEx((Vector2){ x + lo, mapBar.y },
                   (Vector2){ x + lo, mapBar.y + barH }, 1.0f,
                   (Color){ 255, 255, 255, 110 });
        DrawLineEx((Vector2){ x + hi, mapBar.y },
                   (Vector2){ x + hi, mapBar.y + barH }, 1.0f,
                   (Color){ 255, 255, 255, 110 });

        std::snprintf(buf, sizeof(buf), "MAP %3d", (int)roundf(gPatient.map));
        DrawTxt(buf, x, y - 8.0f, 12.0f, (Color){ 180, 200, 215, 255 });

        float bx = x + w * 0.5f + 6.0f;
        Rectangle bisBar = { bx, y + 4.0f, barW, barH };
        DrawRectangleRec(bisBar, (Color){ 18, 20, 24, 255 });
        float bisFrac = Clampf(gPatient.bis / 100.0f, 0.0f, 1.0f);
        Color bisCol;
        if (gPatient.bis > 70.0f && gPatient.sympathetic > 0.18f)
            bisCol = (Color){ 255, 70, 70, 255 };
        else if (gPatient.bis >= 35.0f && gPatient.bis <= 65.0f)
            bisCol = (Color){ 120, 255, 140, 255 };
        else
            bisCol = (Color){ 130, 200, 255, 255 };
        DrawRectangleRec((Rectangle){ bisBar.x, bisBar.y, barW * bisFrac, barH },
                         bisCol);
        DrawRectangleLinesEx(bisBar, 1.0f, (Color){ 50, 60, 70, 255 });

        float blo = 40.0f / 100.0f * barW;
        float bhi = 60.0f / 100.0f * barW;
        DrawLineEx((Vector2){ bx + blo, bisBar.y },
                   (Vector2){ bx + blo, bisBar.y + barH }, 1.0f,
                   (Color){ 255, 255, 255, 110 });
        DrawLineEx((Vector2){ bx + bhi, bisBar.y },
                   (Vector2){ bx + bhi, bisBar.y + barH }, 1.0f,
                   (Color){ 255, 255, 255, 110 });

        std::snprintf(buf, sizeof(buf), "BIS %3d", (int)roundf(gPatient.bis));
        DrawTxt(buf, bx, y - 8.0f, 12.0f, (Color){ 180, 200, 215, 255 });
    }

    y += 24.0f;

    {
        float barW = w;
        Rectangle adrBar = { x, y, barW, 6.0f };
        DrawRectangleRec(adrBar, (Color){ 18, 20, 24, 255 });
        float frac = Clampf(gPatient.adrenalReserve, 0.0f, 1.0f);
        Color ac = (frac > 0.6f) ? (Color){ 120, 255, 140, 255 }
                 : (frac > 0.3f) ? (Color){ 255, 210, 90, 255 }
                                 : (Color){ 255, 70, 70, 255 };
        DrawRectangleRec((Rectangle){ adrBar.x, adrBar.y, barW * frac, 6.0f }, ac);
        DrawTxtRight("ADRENAL RESERVE", x + barW, y - 8.0f, 10.0f,
                     (Color){ 140, 160, 175, 255 });
    }
}

static void DrawDrugBar(void)
{
    const float slotW = 148.0f;
    const float slotH =  74.0f;
    const float gap   =   6.0f;
    int count = DRUG_COUNT;
    float totalW = count * slotW + (count - 1) * gap;
    float startX = ((float)SCREEN_W - totalW) * 0.5f;
    float y = (float)SCREEN_H - slotH - 10.0f;

    for (int i = 0; i < count; i++) {
        const DrugDef *d = &DRUGS[i];
        float x = startX + i * (slotW + gap);
        Rectangle r = { x, y, slotW, slotH };

        bool selected = (gPlayer.selectedDrug == i);

        Color bg = (Color){ 14, 18, 24, 225 };
        if (selected) bg = ColLerp((Color){ 14, 18, 24, 235 }, d->color, 0.14f);

        DrawRectangleRounded(r, 0.10f, 5, bg);
        DrawRectangleRoundedLines(r, 0.10f, 5,
                                  selected ? d->color
                                           : (Color){ 52, 64, 78, 255 });

        DrawRectangle((int)r.x + 4, (int)r.y + 4, 5, (int)r.height - 8, d->color);

        char keybuf[4];
        if (i < 9) std::snprintf(keybuf, sizeof(keybuf), "%d", i + 1);
        else       std::snprintf(keybuf, sizeof(keybuf), "0");

        DrawTxt(keybuf, r.x + 13.0f, r.y + 5.0f, 15.0f,
                selected ? (Color){ 255, 255, 255, 255 }
                         : (Color){ 110, 125, 140, 255 });

        DrawTxt(d->abbrev, r.x + 32.0f, r.y + 6.0f, 19.0f,
                selected ? (Color){ 255, 255, 255, 255 }
                         : (Color){ 190, 200, 212, 255 });

        DrawTxt(d->className, r.x + 8.0f, r.y + 28.0f, 10.0f,
                (Color){ 140, 155, 170, 255 });

        {
            float conc = gDrugPK[i].effect;
            float frac = Clampf(conc / 1.2f, 0.0f, 1.0f);
            Rectangle cb = { r.x + 8.0f, r.y + slotH - 16.0f,
                             slotW - 16.0f, 6.0f };
            DrawRectangleRec(cb, (Color){ 24, 28, 34, 255 });
            Color cc = d->color;
            cc.a = 230;
            DrawRectangleRec((Rectangle){ cb.x, cb.y, cb.width * frac, cb.height },
                             cc);
            DrawRectangleLinesEx(cb, 1.0f, (Color){ 40, 48, 58, 255 });
        }

        if (gPlayer.cooldowns[i] > 0.0f) {
            float frac = gPlayer.cooldowns[i] / d->fireCooldown;
            DrawRectangleRounded(r, 0.10f, 5,
                                 (Color){ 0, 0, 0,
                                          (unsigned char)(110 * frac) });
        }
    }

    {
        const DrugDef *d = &DRUGS[gPlayer.selectedDrug];
        float dx = 16.0f;
        float dy = y - 96.0f;
        Rectangle dr = { dx, dy, 620.0f, 88.0f };
        DrawRectangleRounded(dr, 0.10f, 5, (Color){ 10, 14, 20, 220 });
        DrawRectangleRoundedLines(dr, 0.10f, 5, (Color){ 52, 64, 78, 200 });

        DrawTxt(d->name, dr.x + 12.0f, dr.y + 8.0f, 22.0f, d->color);
        DrawTxt(d->className, dr.x + 12.0f, dr.y + 32.0f, 13.0f,
                (Color){ 150, 165, 180, 255 });

        char buf[160];
        std::snprintf(buf, sizeof(buf),
                      "HYPN %.2f   ANALG %.2f   ANXIO %.2f   AC %.2f",
                      d->hypnotic, d->analgesic, d->anxiolytic,
                      d->anticonvulsant);
        DrawTxt(buf, dr.x + 12.0f, dr.y + 50.0f, 12.0f,
                (Color){ 190, 205, 220, 255 });

        std::snprintf(buf, sizeof(buf),
                      "dMAP %+.0f   dHR %+.0f   Resp %.2f   t1/2(min) %.0f",
                      d->mapDelta, d->hrDelta, d->rrDelta, d->durationMin);
        DrawTxt(buf, dr.x + 12.0f, dr.y + 66.0f, 12.0f,
                (Color){ 160, 175, 190, 255 });
    }
}

static void DrawWaveBanner(void)
{
    if (gCurrentWave < 0 || gCurrentWave >= WAVE_COUNT) return;
    const WaveDef *w = &WAVES[gCurrentWave];

    char buf[192];
    std::snprintf(buf, sizeof(buf), "WAVE %d / %d",
                  gCurrentWave + 1, WAVE_COUNT);
    DrawTxtCentered(buf, (float)SCREEN_W * 0.5f, 18.0f, 18.0f,
                    (Color){ 150, 170, 190, 255 });

    DrawTxtCentered(w->name, (float)SCREEN_W * 0.5f, 40.0f, 30.0f,
                    w->boss ? (Color){ 255, 90, 90, 255 }
                            : (Color){ 235, 240, 245, 255 });

    int remaining = (gWaveQueueCount - gWaveQueueHead) + CountAliveEnemies();
    std::snprintf(buf, sizeof(buf), "STIMULI REMAINING  %d", remaining);
    DrawTxtCentered(buf, (float)SCREEN_W * 0.5f, 76.0f, 15.0f,
                    (Color){ 170, 185, 200, 255 });

    std::snprintf(buf, sizeof(buf), "SCORE %d", (int)roundf(gScore));
    DrawTxtRight(buf, (float)SCREEN_W - 24.0f, 18.0f, 20.0f,
                 (Color){ 220, 230, 240, 255 });

    if (gState == STATE_WAVE_INTRO) {
        float full = w->boss ? 4.4f : 3.0f;
        float t = Clampf(gWaveIntroTimer / full, 0.0f, 1.0f);
        float alpha = 1.0f - fabsf(t * 2.0f - 1.0f);
        DrawRectangle(0, 200, SCREEN_W, 180,
                      (Color){ 0, 0, 0, (unsigned char)(150 * alpha) });

        Color c = w->boss ? (Color){ 255, 80, 80, 255 }
                          : (Color){ 240, 245, 250, 255 };
        c.a = (unsigned char)(255 * alpha);
        DrawTxtCentered(w->name, (float)SCREEN_W * 0.5f, 240.0f, 46.0f, c);

        Color c2 = (Color){ 190, 205, 220, 255 };
        c2.a = (unsigned char)(230 * alpha);

        {
            const char *src = w->subtitle;
            char line[128];
            int li = 0;
            float ly = 300.0f;
            for (const char *p = src; ; p++) {
                if (*p == '\0' || li >= 110) {
                    line[li] = '\0';
                    DrawTxtCentered(line, (float)SCREEN_W * 0.5f, ly, 20.0f, c2);
                    ly += 26.0f;
                    li = 0;
                    if (*p == '\0') break;
                } else {
                    line[li++] = *p;
                }
            }
        }
    }
}

static void DrawAlarms(void)
{
    float y = 120.0f;
    float x = 24.0f;
    char buf[128];

    Color c;
    int n = 0;

    if (gPatient.map < 55.0f) {
        c = (Color){ 255, 70, 70, 255 };
        std::snprintf(buf, sizeof(buf), "!! HYPOTENSION  MAP %d",
                      (int)roundf(gPatient.map));
        DrawTxt(buf, x, y + n * 20.0f, 17.0f, c); n++;
    }
    if (gPatient.map > 145.0f) {
        c = (Color){ 255, 150, 60, 255 };
        std::snprintf(buf, sizeof(buf), "!! HYPERTENSION  MAP %d",
                      (int)roundf(gPatient.map));
        DrawTxt(buf, x, y + n * 20.0f, 17.0f, c); n++;
    }
    if (gPatient.hr < 42.0f) {
        c = (Color){ 255, 70, 70, 255 };
        std::snprintf(buf, sizeof(buf), "!! BRADYCARDIA  HR %d",
                      (int)roundf(gPatient.hr));
        DrawTxt(buf, x, y + n * 20.0f, 17.0f, c); n++;
    }
    if (gPatient.hr > 145.0f) {
        c = (Color){ 255, 150, 60, 255 };
        std::snprintf(buf, sizeof(buf), "!! TACHYCARDIA  HR %d",
                      (int)roundf(gPatient.hr));
        DrawTxt(buf, x, y + n * 20.0f, 17.0f, c); n++;
    }
    if (gPatient.spo2 < 92.0f) {
        c = (Color){ 90, 190, 255, 255 };
        std::snprintf(buf, sizeof(buf), "!! DESATURATION  SpO2 %d",
                      (int)roundf(gPatient.spo2));
        DrawTxt(buf, x, y + n * 20.0f, 17.0f, c); n++;
    }
    if (gPatient.bis > 70.0f && gPatient.sympathetic > 0.18f) {
        c = (Color){ 255, 230, 80, 255 };
        std::snprintf(buf, sizeof(buf), "!! RISK OF AWARENESS  BIS %d",
                      (int)roundf(gPatient.bis));
        DrawTxt(buf, x, y + n * 20.0f, 17.0f, c); n++;
    }
    if (gPatient.bis < 22.0f && gPatient.bis > 0.5f) {
        c = (Color){ 180, 190, 255, 255 };
        std::snprintf(buf, sizeof(buf), "!! EXCESSIVE DEPTH  BIS %d",
                      (int)roundf(gPatient.bis));
        DrawTxt(buf, x, y + n * 20.0f, 17.0f, c); n++;
    }
    if (gPatient.adrenalReserve < 0.35f) {
        c = (Color){ 255, 170, 90, 255 };
        std::snprintf(buf, sizeof(buf), "!! ADRENOCORTICAL SUPPRESSION");
        DrawTxt(buf, x, y + n * 20.0f, 17.0f, c); n++;
    }
    if (gPatient.etco2 > 60.0f) {
        c = (Color){ 255, 210, 90, 255 };
        std::snprintf(buf, sizeof(buf), "!! HYPERCAPNIA  EtCO2 %d",
                      (int)roundf(gPatient.etco2));
        DrawTxt(buf, x, y + n * 20.0f, 17.0f, c); n++;
    }
}

/* ============================================================================
 *  SECTION 18 -- WORLD RENDERING
 * ==========================================================================*/

static void DrawOperatingRoomFloor(void)
{
    ClearBackground((Color){ 16, 20, 26, 255 });

    const int tile = 64;
    for (int ty = 0; ty < SCREEN_H; ty += tile) {
        for (int tx = 0; tx < SCREEN_W; tx += tile) {
            bool alt = ((tx / tile) + (ty / tile)) % 2 == 0;
            Color c = alt ? (Color){ 20, 25, 32, 255 }
                          : (Color){ 18, 22, 29, 255 };
            DrawRectangle(tx, ty, tile - 1, tile - 1, c);
        }
    }

    for (int i = 0; i < 8; i++) {
        float r = 260.0f + i * 70.0f;
        Color c = { 40, 60, 80, (unsigned char)(8 + i * 2) };
        DrawRing((Vector2){ ARENA_CX, ARENA_CY }, r, r + 70.0f,
                 0.0f, 360.0f, 64, c);
    }
}

static void DrawPatient(void)
{
    Vector2 c = { ARENA_CX, ARENA_CY };

    DrawRectangleRounded((Rectangle){ c.x - 200.0f, c.y - 78.0f,
                                      400.0f, 156.0f },
                         0.16f, 10, (Color){ 42, 60, 74, 255 });
    DrawRectangleRoundedLines((Rectangle){ c.x - 200.0f, c.y - 78.0f,
                                           400.0f, 156.0f },
                              0.16f, 10, (Color){ 70, 96, 118, 255 });

    DrawRectangleRounded((Rectangle){ c.x - 186.0f, c.y - 66.0f,
                                      372.0f, 132.0f },
                         0.18f, 10, (Color){ 34, 96, 132, 255 });
    DrawRectangleRounded((Rectangle){ c.x - 178.0f, c.y - 58.0f,
                                      356.0f, 116.0f },
                         0.20f, 10, (Color){ 46, 122, 164, 255 });

    DrawRectangleRounded((Rectangle){ c.x - 60.0f, c.y - 40.0f,
                                      120.0f, 80.0f },
                         0.30f, 8, (Color){ 196, 168, 148, 255 });

    DrawCircleV((Vector2){ c.x - 128.0f, c.y }, 26.0f,
                (Color){ 224, 196, 172, 255 });
    DrawCircleLines((int)(c.x - 128.0f), (int)c.y, 26.0f,
                    (Color){ 120, 96, 80, 255 });

    float frac = Clampf(gPatient.health / gPatient.healthMax, 0.0f, 1.0f);
    Color ringCol;
    if (frac > 0.6f)      ringCol = (Color){ 90, 240, 130, 220 };
    else if (frac > 0.3f) ringCol = (Color){ 255, 205, 80, 220 };
    else                  ringCol = (Color){ 255, 70, 70, 240 };

    DrawRing(c, PATIENT_RADIUS + 12.0f, PATIENT_RADIUS + 20.0f,
             -90.0f, -90.0f + 360.0f * frac, 90, ringCol);
    DrawRing(c, PATIENT_RADIUS + 12.0f, PATIENT_RADIUS + 20.0f,
             -90.0f, 270.0f, 90, (Color){ 40, 48, 58, 120 });

    if (frac < 0.35f) {
        float pulse = 0.5f + 0.5f * sinf(gGameTime * 6.0f);
        Color p = { 255, 60, 60, (unsigned char)(60 * pulse) };
        DrawRing(c, PATIENT_RADIUS + 22.0f, PATIENT_RADIUS + 30.0f,
                 0.0f, 360.0f, 64, p);
    }

    {
        float depth = Clampf(gPatient.totalHypnosis, 0.0f, 1.6f) / 1.6f;
        Color aura = { 120, 140, 255, (unsigned char)(28 * depth) };
        DrawRing(c, PATIENT_RADIUS + 32.0f, PATIENT_RADIUS + 60.0f,
                 0.0f, 360.0f, 64, aura);
    }
}

static void DrawEnemyShape(Enemy *e)
{
    const EnemyDef *def = &ENEMIES[e->type];
    Color body = def->color;

    if (e->hitFlash > 0.0f) {
        float t = Clampf(e->hitFlash / 0.22f, 0.0f, 1.0f);
        body = ColLerp(body, (Color){ 255, 255, 255, 255 }, t * 0.85f);
    }

    float r = e->radius;
    float spawnScale = Clampf(e->spawnAnim, 0.0f, 1.0f);
    r *= 0.4f + 0.6f * spawnScale;

    DrawCircleV(e->pos, r + 3.0f, (Color){ 8, 10, 14, 200 });

    if (def->isBoss) {
        float pulse = 0.5f + 0.5f * sinf(e->pulse * 1.4f);
        DrawRing(e->pos, r + 6.0f, r + 12.0f + pulse * 6.0f,
                 0.0f, 360.0f, 48,
                 (Color){ 255, 50, 50, (unsigned char)(90 + 80 * pulse) });
        DrawCircleV(e->pos, r, body);
        DrawRing(e->pos, r * 0.6f, r * 0.72f, 0.0f, 360.0f, 32,
                 (Color){ 20, 0, 0, 200 });
        DrawCircleV(e->pos, r * 0.3f, (Color){ 255, 220, 220, 255 });
    } else {
        switch (e->type) {
            case ENEMY_ANXIETY:
                DrawCircleV(e->pos, r, body);
                DrawRing(e->pos, r * 0.55f, r * 0.75f, 0.0f, 360.0f, 20,
                         (Color){ 255, 255, 255, 90 });
                break;

            case ENEMY_NOCICEPTION: {
                Vector2 up = { e->pos.x, e->pos.y - r };
                Vector2 dl = { e->pos.x - r, e->pos.y + r * 0.7f };
                Vector2 dr = { e->pos.x + r, e->pos.y + r * 0.7f };
                DrawTriangle(up, dl, dr, body);
                DrawCircleV(e->pos, r * 0.35f,
                            (Color){ 255, 240, 240, 180 });
            } break;

            case ENEMY_AWARENESS: {
                DrawCircleV(e->pos, r, body);
                DrawRing(e->pos, r * 0.25f, r * 0.75f, 0.0f, 360.0f, 24,
                         (Color){ 30, 20, 0, 220 });
                DrawCircleV(e->pos, r * 0.22f,
                            (Color){ 255, 255, 255, 240 });
            } break;

            case ENEMY_LARYNGOSPASM: {
                DrawRing(e->pos, r * 0.45f, r, 0.0f, 360.0f, 32, body);
                DrawRing(e->pos, r * 0.45f, r * 0.62f, 0.0f, 360.0f, 32,
                         (Color){ 20, 40, 60, 200 });
            } break;

            case ENEMY_BRONCHOSPASM: {
                DrawCircleV(e->pos, r * 0.7f, body);
                for (int k = 0; k < 5; k++) {
                    float a = k * (2.0f * PI / 5.0f) + e->wobble * 0.5f;
                    DrawLineEx(e->pos,
                               Vector2Add(e->pos,
                                          Vector2Scale(VecFromAngle(a), r)),
                               3.0f, body);
                }
            } break;

            case ENEMY_SEIZURE: {
                DrawCircleV(e->pos, r * 0.85f, body);
                Vector2 pts[4] = {
                    { e->pos.x - r * 0.4f, e->pos.y - r * 0.8f },
                    { e->pos.x + r * 0.25f, e->pos.y - r * 0.1f },
                    { e->pos.x - r * 0.2f,  e->pos.y + r * 0.1f },
                    { e->pos.x + r * 0.4f,  e->pos.y + r * 0.8f },
                };
                for (int k = 0; k < 3; k++) {
                    DrawLineEx(pts[k], pts[k + 1], 4.0f,
                               (Color){ 30, 0, 30, 220 });
                }
            } break;

            case ENEMY_SYMPATHETIC_SURGE: {
                DrawCircleV(e->pos, r, body);
                float pulse = 0.5f + 0.5f * sinf(e->pulse * 2.2f);
                DrawRing(e->pos, r + 3.0f, r + 6.0f + pulse * 8.0f,
                         0.0f, 360.0f, 40,
                         (Color){ 255, 200, 100,
                                  (unsigned char)(120 * (1.0f - pulse)) });
            } break;

            case ENEMY_PONV: {
                DrawCircleV(e->pos, r * 0.9f, body);
                DrawRing(e->pos, r * 0.3f, r * 0.5f, 0.0f, 360.0f, 20,
                         (Color){ 60, 80, 20, 200 });
            } break;

            case ENEMY_EMERGENCE_DELIRIUM: {
                DrawCircleV(e->pos, r, body);
                for (int k = 0; k < 6; k++) {
                    float a = k * (PI / 3.0f) + e->wobble;
                    DrawLineEx(
                        Vector2Add(e->pos, Vector2Scale(VecFromAngle(a), r * 0.7f)),
                        Vector2Add(e->pos, Vector2Scale(VecFromAngle(a), r * 1.35f)),
                        2.5f, body);
                }
            } break;

            default:
                DrawCircleV(e->pos, r, body);
                break;
        }
    }

    if (!def->isBoss && e->hp < e->hpMax) {
        float bw = e->radius * 2.2f;
        float bh = 4.0f;
        float bx = e->pos.x - bw * 0.5f;
        float by = e->pos.y - e->radius - 12.0f;
        DrawRectangle((int)bx, (int)by, (int)bw, (int)bh,
                      (Color){ 20, 22, 26, 220 });
        float frac = Clampf(e->hp / e->hpMax, 0.0f, 1.0f);
        DrawRectangle((int)bx, (int)by, (int)(bw * frac), (int)bh,
                      ColLerp((Color){ 255, 70, 70, 255 },
                              (Color){ 255, 220, 90, 255 }, frac));
    }

    if (def->isBoss) {
        DrawTxtCentered("AWARENESS UNDER PARALYSIS", e->pos.x,
                        e->pos.y - e->radius - 28.0f, 18.0f,
                        (Color){ 255, 90, 90, 255 });
        float bw = 360.0f;
        float bx = e->pos.x - bw * 0.5f;
        float by = e->pos.y - e->radius - 8.0f;
        DrawRectangle((int)bx, (int)by, (int)bw, 9,
                      (Color){ 26, 14, 18, 230 });
        float frac = Clampf(e->hp / e->hpMax, 0.0f, 1.0f);
        DrawRectangle((int)bx, (int)by, (int)(bw * frac), 9,
                      (Color){ 220, 40, 60, 255 });
        DrawRectangleLines((int)bx, (int)by, (int)bw, 9,
                           (Color){ 90, 60, 70, 255 });
    }
}

static void DrawEnemies(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (gEnemies[i].active) DrawEnemyShape(&gEnemies[i]);
    }
}

static void DrawPlayer(void)
{
    Vector2 p = gPlayer.pos;

    DrawEllipse((int)p.x, (int)(p.y + 16.0f), 17.0f, 8.0f,
                (Color){ 0, 0, 0, 90 });

    float flap = gPlayer.whiteCoatFlap;
    DrawCircleV(p, gPlayer.radius, (Color){ 238, 242, 248, 255 });
    DrawCircleLines((int)p.x, (int)p.y, gPlayer.radius,
                    (Color){ 160, 175, 190, 255 });

    Vector2 back = VecFromAngle(gPlayer.aimAngle + PI);
    Vector2 perp = (Vector2){ -back.y, back.x };
    Vector2 t1 = Vector2Add(p, Vector2Scale(back,
                                            gPlayer.radius + 6.0f + flap * 6.0f));
    DrawTriangle(Vector2Add(p, Vector2Scale(perp, 8.0f)),
                 Vector2Subtract(p, Vector2Scale(perp, 8.0f)),
                 t1,
                 (Color){ 220, 228, 238, 210 });

    DrawCircleV(p, 8.0f, (Color){ 226, 190, 160, 255 });

    float bobY = sinf(gPlayer.bob) * 1.4f;

    Vector2 armDir = VecFromAngle(gPlayer.aimAngle);
    Vector2 armPerp = (Vector2){ -armDir.y, armDir.x };
    Vector2 shoulder = Vector2Add(Vector2Add(p, (Vector2){ 0.0f, bobY }),
                                  Vector2Scale(armPerp, 9.0f));
    float recoilPull = gPlayer.recoil * 5.0f;
    Vector2 hand = Vector2Add(
        Vector2Add(shoulder,
                   Vector2Scale(armDir, gPlayer.radius + 14.0f - recoilPull)),
        (Vector2){ 0.0f, bobY });

    DrawLineEx(shoulder, hand, 6.0f, (Color){ 232, 238, 246, 255 });

    {
        const DrugDef *d = &DRUGS[gPlayer.selectedDrug];
        Vector2 tip = Vector2Add(hand, Vector2Scale(armDir, 22.0f));
        Vector2 backEnd = Vector2Subtract(hand, Vector2Scale(armDir, 12.0f));

        Color body = d->color;
        if (gPlayer.syringeGlow > 0.01f) {
            Color glow = body;
            glow.a = (unsigned char)(150 * gPlayer.syringeGlow);
            DrawCircleV(tip, 16.0f * gPlayer.syringeGlow + 6.0f, glow);
        }

        DrawLineEx(backEnd, hand, 8.0f, body);
        DrawLineEx(hand, tip, 2.0f, (Color){ 220, 226, 236, 240 });
        DrawCircleV(tip, 2.6f, (Color){ 255, 255, 255, 240 });
    }

    {
        Vector2 mouse = GetMousePosition();
        float dist = Vector2Distance(p, mouse);
        if (dist > 30.0f) {
            const DrugDef *d = &DRUGS[gPlayer.selectedDrug];
            Color rc = d->color;
            rc.a = 150;
            DrawRing(mouse, 13.0f, 15.0f, 0.0f, 360.0f, 32, rc);
            DrawLineEx((Vector2){ mouse.x - 20.0f, mouse.y },
                       (Vector2){ mouse.x - 8.0f, mouse.y }, 2.0f, rc);
            DrawLineEx((Vector2){ mouse.x + 8.0f, mouse.y },
                       (Vector2){ mouse.x + 20.0f, mouse.y }, 2.0f, rc);
            DrawLineEx((Vector2){ mouse.x, mouse.y - 20.0f },
                       (Vector2){ mouse.x, mouse.y - 8.0f }, 2.0f, rc);
            DrawLineEx((Vector2){ mouse.x, mouse.y + 8.0f },
                       (Vector2){ mouse.x, mouse.y + 20.0f }, 2.0f, rc);
        }
    }
}

/* ============================================================================
 *  SECTION 19 -- SCREENS
 * ==========================================================================*/

static void DrawTitleScreen(void)
{
    ClearBackground((Color){ 10, 14, 20, 255 });

    for (int y = 0; y < SCREEN_H; y += 40)
        DrawLine(0, y, SCREEN_W, y, (Color){ 18, 24, 32, 255 });
    for (int x = 0; x < SCREEN_W; x += 40)
        DrawLine(x, 0, x, SCREEN_H, (Color){ 18, 24, 32, 255 });

    DrawTxtCentered("BALANCED ANESTHESIA",
                    (float)SCREEN_W * 0.5f, 108.0f, 68.0f,
                    (Color){ 240, 246, 252, 255 });
    DrawTxtCentered("INTRAVENOUS NON-OPIOID ANESTHETICS -- AREA DEFENSE",
                    (float)SCREEN_W * 0.5f, 186.0f, 22.0f,
                    (Color){ 130, 165, 200, 255 });

    DrawLineEx((Vector2){ (float)SCREEN_W * 0.5f - 340.0f, 224.0f },
               (Vector2){ (float)SCREEN_W * 0.5f + 340.0f, 224.0f }, 2.0f,
               (Color){ 60, 90, 120, 255 });

    const char *lines[] = {
        "Noxious stimuli advance on the patient in the centre of the room.",
        "Fire syringes of intravenous anesthetic to suppress them.",
        "",
        "EVERY syringe you give enters the patient, hit or miss.",
        "Too little  ->  awareness, pain, sympathetic surge, injury.",
        "Too much    ->  vasodilation, apnoea, bradycardia, injury.",
        "",
        "Match the drug to the stimulus. Hypnotics do not treat pain.",
        "Methohexital and etomidate make seizure foci worse.",
        "Ketamine is the bronchodilator and the analgesic.",
        "Propofol is the antiemetic and the airway-reflex suppressant.",
    };
    int nLines = (int)(sizeof(lines) / sizeof(lines[0]));
    for (int i = 0; i < nLines; i++) {
        DrawTxtCentered(lines[i], (float)SCREEN_W * 0.5f,
                        258.0f + i * 28.0f, 20.0f,
                        (Color){ 180, 196, 212, 255 });
    }

    float pulse = 0.5f + 0.5f * sinf(GetTime() * 3.0f);
    Color c = { 120, 230, 160, (unsigned char)(160 + 95 * pulse) };
    DrawTxtCentered("PRESS  ENTER  TO BEGIN",
                    (float)SCREEN_W * 0.5f, 620.0f, 30.0f, c);

    DrawTxtCentered("WASD move    MOUSE aim    LMB fire    1-9 select drug    0 flumazenil",
                    (float)SCREEN_W * 0.5f, 700.0f, 18.0f,
                    (Color){ 120, 140, 160, 255 });
    DrawTxtCentered("TAB pharmacology reference    P pause    R restart",
                    (float)SCREEN_W * 0.5f, 726.0f, 18.0f,
                    (Color){ 120, 140, 160, 255 });
    DrawTxtCentered("Q / E or mouse wheel cycle drugs",
                    (float)SCREEN_W * 0.5f, 752.0f, 18.0f,
                    (Color){ 120, 140, 160, 255 });

    DrawTxtCentered("Based on Chapter 8, Intravenous Anesthetics -- Bokoch & Eilers",
                    (float)SCREEN_W * 0.5f, (float)SCREEN_H - 44.0f, 16.0f,
                    (Color){ 80, 96, 112, 255 });
}

static void DrawCodexScreen(void)
{
    ClearBackground((Color){ 10, 14, 20, 255 });

    DrawTxt("PHARMACOLOGY REFERENCE -- INTRAVENOUS ANESTHETICS",
            40.0f, 26.0f, 28.0f, (Color){ 235, 242, 250, 255 });
    DrawTxt("ESC or TAB to return.  Scroll with the mouse wheel.",
            40.0f, 60.0f, 16.0f, (Color){ 130, 150, 170, 255 });

    float y = 104.0f - gCodexScroll;
    float x = 40.0f;
    float colW = (float)SCREEN_W - 80.0f;

    for (int i = 0; i < DRUG_COUNT; i++) {
        const DrugDef *d = &DRUGS[i];
        float boxH = 152.0f;

        if (y + boxH < 0.0f || y > (float)SCREEN_H) {
            y += boxH + 12.0f;
            continue;
        }

        Rectangle r = { x, y, colW, boxH };
        DrawRectangleRounded(r, 0.03f, 5, (Color){ 16, 22, 30, 235 });
        DrawRectangleRoundedLines(r, 0.03f, 5, (Color){ 46, 60, 74, 255 });

        DrawRectangle((int)r.x + 6, (int)r.y + 6, 6, (int)r.height - 12,
                      d->color);

        DrawTxt(d->name, r.x + 24.0f, r.y + 12.0f, 24.0f, d->color);
        DrawTxt(d->className, r.x + 24.0f, r.y + 40.0f, 15.0f,
                (Color){ 145, 162, 180, 255 });

        char buf[256];

        std::snprintf(buf, sizeof(buf),
                      "HYPNOTIC %.2f    ANALGESIC %.2f    ANXIOLYTIC %.2f    "
                      "ANTICONVULSANT %.2f    AMNESIA %.2f",
                      d->hypnotic, d->analgesic, d->anxiolytic,
                      d->anticonvulsant, d->amnesia);
        DrawTxt(buf, r.x + 24.0f, r.y + 62.0f, 13.0f,
                (Color){ 190, 205, 220, 255 });

        std::snprintf(buf, sizeof(buf),
                      "MAP %+.0f mmHg    HR %+.0f bpm    Respiratory depression %.2f    "
                      "Elimination t1/2 ~%.0f min    ke %.3f /s    ke0 %.2f /s    CSHT growth %.2f",
                      d->mapDelta, d->hrDelta, d->rrDelta, d->durationMin,
                      d->ke, d->ke0, d->cshtGrowth);
        DrawTxt(buf, r.x + 24.0f, r.y + 80.0f, 12.0f,
                (Color){ 160, 178, 196, 255 });

        {
            float fx = r.x + 24.0f;
            float fy = r.y + 100.0f;
            struct FlagRow { bool on; const char *label; Color c; };
            FlagRow flags[8] = {
                { d->isBenzodiazepine, "BENZODIAZEPINE",        (Color){ 150, 220, 255, 255 } },
                { d->antiemetic,       "ANTIEMETIC",            (Color){ 190, 230, 120, 255 } },
                { d->bronchodilator,   "BRONCHODILATOR",        (Color){ 150, 255, 160, 255 } },
                { d->raisesICP,        "RAISES ICP",            (Color){ 255, 140, 255, 255 } },
                { d->adrenalSuppress,  "ADRENAL SUPPRESS",      (Color){ 255, 170, 90, 255 } },
                { d->injectionPain,    "PAIN ON INJECTION",     (Color){ 255, 120, 120, 255 } },
                { d->reversible,       "FLUMAZENIL-REVERSIBLE", (Color){ 255, 255, 255, 255 } },
                { d->waterSoluble,     "WATER SOLUBLE",         (Color){ 140, 200, 255, 255 } },
            };
            for (int f = 0; f < 8; f++) {
                if (!flags[f].on) continue;
                Vector2 m = MeasureTextEx(GetFontDefault(),
                                          flags[f].label, 11.0f, 0.9f);
                if (fx + m.x + 12.0f > r.x + colW - 12.0f) {
                    fx = r.x + 24.0f;
                    fy += 16.0f;
                }
                DrawRectangleRounded(
                    (Rectangle){ fx, fy, m.x + 10.0f, 14.0f },
                    0.35f, 4, (Color){ 28, 36, 46, 255 });
                DrawRectangleRoundedLines(
                    (Rectangle){ fx, fy, m.x + 10.0f, 14.0f },
                    0.35f, 4, flags[f].c);
                DrawTxt(flags[f].label, fx + 5.0f, fy + 2.0f, 11.0f,
                        flags[f].c);
                fx += m.x + 18.0f;
            }
        }

        {
            const char *src = d->note;
            char line[128];
            int li = 0;
            float ly = r.y + 124.0f;
            for (const char *p = src; ; p++) {
                if (*p == '\0' || li >= 120) {
                    line[li] = '\0';
                    DrawTxt(line, r.x + 24.0f, ly, 12.0f,
                            (Color){ 175, 190, 205, 255 });
                    ly += 14.0f;
                    li = 0;
                    if (*p == '\0') break;
                } else {
                    line[li++] = *p;
                }
            }
        }

        y += boxH + 12.0f;
    }

    float maxScroll = y + gCodexScroll - (float)SCREEN_H + 60.0f;
    if (maxScroll < 0.0f) maxScroll = 0.0f;
    if (gCodexScroll > maxScroll) gCodexScroll = maxScroll;
    if (gCodexScroll < 0.0f) gCodexScroll = 0.0f;

    DrawTxtRight("ESC / TAB", (float)SCREEN_W - 40.0f,
                 (float)SCREEN_H - 34.0f, 16.0f,
                 (Color){ 120, 140, 160, 255 });
}

static void DrawEndScreen(bool victory)
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 4, 6, 10, 225 });

    if (victory) {
        DrawTxtCentered("CASE COMPLETE", (float)SCREEN_W * 0.5f, 90.0f, 62.0f,
                        (Color){ 120, 240, 160, 255 });
        DrawTxtCentered("The patient emerges smoothly, pain free, with no recall.",
                        (float)SCREEN_W * 0.5f, 168.0f, 22.0f,
                        (Color){ 190, 210, 230, 255 });
    } else {
        DrawTxtCentered("PATIENT COMPROMISED",
                        (float)SCREEN_W * 0.5f, 90.0f, 62.0f,
                        (Color){ 255, 80, 80, 255 });
        DrawTxtCentered("The physiological reserve was exhausted.",
                        (float)SCREEN_W * 0.5f, 168.0f, 22.0f,
                        (Color){ 210, 190, 190, 255 });
    }

    Rectangle r = { (float)SCREEN_W * 0.5f - 480.0f, 232.0f, 960.0f, 430.0f };
    DrawRectangleRounded(r, 0.03f, 6, (Color){ 14, 18, 26, 240 });
    DrawRectangleRoundedLines(r, 0.03f, 6, (Color){ 60, 78, 96, 255 });

    DrawTxt("CASE DEBRIEF", r.x + 24.0f, r.y + 16.0f, 26.0f,
            (Color){ 235, 242, 250, 255 });

    char buf[192];
    float y = r.y + 62.0f;
    float lx = r.x + 30.0f;
    float rx = r.x + r.width * 0.5f + 20.0f;

    int wavesSurvived = gCurrentWave + (victory ? 1 : 0);
    if (wavesSurvived > WAVE_COUNT) wavesSurvived = WAVE_COUNT;
    if (wavesSurvived < 0) wavesSurvived = 0;

    std::snprintf(buf, sizeof(buf), "Waves survived      %d / %d",
                  wavesSurvived, WAVE_COUNT);
    DrawTxt(buf, lx, y, 19.0f, (Color){ 200, 214, 228, 255 });
    y += 30.0f;

    std::snprintf(buf, sizeof(buf), "Stimuli suppressed  %d", gKills);
    DrawTxt(buf, lx, y, 19.0f, (Color){ 200, 214, 228, 255 });
    y += 30.0f;

    std::snprintf(buf, sizeof(buf), "Syringes given     %d", gPlayer.shotsFired);
    DrawTxt(buf, lx, y, 19.0f, (Color){ 200, 214, 228, 255 });
    y += 30.0f;

    float acc = (gPlayer.shotsFired > 0)
              ? 100.0f * (float)gPlayer.hits / (float)gPlayer.shotsFired
              : 0.0f;
    std::snprintf(buf, sizeof(buf), "Hit accuracy       %.1f%%", acc);
    DrawTxt(buf, lx, y, 19.0f, (Color){ 200, 214, 228, 255 });
    y += 30.0f;

    std::snprintf(buf, sizeof(buf), "Total drug given   %.1f units",
                  gStatTotalDrug);
    DrawTxt(buf, lx, y, 19.0f, (Color){ 200, 214, 228, 255 });
    y += 30.0f;

    std::snprintf(buf, sizeof(buf), "Final viability    %d%%",
                  (int)roundf(gPatient.health));
    Color hc = (gPatient.health > 50.0f) ? (Color){ 120, 240, 160, 255 }
                                         : (Color){ 255, 90, 90, 255 };
    DrawTxt(buf, lx, y, 19.0f, hc);
    y += 40.0f;

    float ry = r.y + 62.0f;
    DrawTxt("TIME IN PHYSIOLOGICAL FAILURE MODE", rx, ry, 17.0f,
            (Color){ 235, 242, 250, 255 });
    ry += 32.0f;

    std::snprintf(buf, sizeof(buf), "Hypotension (MAP < 55)      %.1f s",
                  gStatHypotensionTime);
    DrawTxt(buf, rx, ry, 17.0f, (Color){ 255, 140, 140, 255 });
    ry += 28.0f;

    std::snprintf(buf, sizeof(buf), "Hypoxaemia (SpO2 < 90)      %.1f s",
                  gStatHypoxiaTime);
    DrawTxt(buf, rx, ry, 17.0f, (Color){ 120, 190, 255, 255 });
    ry += 28.0f;

    std::snprintf(buf, sizeof(buf), "Awareness risk (BIS > 72)   %.1f s",
                  gStatAwarenessTime);
    DrawTxt(buf, rx, ry, 17.0f, (Color){ 255, 225, 90, 255 });
    ry += 34.0f;

    DrawTxt("DRUG EXPOSURE", rx, ry, 17.0f, (Color){ 235, 242, 250, 255 });
    ry += 28.0f;

    std::snprintf(buf, sizeof(buf), "Propofol        %.1f", gStatPropofol);
    DrawTxt(buf, rx, ry, 16.0f, DRUGS[DRUG_PROPOFOL].color);
    ry += 22.0f;

    std::snprintf(buf, sizeof(buf), "Ketamine        %.1f", gStatKetamine);
    DrawTxt(buf, rx, ry, 16.0f, DRUGS[DRUG_KETAMINE].color);
    ry += 22.0f;

    std::snprintf(buf, sizeof(buf), "Etomidate       %.1f", gStatEtomidate);
    DrawTxt(buf, rx, ry, 16.0f, DRUGS[DRUG_ETOMIDATE].color);
    ry += 22.0f;

    std::snprintf(buf, sizeof(buf), "Adrenal reserve %.0f%%",
                  gPatient.adrenalReserve * 100.0f);
    Color ac = (gPatient.adrenalReserve > 0.6f)
             ? (Color){ 120, 240, 160, 255 }
             : (Color){ 255, 170, 90, 255 };
    DrawTxt(buf, rx, ry, 16.0f, ac);
    ry += 30.0f;

    {
        const char *summary;
        if (victory) {
            summary = "You maintained the therapeutic window: enough hypnosis "
                      "to prevent awareness, enough analgesia to suppress the "
                      "surgical stress response, and not so much of either "
                      "that the patient could not tolerate the vasodilation "
                      "and respiratory depression that accompany these drugs.";
        } else {
            summary = "Remember the two failure modes. Under-dosing produces "
                      "awareness, tachycardia and hypertension. Over-dosing "
                      "produces vasodilation, myocardial depression, apnoea "
                      "and bradycardia. Balanced anaesthesia means small doses "
                      "of several drugs with complementary profiles.";
        }
        char line[128];
        int li = 0;
        float ly = r.y + r.height - 74.0f;
        for (const char *p = summary; ; p++) {
            if (*p == '\0' || li >= 120) {
                line[li] = '\0';
                DrawTxt(line, r.x + 30.0f, ly, 15.0f,
                        (Color){ 170, 188, 206, 255 });
                ly += 18.0f;
                li = 0;
                if (*p == '\0') break;
            } else {
                line[li++] = *p;
            }
        }
    }

    float pulse = 0.5f + 0.5f * sinf(GetTime() * 3.0f);
    Color c = { 200, 220, 240, (unsigned char)(150 + 100 * pulse) };
    DrawTxtCentered("PRESS  R  TO RUN ANOTHER CASE",
                    (float)SCREEN_W * 0.5f, (float)SCREEN_H - 96.0f, 26.0f, c);
    DrawTxtCentered("ESC returns to the title screen",
                    (float)SCREEN_W * 0.5f, (float)SCREEN_H - 58.0f, 16.0f,
                    (Color){ 130, 150, 170, 255 });
}

static void DrawGameplayOverlay(void)
{
    if (!gShowHelp) return;

    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 4, 8, 14, 220 });

    DrawTxtCentered("PHARMACOLOGY QUICK REFERENCE",
                    (float)SCREEN_W * 0.5f, 60.0f, 36.0f,
                    (Color){ 240, 246, 252, 255 });

    const char *rows[][2] = {
        { "Propofol",        "Hypnotic. Biggest MAP fall. Apnoea. Antiemetic. No analgesia." },
        { "Fospropofol",     "Prodrug. Slow onset. Less hypotension. Perineal burning." },
        { "Thiopental",      "Hypnotic. Lowers ICP/CBF/CMRO2. Long context-sensitive t1/2." },
        { "Methohexital",    "Like thiopental but PROCONVULSANT. Do not use on seizures." },
        { "Midazolam",       "Anxiolysis, amnesia, anticonvulsant. Ceiling on hypnosis." },
        { "Diazepam",        "Long acting. Active metabolites. Pain on injection." },
        { "Ketamine",        "ANALGESIC. Raises HR/MAP. Bronchodilator. Raises ICP." },
        { "Etomidate",       "Haemodynamically flat. Adrenal suppression. Proconvulsant." },
        { "Dexmedetomidine", "Sedation + analgesia. Bradycardia. No respiratory depression." },
        { "Flumazenil",      "Reverses benzodiazepines. Short acting; resedation possible." },
    };

    float y = 130.0f;
    for (int i = 0; i < 10; i++) {
        DrawTxt(rows[i][0], 240.0f, y, 20.0f, (Color){ 190, 220, 250, 255 });
        DrawTxt(rows[i][1], 520.0f, y, 19.0f, (Color){ 190, 200, 212, 255 });
        y += 34.0f;
    }

    y += 14.0f;
    DrawTxtCentered("Noxious stimuli and their specific antagonists",
                    (float)SCREEN_W * 0.5f, y, 24.0f,
                    (Color){ 240, 246, 252, 255 });
    y += 36.0f;

    const char *rows2[][2] = {
        { "Anxiety",                "Benzodiazepines (anxiolysis + anterograde amnesia)." },
        { "Nociception",            "Ketamine or dexmedetomidine. Hypnotics will not do." },
        { "Awareness",              "Deep hypnosis. Analgesia does nothing." },
        { "Laryngospasm",           "Propofol suppresses upper airway reflexes best." },
        { "Bronchospasm",           "Ketamine relaxes bronchial smooth muscle." },
        { "Seizure Focus",          "Benzodiazepine, propofol, thiopental. NOT methohexital." },
        { "Sympathetic Surge",      "Deep anaesthesia. Dexmedetomidine. NOT ketamine." },
        { "PONV",                   "Propofol (antiemetic). Etomidate makes it worse." },
        { "Emergence Delirium",     "Dexmedetomidine." },
        { "Awareness Under Paralysis", "Hypnosis AND analgesia, simultaneously." },
    };

    for (int i = 0; i < 10; i++) {
        DrawTxt(rows2[i][0], 240.0f, y, 19.0f, (Color){ 255, 200, 200, 255 });
        DrawTxt(rows2[i][1], 620.0f, y, 18.0f, (Color){ 190, 200, 212, 255 });
        y += 30.0f;
    }

    DrawTxtCentered("TAB to close",
                    (float)SCREEN_W * 0.5f, (float)SCREEN_H - 50.0f, 20.0f,
                    (Color){ 160, 180, 200, 255 });
}

/* ============================================================================
 *  SECTION 20 -- MAIN UPDATE / DRAW
 * ==========================================================================*/

static void ResetGame(void)
{
    PatientInit();
    PlayerInit();
    ProjectilesClear();
    EnemiesClear();
    ParticlesClear();
    FloatersClear();

    gCurrentWave = 0;
    gGameTime    = 0.0f;
    gScore       = 0.0f;
    gKills       = 0;
    gShake       = 0.0f;
    gFlashRed    = 0.0f;
    gFlashWhite  = 0.0f;
    gNextEnemyId = 0;
    gCodexScroll = 0.0f;

    WaveBegin(0);
}

static void UpdateGameplay(float dt)
{
    gGameTime += dt;

    UpdatePlayer(dt);
    UpdateProjectiles(dt);
    UpdateEnemies(dt);
    UpdatePatient(dt);
    UpdateParticles(dt);
    UpdateFloaters(dt);
    UpdateWaveDirector(dt);

    gShake      = Lerpf(gShake, 0.0f, 5.0f * dt);
    gFlashRed   = Lerpf(gFlashRed, 0.0f, 4.0f * dt);
    gFlashWhite = Lerpf(gFlashWhite, 0.0f, 6.0f * dt);

    if (!gPatient.alive) gState = STATE_GAMEOVER;
}

static void DrawGameplay(void)
{
    Vector2 shake = { 0.0f, 0.0f };
    if (gShake > 0.1f) {
        shake.x = Randf(-gShake, gShake);
        shake.y = Randf(-gShake, gShake);
    }

    Camera2D cam = { { shake.x, shake.y }, { 0.0f, 0.0f }, 0.0f, 1.0f };
    BeginMode2D(cam);

    DrawOperatingRoomFloor();
    DrawPatient();
    DrawEnemies();
    DrawProjectiles();
    DrawParticles();
    DrawPlayer();
    DrawFloaters();

    EndMode2D();

    DrawAlarms();
    DrawMonitorPanel();
    DrawWaveBanner();
    DrawDrugBar();

    if (gFlashRed > 0.01f) {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H,
                      (Color){ 180, 0, 0,
                               (unsigned char)(80 * gFlashRed) });
    }
    if (gFlashWhite > 0.01f) {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H,
                      (Color){ 255, 255, 255,
                               (unsigned char)(120 * gFlashWhite) });
    }

    {
        float danger = 0.0f;
        if (gPatient.health < 45.0f)
            danger = (45.0f - gPatient.health) / 45.0f;
        if (gPatient.spo2 < 90.0f)
            danger = fmaxf(danger, (90.0f - gPatient.spo2) / 40.0f);
        if (gPatient.map < 55.0f)
            danger = fmaxf(danger, (55.0f - gPatient.map) / 40.0f);
        danger = Clampf(danger, 0.0f, 1.0f);
        if (danger > 0.02f) {
            float pulse = 0.5f + 0.5f * sinf(gGameTime * 5.0f);
            unsigned char a =
                (unsigned char)(70 * danger * (0.55f + 0.45f * pulse));
            DrawRectangleGradientV(0, 0, SCREEN_W, 120,
                                   (Color){ 120, 0, 0, a },
                                   (Color){ 120, 0, 0, 0 });
            DrawRectangleGradientV(0, SCREEN_H - 120, SCREEN_W, 120,
                                   (Color){ 120, 0, 0, 0 },
                                   (Color){ 120, 0, 0, a });
        }
    }

    DrawGameplayOverlay();

    if (gState == STATE_PAUSED) {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 0, 0, 0, 170 });
        DrawTxtCentered("PAUSED", (float)SCREEN_W * 0.5f,
                        (float)SCREEN_H * 0.5f - 40.0f, 60.0f,
                        (Color){ 240, 246, 252, 255 });
        DrawTxtCentered("P to resume   TAB for pharmacology   R to restart   ESC to quit",
                        (float)SCREEN_W * 0.5f,
                        (float)SCREEN_H * 0.5f + 40.0f, 20.0f,
                        (Color){ 170, 190, 210, 255 });
    }
}

/* ============================================================================
 *  SECTION 21 -- MAIN
 * ==========================================================================*/

int main(void)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H,
               "Balanced Anesthesia -- Intravenous Non-Opioid Anesthetics");
    SetTargetFPS(TARGET_FPS);

    SetRandomSeed((unsigned int)(GetTime() * 7919.0) + 13u);

    PatientInit();
    PlayerInit();
    ProjectilesClear();
    EnemiesClear();
    ParticlesClear();
    FloatersClear();

    while (!WindowShouldClose()) {

        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        switch (gState) {

            case STATE_MENU:
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                    ResetGame();
                }
                if (IsKeyPressed(KEY_TAB)) {
                    gState = STATE_CODEX;
                    gCodexScroll = 0.0f;
                }
                break;

            case STATE_CODEX:
                if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE)) {
                    gState = STATE_MENU;
                }
                gCodexScroll -= GetMouseWheelMove() * 60.0f;
                if (IsKeyDown(KEY_DOWN)) gCodexScroll += 500.0f * dt;
                if (IsKeyDown(KEY_UP))   gCodexScroll -= 500.0f * dt;
                if (gCodexScroll < 0.0f) gCodexScroll = 0.0f;
                break;

            case STATE_WAVE_INTRO:
            case STATE_PLAYING:
                UpdateGameplay(dt);
                if (IsKeyPressed(KEY_P))     gState = STATE_PAUSED;
                if (IsKeyPressed(KEY_TAB))   gShowHelp = !gShowHelp;
                if (IsKeyPressed(KEY_R))     ResetGame();
                if (IsKeyPressed(KEY_ESCAPE)) gState = STATE_MENU;
                break;

            case STATE_PAUSED:
                if (IsKeyPressed(KEY_P))     gState = STATE_PLAYING;
                if (IsKeyPressed(KEY_TAB))   gShowHelp = !gShowHelp;
                if (IsKeyPressed(KEY_R))     ResetGame();
                if (IsKeyPressed(KEY_ESCAPE)) gState = STATE_MENU;
                break;

            case STATE_GAMEOVER:
            case STATE_VICTORY:
                if (IsKeyPressed(KEY_R))  ResetGame();
                if (IsKeyPressed(KEY_ESCAPE)) gState = STATE_MENU;
                if (IsKeyPressed(KEY_TAB)) {
                    gState = STATE_CODEX;
                    gCodexScroll = 0.0f;
                }
                break;

            default:
                break;
        }

        BeginDrawing();

        switch (gState) {
            case STATE_MENU:
                DrawTitleScreen();
                break;

            case STATE_CODEX:
                DrawCodexScreen();
                break;

            case STATE_WAVE_INTRO:
            case STATE_PLAYING:
            case STATE_PAUSED:
                DrawGameplay();
                break;

            case STATE_GAMEOVER:
                DrawGameplay();
                DrawEndScreen(false);
                break;

            case STATE_VICTORY:
                DrawGameplay();
                DrawEndScreen(true);
                break;

            default:
                break;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}