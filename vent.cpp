/* ============================================================================
   VENTILATOR SIMULATOR
   A raylib (C++) game based on "BASIC PRINCIPLES OF MECHANICAL VENTILATION"
   ----------------------------------------------------------------------------
   You are the intensivist. Adjust the ventilator to keep the patient's
   gas exchange, pressures and haemodynamics inside safe ranges.

   Controls:  MOUSE  - drag sliders
              UP/DOWN or TAB - select a control
              LEFT/RIGHT     - fine adjust
              ENTER          - restart / back to menu
   ============================================================================ */

#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

using std::vector;
using std::string;

// ---------------------------------------------------------------- constants
static const int SW = 1280;
static const int SH = 720;

static const Color C_BG     = { 12, 14, 22,255};
static const Color C_PANEL  = { 22, 26, 38,255};
static const Color C_PANEL2 = { 30, 36, 52,255};
static const Color C_LINE   = { 48, 58, 80,255};
static const Color C_TEXT   = {200,212,230,255};
static const Color C_DIM    = {116,130,156,255};
static const Color C_ACCENT = {  0,190,255,255};
static const Color C_GOOD   = { 70,225,130,255};
static const Color C_BAD    = {255, 85, 85,255};
static const Color C_WARN   = {255,190, 60,255};

// ---------------------------------------------------------------- layout
static const Rectangle R_MON  = {  12.f, 58.f, 300.f, 650.f};
static const Rectangle R_LUNG = { 324.f, 58.f, 560.f, 400.f};
static const Rectangle R_WAVE = { 324.f,470.f, 560.f, 238.f};
static const Rectangle R_CTRL = { 896.f, 58.f, 372.f, 650.f};

static float Clampf(float v, float a, float b){ return v<a?a:(v>b?b:v); }

// ============================================================================
//  PHYSIOLOGY MODEL
// ============================================================================
struct Scenario {
    const char* name;
    const char* blurb;
    float compliance;     // L / cmH2O
    float resistance;     // cmH2O / (L/s)
    float baseShunt;      // fraction of cardiac output shunted
    float baseDeadSpace;  // L
    float vco2;           // mL/min CO2 production
    Color tint;
};

static Scenario SCEN[4] = {
    { "HEALTHY POST-OP",
      "Normal lungs. Easy - just don't over-ventilate.",
      0.050f,  5.0f, 0.05f, 0.130f, 200.f, { 70,225,130,255} },

    { "ARDS",
      "Stiff lungs + big shunt. Recruit with PEEP, but keep plateau < 30.",
      0.025f,  8.0f, 0.40f, 0.160f, 220.f, {255,150, 60,255} },

    { "ACUTE ASTHMA",
      "High airway resistance. Watch expiratory time or gas will trap.",
      0.040f, 22.0f, 0.08f, 0.160f, 230.f, { 90,180,255,255} },

    { "COPD",
      "Flabby airways, air trapping, chronic CO2 retainer.",
      0.045f, 16.0f, 0.15f, 0.200f, 210.f, {190,130,255,255} },
};
static const int NSCEN = 4;

struct Settings {
    float fio2        = 0.21f;   // fraction
    float peep        = 5.0f;    // cmH2O
    float tidalVolume = 450.f;   // mL
    float rr          = 14.f;    // breaths / min
    float inspFrac    = 0.33f;   // fraction of cycle spent in inspiration
};

struct Derived {
    float peak=0, plateau=0, autoPeep=0, totalPeep=0, mean=0;
    float minuteVent=0, alveolVent=0;
    float paO2=0, paCO2=0, pH=0, spo2=0, map=0;
    float airwayResist=0;
};

// ---- the whole respiratory physiology in one function ----------------------
static Derived Compute(const Settings& s, const Scenario& sc)
{
    Derived d;
    float vt    = s.tidalVolume / 1000.0f;      // L
    float cycle = 60.0f / s.rr;                 // s
    float inspT = cycle * s.inspFrac;
    float expT  = cycle - inspT;
    if (inspT < 0.05f) inspT = 0.05f;
    if (expT  < 0.05f) expT  = 0.05f;

    // ---- gas trapping / auto-PEEP -----------------------------------------
    float tau   = sc.resistance * sc.compliance;   // time constant (s)
    float empty = 3.0f * tau;                      // time needed to exhale
    float autoP = 0.0f;
    if (expT < empty)
        autoP = ((empty - expT) / empty) * (vt / sc.compliance) * 0.5f;
    autoP = Clampf(autoP, 0.0f, 25.0f);
    d.autoPeep  = autoP;
    d.totalPeep = s.peep + autoP;

    // ---- pressures ---------------------------------------------------------
    d.plateau     = d.totalPeep + vt / sc.compliance;
    float flow    = vt / inspT;                    // L/s
    d.airwayResist= sc.resistance;
    d.peak        = d.plateau + flow * sc.resistance;
    d.mean        = s.peep + 0.5f * (d.plateau - s.peep);

    // ---- ventilation & CO2 -------------------------------------------------
    d.minuteVent = vt * s.rr;                                   // L/min
    float ds     = sc.baseDeadSpace + 0.004f * s.peep;          // PEEP adds dead space
    float va     = (vt - ds) * s.rr;                            // L/min
    va = std::max(va, 0.05f);
    d.alveolVent = va;
    d.paCO2 = Clampf(0.863f * sc.vco2 / va, 5.0f, 200.0f);

    // ---- oxygenation -------------------------------------------------------
    float PAO2 = s.fio2 * 713.0f - d.paCO2 / 0.8f;
    PAO2 = std::max(PAO2, 5.0f);
    float recruit = 1.0f - expf(-s.peep / 8.0f);     // PEEP recruits alveoli
    float shunt   = sc.baseShunt * (1.0f - 0.7f * recruit);
    shunt = Clampf(shunt, 0.02f, 0.90f);
    d.paO2 = PAO2 * (1.0f - shunt);

    // ---- haemoglobin saturation (Hill curve) -------------------------------
    float p = powf(std::max(d.paO2,0.1f), 2.6f);
    d.spo2 = 100.0f * p / (p + powf(26.0f, 2.6f));

    // ---- acid-base ---------------------------------------------------------
    d.pH = 6.1f + log10f(24.0f / (0.03f * d.paCO2));

    // ---- haemodynamics -----------------------------------------------------
    d.map = 92.0f - std::max(0.0f, s.peep - 10.0f) * 3.5f - autoP * 2.0f;

    return d;
}

// ============================================================================
//  GAME STATE
// ============================================================================
enum GameState { STATE_MENU, STATE_PLAY, STATE_END };

struct Slider {
    string label;
    float* val;
    float lo, hi;
    Rectangle track;
    bool drag;
    float mul;
    const char* unit;
    float step;
};

struct Particle {
    Vector2 pos, vel;
    float life, maxLife, r;
    Color col;
};

struct Game {
    GameState state = STATE_MENU;
    int   scen      = 0;
    float health    = 100.f;
    float t         = 0.f;
    float duration  = 120.f;
    float stable    = 0.f;
    float phase     = 0.f;
    float grace     = 4.f;
    bool  won       = false;

    Settings set;
    Derived  d;

    vector<Slider>   sliders;
    int              sel = 0;

    vector<Particle> parts;
    vector<float>    trace;
    float sampleAcc = 0.f;
    float spawnAcc  = 0.f;
    float dmgNow    = 0.f;
};

// --------------------------------------------------------------- slider setup
static void SetupSliders(Game& g)
{
    g.sliders.clear();
    float x = R_CTRL.x + 22.f;
    float w = R_CTRL.width - 44.f;
    float y = R_CTRL.y + 78.f;
    const float gap = 84.f;

    g.sliders.push_back({"FiO2",             &g.set.fio2,        0.21f, 1.00f, {x,y,w,14}, false, 100.f, "%",       0.01f}); y += gap;
    g.sliders.push_back({"PEEP",             &g.set.peep,        0.00f,20.00f, {x,y,w,14}, false,   1.f, " cmH2O",  0.50f}); y += gap;
    g.sliders.push_back({"Tidal Volume",     &g.set.tidalVolume,200.0f,800.0f, {x,y,w,14}, false,   1.f, " mL",    10.00f}); y += gap;
    g.sliders.push_back({"Respiratory Rate", &g.set.rr,          6.00f,35.00f, {x,y,w,14}, false,   1.f, " /min",   1.00f}); y += gap;
    g.sliders.push_back({"Inspiratory Time", &g.set.inspFrac,    0.20f, 0.60f, {x,y,w,14}, false, 100.f, "% cycle", 0.01f});
}

// ---------------------------------------------------------------- start over
static void StartScenario(Game& g, int idx)
{
    g.scen    = idx;
    g.state   = STATE_PLAY;
    g.health  = 100.f;
    g.t       = 0.f;
    g.stable  = 0.f;
    g.phase   = 0.f;
    g.grace   = 4.f;
    g.won     = false;
    g.dmgNow  = 0.f;
    g.sel     = 0;
    g.set     = Settings{};
    g.parts.clear();
    g.trace.clear();
    g.d = Compute(g.set, SCEN[g.scen]);
}

// ------------------------------------------------------------- instantaneous P
static float InstantPressure(float t, float inspT, float expT,
                             float peak, float plateau, float peep)
{
    if (t < inspT) {
        float u = t / inspT;
        float k = (1.0f - expf(-6.0f*u)) / (1.0f - expf(-6.0f));
        return peep + (peak - peep) * k;
    } else {
        float u = (t - inspT) / expT;
        return peep + (plateau - peep) * expf(-5.0f * u);
    }
}

// ============================================================================
//  UPDATE
// ============================================================================
static void UpdateGame(Game& g, float dt)
{
    Vector2 mouse = GetMousePosition();

    // ------------------------------------------------------------- main menu
    if (g.state == STATE_MENU) {
        for (int i = 0; i < NSCEN; i++) {
            float x = 90.f + (i % 2) * 580.f;
            float y = 200.f + (i / 2) * 210.f;
            Rectangle r = {x, y, 520.f, 180.f};
            if (CheckCollisionPointRec(mouse, r) &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                StartScenario(g, i);
        }
        return;
    }

    // --------------------------------------------------------------- endgame
    if (g.state == STATE_END) {
        if (IsKeyPressed(KEY_ENTER) ||
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            g.state = STATE_MENU;
        return;
    }

    // ================================================================ PLAYING
    g.t     += dt;
    g.phase += dt;
    if (g.grace > 0.f) g.grace -= dt;

    // ---- slider input ------------------------------------------------------
    for (size_t i = 0; i < g.sliders.size(); i++) {
        Slider& s = g.sliders[i];
        Rectangle hit = {s.track.x, s.track.y - 12.f,
                         s.track.width, s.track.height + 24.f};

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(mouse, hit)) {
            s.drag = true;
            g.sel  = (int)i;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) s.drag = false;

        if (s.drag) {
            float u = Clampf((mouse.x - s.track.x) / s.track.width, 0.f, 1.f);
            *s.val = s.lo + u * (s.hi - s.lo);
        }
    }

    // ---- keyboard ----------------------------------------------------------
    int n = (int)g.sliders.size();
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_TAB)) g.sel = (g.sel + 1) % n;
    if (IsKeyPressed(KEY_UP))                            g.sel = (g.sel + n - 1) % n;

    Slider& cur = g.sliders[g.sel];
    if (IsKeyDown(KEY_RIGHT))
        *cur.val = Clampf(*cur.val + cur.step * dt * 6.f, cur.lo, cur.hi);
    if (IsKeyDown(KEY_LEFT))
        *cur.val = Clampf(*cur.val - cur.step * dt * 6.f, cur.lo, cur.hi);

    // ---- physiology --------------------------------------------------------
    g.d = Compute(g.set, SCEN[g.scen]);

    // ---- breath cycle ------------------------------------------------------
    float cycle = 60.0f / g.set.rr;
    float inspT = cycle * g.set.inspFrac;
    float expT  = cycle - inspT;

    // ---- damage ------------------------------------------------------------
    float dmg = 0.f;
    if (g.d.paO2 < 60.f)   dmg += (60.f - g.d.paO2) * 0.050f;
    if (g.d.spo2 < 90.f)   dmg += (90.f - g.d.spo2) * 0.100f;
    if (g.d.paCO2 > 50.f)  dmg += (g.d.paCO2 - 50.f) * 0.050f;
    if (g.d.paCO2 < 32.f)  dmg += (32.f - g.d.paCO2) * 0.050f;
    if (g.d.pH < 7.32f)    dmg += (7.32f - g.d.pH) * 40.f;
    if (g.d.pH > 7.48f)    dmg += (g.d.pH - 7.48f) * 40.f;
    if (g.d.plateau > 30.f)dmg += (g.d.plateau - 30.f) * 0.40f;
    if (g.d.peak > 40.f)   dmg += (g.d.peak - 40.f) * 0.15f;
    if (g.d.autoPeep > 4.f)dmg += (g.d.autoPeep - 4.f) * 0.50f;
    if (g.d.map < 65.f)    dmg += (65.f - g.d.map) * 0.15f;
    if (g.set.fio2 > 0.6f) dmg += (g.set.fio2 - 0.6f) * 3.0f;

    g.dmgNow = dmg;

    if (g.grace <= 0.f) {
        g.health -= dmg * dt;
        if (dmg < 0.30f) g.health += 5.0f * dt;
    }
    g.health = Clampf(g.health, 0.f, 100.f);

    // ---- "stable" scoring --------------------------------------------------
    bool ok = g.d.paO2  >= 60.f && g.d.paO2  <= 150.f &&
              g.d.paCO2 >= 35.f && g.d.paCO2 <= 45.f  &&
              g.d.pH    >= 7.35f && g.d.pH   <= 7.45f &&
              g.d.plateau < 30.f && g.d.autoPeep < 4.f;
    if (ok) g.stable += dt;

    // ---- particles ---------------------------------------------------------
    float ti = fmodf(g.phase, cycle);
    bool inspiring = (ti < inspT);

    g.spawnAcc += dt;
    while (g.spawnAcc > 0.028f) {
        g.spawnAcc -= 0.028f;
        Particle p;
        if (inspiring) {
            p.pos = { 604.f + (float)GetRandomValue(-10,10), 74.f };
            p.vel = { (float)GetRandomValue(-30,30), 260.f };
            p.col = { 80,200,255,255 };
        } else {
            p.pos = { 604.f + (float)GetRandomValue(-150,150),
                      300.f + (float)GetRandomValue(-50,50) };
            p.vel = { (float)GetRandomValue(-50,50), -240.f };
            p.col = { 140,150,170,255 };
        }
        p.maxLife = p.life = 0.85f;
        p.r = 3.0f;
        g.parts.push_back(p);
    }

    for (auto& p : g.parts) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.life  -= dt;
    }
    g.parts.erase(std::remove_if(g.parts.begin(), g.parts.end(),
                  [](const Particle& p){ return p.life <= 0.f; }),
                  g.parts.end());

    // ---- pressure trace ----------------------------------------------------
    g.sampleAcc += dt;
    while (g.sampleAcc > 0.02f) {
        g.sampleAcc -= 0.02f;
        float tInCycle = fmodf(g.phase, cycle);
        float p = InstantPressure(tInCycle, inspT, expT,
                                  g.d.peak, g.d.plateau, g.set.peep);
        g.trace.push_back(p);
        if (g.trace.size() > 400) g.trace.erase(g.trace.begin());
    }

    // ---- win / lose --------------------------------------------------------
    if (g.health <= 0.f) { g.state = STATE_END; g.won = false; }
    else if (g.t >= g.duration) { g.state = STATE_END; g.won = true; }
}

// ============================================================================
//  DRAWING HELPERS
// ============================================================================
static Color OkColor(bool ok) { return ok ? C_GOOD : C_BAD; }

static void DataRow(const char* label, const char* value, Color vc,
                    float x, float y, float w)
{
    DrawText(label, (int)x, (int)y, 17, C_DIM);
    int tw = MeasureText(value, 20);
    DrawText(value, (int)(x + w - tw), (int)y - 2, 20, vc);
}

// ---------------------------------------------------------------- lungs view
static void DrawLungs(const Game& g)
{
    DrawRectangleRec(R_LUNG, C_PANEL);
    DrawRectangleLinesEx(R_LUNG, 1, C_LINE);
    DrawText("THORAX", (int)R_LUNG.x + 12, (int)R_LUNG.y + 8, 16, C_DIM);

    const float cx  = R_LUNG.x + R_LUNG.width * 0.5f;
    const float top = R_LUNG.y + 46.f;

    // ---- breathing animation ----------------------------------------------
    float cycle = 60.0f / g.set.rr;
    float inspT = cycle * g.set.inspFrac;
    float expT  = cycle - inspT;
    float ti    = fmodf(g.phase, cycle);
    float fill;
    if (ti < inspT) fill = 0.5f - 0.5f * cosf(PI * (ti / inspT));
    else            fill = 0.5f + 0.5f * cosf(PI * ((ti - inspT) / expT));

    float scale = 0.72f + 0.30f * fill + g.set.peep / 70.0f;

    // ---- endotracheal tube -------------------------------------------------
    DrawRectangleRounded({cx-16.f, top-44.f, 32.f, 178.f}, 0.5f, 10, {58,68,92,255});
    DrawRectangleRounded({cx-11.f, top-40.f, 22.f, 170.f}, 0.5f, 10, {92,108,144,255});
    // cuff
    DrawRectangleRounded({cx-19.f, top+72.f, 38.f, 26.f}, 0.6f, 10, {140,155,190,200});

    // ---- bronchi -----------------------------------------------------------
    DrawLineEx({cx, top+126.f}, {cx- 92.f, top+186.f}, 13.f, {80,96,130,255});
    DrawLineEx({cx, top+126.f}, {cx+ 92.f, top+186.f}, 13.f, {80,96,130,255});

    // ---- lungs -------------------------------------------------------------
    float rx = 74.f * scale;
    float ry = 96.f * scale;
    float ly = top + 250.f;
    float lx = cx - 96.f;
    float rxx = cx + 96.f;

    // glow
    DrawEllipse((int)lx, (int)ly, rx+8.f, ry+8.f, {40,110,150,40});
    DrawEllipse((int)rxx,(int)ly, rx+8.f, ry+8.f, {40,110,150,40});

    // body
    DrawEllipse((int)lx, (int)ly, rx, ry, { 44, 82,110,255});
    DrawEllipse((int)rxx,(int)ly, rx, ry, { 44, 82,110,255});
    DrawEllipse((int)lx, (int)ly, rx*0.88f, ry*0.88f, { 62,120,152,255});
    DrawEllipse((int)rxx,(int)ly, rx*0.88f, ry*0.88f, { 62,120,152,255});

    // alveoli speckle
    for (int i = 0; i < 26; i++) {
        float a = (float)i * 2.399f;
        float rr1 = 0.62f * sqrtf((float)i / 26.f);
        float px = lx + cosf(a) * rr1 * rx * 0.85f;
        float py = ly + sinf(a) * rr1 * ry * 0.85f;
        DrawCircleV({px, py}, 3.2f, {100,160,190,120});

        float px2 = rxx + cosf(a+1.1f) * rr1 * rx * 0.85f;
        float py2 = ly  + sinf(a+1.1f) * rr1 * ry * 0.85f;
        DrawCircleV({px2, py2}, 3.2f, {100,160,190,120});
    }

    // ---- particles ---------------------------------------------------------
    BeginScissorMode((int)R_LUNG.x, (int)R_LUNG.y,
                     (int)R_LUNG.width, (int)R_LUNG.height);
    for (const auto& p : g.parts) {
        float a = Clampf(p.life / p.maxLife, 0.f, 1.f);
        Color c = p.col;
        c.a = (unsigned char)(200 * a);
        DrawCircleV(p.pos, p.r, c);
    }
    EndScissorMode();

    // ---- labels ------------------------------------------------------------
    const char* flowTxt = (ti < inspT) ? "INSPIRATION  (active)"
                                       : "EXPIRATION  (passive)";
    Color flowCol = (ti < inspT) ? C_ACCENT : C_DIM;
    DrawText(flowTxt, (int)(cx - MeasureText(flowTxt,18)*0.5f),
             (int)(R_LUNG.y + R_LUNG.height - 34), 18, flowCol);

    // pressure bubble
    const char* pk = TextFormat("Peak %.0f   Plateau %.0f   PEEPtot %.0f",
                                g.d.peak, g.d.plateau, g.d.totalPeep);
    DrawText(pk, (int)(cx - MeasureText(pk,16)*0.5f),
             (int)(R_LUNG.y + R_LUNG.height - 16), 16, C_DIM);
}

// ------------------------------------------------------------- waveform view
static void DrawWaveform(const Game& g)
{
    DrawRectangleRec(R_WAVE, C_PANEL);
    DrawRectangleLinesEx(R_WAVE, 1, C_LINE);
    DrawText("AIRWAY PRESSURE  (cmH2O)", (int)R_WAVE.x + 12,
             (int)R_WAVE.y + 8, 16, C_DIM);

    Rectangle plot = { R_WAVE.x + 12.f, R_WAVE.y + 34.f,
                       R_WAVE.width - 24.f, R_WAVE.height - 52.f };
    DrawRectangleRec(plot, {10,12,20,255});

    for (int i = 0; i <= 4; i++) {
        float y = plot.y + plot.height * i / 4.f;
        DrawLine((int)plot.x, (int)y, (int)(plot.x+plot.width), (int)y,
                 {28,34,48,255});
    }

    auto Y = [&](float p){
        return plot.y + plot.height * (1.f - Clampf(p,0.f,50.f)/50.f);
    };

    // 30 cmH2O danger line
    DrawLineEx({plot.x, Y(30.f)}, {plot.x+plot.width, Y(30.f)}, 1.5f,
               {110,50,50,255});
    DrawText("30", (int)(plot.x+4), (int)Y(30.f)-16, 12, {150,70,70,255});

    const int N = 400;
    int n = (int)g.trace.size();
    int off = N - n;
    if (off < 0) off = 0;

    for (int i = 1; i < n; i++) {
        float x0 = plot.x + plot.width * (float)(off + i - 1) / (float)(N - 1);
        float x1 = plot.x + plot.width * (float)(off + i)     / (float)(N - 1);
        Vector2 a = { x0, Y(g.trace[i-1]) };
        Vector2 b = { x1, Y(g.trace[i])   };
        DrawLineEx(a, b, 2.0f, C_ACCENT);
    }

    // live marker
    if (n > 0) {
        float x = plot.x + plot.width * (float)(off + n - 1) / (float)(N - 1);
        DrawCircleV({x, Y(g.trace[n-1])}, 4.f, WHITE);
    }
}

// -------------------------------------------------------------- monitor view
static void DrawMonitor(const Game& g)
{
    DrawRectangleRec(R_MON, C_PANEL);
    DrawRectangleLinesEx(R_MON, 1, C_LINE);

    float x = R_MON.x + 16.f;
    float w = R_MON.width - 32.f;
    float y = R_MON.y + 14.f;

    DrawText("PATIENT MONITOR", (int)x, (int)y, 18, C_TEXT);
    y += 34.f;

    // --- SpO2 big readout ---------------------------------------------------
    DrawText("SpO2", (int)x, (int)y, 16, C_DIM);
    bool spo2ok = g.d.spo2 >= 92.f;
    DrawText(TextFormat("%.0f%%", g.d.spo2), (int)(x + w - 110), (int)y - 12, 46,
             OkColor(spo2ok));
    y += 46.f;

    // --- rows ---------------------------------------------------------------
    bool o2ok  = g.d.paO2  >= 60.f && g.d.paO2 <= 150.f;
    bool co2ok = g.d.paCO2 >= 35.f && g.d.paCO2 <= 45.f;
    bool phok  = g.d.pH    >= 7.35f && g.d.pH  <= 7.45f;
    bool mapok = g.d.map   >= 65.f;

    DataRow("PaO2",  TextFormat("%.0f mmHg", g.d.paO2),  OkColor(o2ok),  x, y, w); y += 30.f;
    DataRow("PaCO2", TextFormat("%.0f mmHg", g.d.paCO2), OkColor(co2ok), x, y, w); y += 30.f;
    DataRow("pH",    TextFormat("%.2f",      g.d.pH),    OkColor(phok),  x, y, w); y += 30.f;
    DataRow("MAP",   TextFormat("%.0f mmHg", g.d.map),   OkColor(mapok), x, y, w); y += 34.f;

    DrawLine((int)x, (int)y, (int)(x+w), (int)y, C_LINE); y += 14.f;

    // --- ventilator numbers -------------------------------------------------
    bool plok = g.d.plateau < 30.f;
    bool apok = g.d.autoPeep < 4.f;
    bool fiok = g.set.fio2 <= 0.6f;

    DataRow("Peak Pressure", TextFormat("%.0f", g.d.peak),
            C_TEXT, x, y, w); y += 26.f;
    DataRow("Plateau",       TextFormat("%.0f", g.d.plateau),
            OkColor(plok), x, y, w); y += 26.f;
    DataRow("Auto-PEEP",     TextFormat("%.1f", g.d.autoPeep),
            OkColor(apok), x, y, w); y += 26.f;
    DataRow("Minute Vol",    TextFormat("%.1f L/min", g.d.minuteVent),
            C_TEXT, x, y, w); y += 26.f;
    DataRow("Alveolar Vol",  TextFormat("%.1f L/min", g.d.alveolVent),
            C_TEXT, x, y, w); y += 26.f;
    DataRow("FiO2",          TextFormat("%.0f%%", g.set.fio2*100.f),
            OkColor(fiok), x, y, w); y += 26.f;

    // --- compliance / resistance read-out -----------------------------------
    y += 8.f;
    DrawLine((int)x, (int)y, (int)(x+w), (int)y, C_LINE); y += 12.f;
    DataRow("Compliance", TextFormat("%.0f mL/cmH2O",
            SCEN[g.scen].compliance * 1000.f), C_DIM, x, y, w); y += 24.f;
    DataRow("Resistance", TextFormat("%.0f cmH2O/L/s",
            SCEN[g.scen].resistance), C_DIM, x, y, w); y += 30.f;

    // --- stability score ----------------------------------------------------
    DrawText("TIME IN TARGET RANGE", (int)x, (int)y, 15, C_DIM); y += 22.f;
    DrawText(TextFormat("%.1f s", g.stable), (int)x, (int)y, 26, C_ACCENT);
}

// ---------------------------------------------------------------- controls
static void DrawControls(const Game& g)
{
    DrawRectangleRec(R_CTRL, C_PANEL);
    DrawRectangleLinesEx(R_CTRL, 1, C_LINE);

    DrawText("VENTILATOR CONTROLS", (int)R_CTRL.x + 20,
             (int)R_CTRL.y + 16, 20, C_TEXT);
    DrawText("Volume pre-set / pressure limited", (int)R_CTRL.x + 20,
             (int)R_CTRL.y + 42, 14, C_DIM);

    for (size_t i = 0; i < g.sliders.size(); i++) {
        const Slider& s = g.sliders[i];
        bool selected = ((int)i == g.sel);

        Color lbl = selected ? C_ACCENT : C_TEXT;
        DrawText(s.label.c_str(), (int)s.track.x, (int)s.track.y - 34, 17, lbl);

        const char* v = TextFormat("%.0f%s", (*s.val) * s.mul, s.unit);
        int tw = MeasureText(v, 18);
        DrawText(v, (int)(s.track.x + s.track.width - tw),
                 (int)s.track.y - 34, 18, selected ? WHITE : C_TEXT);

        // track
        DrawRectangleRounded(s.track, 1.0f, 8, C_PANEL2);
        DrawRectangleLinesEx(s.track, 1, C_LINE);

        // fill
        float u = Clampf((*s.val - s.lo) / (s.hi - s.lo), 0.f, 1.f);
        Rectangle fill = { s.track.x, s.track.y, s.track.width * u, s.track.height };
        DrawRectangleRounded(fill, 1.0f, 8, selected ? C_ACCENT : Color{0,120,170,255});

        // knob
        float kx = s.track.x + s.track.width * u;
        float ky = s.track.y + s.track.height * 0.5f;
        DrawCircleV({kx, ky}, 9.f, selected ? WHITE : Color{180,200,220,255});
        if (selected) DrawCircleLines((int)kx, (int)ky, 12.f, C_ACCENT);
    }

    // ---- warnings ----------------------------------------------------------
    float wy = R_CTRL.y + 500.f;
    DrawLine((int)R_CTRL.x + 20, (int)wy - 12,
             (int)(R_CTRL.x + R_CTRL.width - 20), (int)wy - 12, C_LINE);

    DrawText("ALERTS", (int)R_CTRL.x + 20, (int)wy, 16, C_DIM);
    wy += 24.f;

    auto alert = [&](bool bad, const char* txt) {
        if (!bad) return;
        DrawText("!", (int)R_CTRL.x + 20, (int)wy, 18, C_BAD);
        DrawText(txt, (int)R_CTRL.x + 40, (int)wy, 16, C_BAD);
        wy += 22.f;
    };

    alert(g.d.plateau > 30.f, "Barotrauma - plateau > 30");
    alert(g.d.peak > 40.f,    "High peak airway pressure");
    alert(g.d.autoPeep > 4.f, "Gas trapping - auto-PEEP");
    alert(g.set.fio2 > 0.6f,  "Oxygen toxicity risk (FiO2 > 60%)");
    alert(g.set.tidalVolume / 70.f > 8.f, "Volutrauma - Vt > 8 mL/kg");
    alert(g.d.map < 65.f,     "Hypotension - high ITP");
    alert(g.d.paCO2 < 32.f,   "Hypocarbia - cerebral vasoconstriction");
    alert(g.d.paO2 < 60.f,    "Hypoxaemia - shunt / V-Q mismatch");

    if (wy < R_CTRL.y + 520.f)
        DrawText("All parameters acceptable.", (int)R_CTRL.x + 20,
                 (int)wy, 16, C_GOOD);
}

// ------------------------------------------------------------------- header
static void DrawHeader(const Game& g)
{
    DrawRectangle(0, 0, SW, 50, C_PANEL);
    DrawLine(0, 50, SW, 50, C_LINE);

    if (g.state == STATE_PLAY || g.state == STATE_END) {
        DrawText(SCEN[g.scen].name, 20, 15, 22, SCEN[g.scen].tint);

        // timer
        const char* tt = TextFormat("T+ %05.1f s", g.t);
        DrawText(tt, 470, 17, 20, C_TEXT);

        // health bar
        DrawText("PATIENT", 700, 17, 16, C_DIM);
        Rectangle bar = { 780.f, 16.f, 300.f, 20.f };
        DrawRectangleRounded(bar, 0.5f, 8, C_PANEL2);
        float u = g.health / 100.f;
        Rectangle fill = { bar.x, bar.y, bar.width * u, bar.height };
        Color hc = g.health > 60.f ? C_GOOD :
                   g.health > 30.f ? C_WARN : C_BAD;
        DrawRectangleRounded(fill, 0.5f, 8, hc);
        DrawRectangleLinesEx(bar, 1, C_LINE);
        DrawText(TextFormat("%.0f%%", g.health), 1095, 17, 18, C_TEXT);
    } else {
        DrawText("MECHANICAL VENTILATION SIMULATOR", 20, 14, 22, C_TEXT);
    }
}

// --------------------------------------------------------------------- menu
static void DrawMenu(const Game& g)
{
    const char* title = "BASIC PRINCIPLES OF MECHANICAL VENTILATION";
    DrawText(title, SW/2 - MeasureText(title, 34)/2, 60, 34, C_TEXT);

    const char* sub = "Choose a patient. Keep them alive for 120 seconds.";
    DrawText(sub, SW/2 - MeasureText(sub, 20)/2, 108, 20, C_DIM);

    Vector2 m = GetMousePosition();

    for (int i = 0; i < NSCEN; i++) {
        float x = 90.f + (i % 2) * 580.f;
        float y = 200.f + (i / 2) * 210.f;
        Rectangle r = { x, y, 520.f, 180.f };
        bool hov = CheckCollisionPointRec(m, r);

        DrawRectangleRec(r, hov ? C_PANEL2 : C_PANEL);
        DrawRectangleLinesEx(r, hov ? 3.f : 1.f, hov ? SCEN[i].tint : C_LINE);

        DrawRectangle((int)r.x, (int)r.y, 8, (int)r.height, SCEN[i].tint);

        DrawText(SCEN[i].name, (int)r.x + 26, (int)r.y + 26, 26, SCEN[i].tint);

        // wrap blurb manually on two lines
        const char* b = SCEN[i].blurb;
        DrawText(b, (int)r.x + 26, (int)r.y + 74, 18, C_TEXT);

        DrawText(TextFormat("Compliance %.0f mL/cmH2O",
                 SCEN[i].compliance*1000.f),
                 (int)r.x + 26, (int)r.y + 116, 16, C_DIM);
        DrawText(TextFormat("Resistance %.0f cmH2O/L/s   Shunt %.0f%%",
                 SCEN[i].resistance, SCEN[i].baseShunt*100.f),
                 (int)r.x + 26, (int)r.y + 140, 16, C_DIM);
    }

    const char* help =
        "MOUSE: drag sliders   |   UP/DOWN: select   |   LEFT/RIGHT: fine tune";
    DrawText(help, SW/2 - MeasureText(help, 18)/2, 660, 18, C_DIM);
}

// ----------------------------------------------------------------- endgame
static void DrawEnd(const Game& g)
{
    Color c = g.won ? C_GOOD : C_BAD;
    const char* big = g.won ? "PATIENT STABILISED" : "PATIENT LOST";
    DrawText(big, SW/2 - MeasureText(big, 52)/2, 200, 52, c);

    const char* sub = g.won
        ? "You maintained adequate gas exchange for 120 seconds."
        : "Ventilation failed. Review your settings.";
    DrawText(sub, SW/2 - MeasureText(sub, 22)/2, 274, 22, C_TEXT);

    DrawText(TextFormat("Time in target range : %.1f s", g.stable),
             SW/2 - 190, 340, 26, C_ACCENT);
    DrawText(TextFormat("Final PaO2  : %.0f mmHg", g.d.paO2),
             SW/2 - 190, 380, 22, C_TEXT);
    DrawText(TextFormat("Final PaCO2 : %.0f mmHg", g.d.paCO2),
             SW/2 - 190, 412, 22, C_TEXT);
    DrawText(TextFormat("Final pH    : %.2f", g.d.pH),
             SW/2 - 190, 444, 22, C_TEXT);
    DrawText(TextFormat("Plateau P   : %.0f cmH2O", g.d.plateau),
             SW/2 - 190, 476, 22, C_TEXT);

    const char* hint = "Press ENTER or CLICK to return to the menu";
    DrawText(hint, SW/2 - MeasureText(hint, 20)/2, 580, 20, C_DIM);
}

// ============================================================================
//  MASTER DRAW
// ============================================================================
static void DrawGame(const Game& g)
{
    DrawHeader(g);

    if (g.state == STATE_MENU) { DrawMenu(g); return; }

    DrawMonitor(g);
    DrawLungs(g);
    DrawWaveform(g);
    DrawControls(g);

    if (g.state == STATE_END) {
        DrawRectangle(0, 0, SW, SH, {0,0,0,190});
        DrawEnd(g);
    }

    // grace-period hint
    if (g.state == STATE_PLAY && g.grace > 0.f) {
        const char* s = TextFormat("Assessing patient... %.0f", g.grace);
        DrawText(s, 620 - MeasureText(s,20)/2, 300, 20, C_WARN);
    }
}

// ============================================================================
//  MAIN
// ============================================================================
int main()
{
    InitWindow(SW, SH, "Mechanical Ventilation Simulator");
    SetTargetFPS(60);

    Game game;
    SetupSliders(game);
    game.d = Compute(game.set, SCEN[0]);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;   // clamp after window drag

        UpdateGame(game, dt);

        BeginDrawing();
        ClearBackground(C_BG);
        DrawGame(game);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}