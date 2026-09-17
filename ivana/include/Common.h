// =====================================================================================
//  Common.h
//  Shared constants, small helpers, and enums used across the whole project.
//
//  MEDICAL ACCURACY NOTE
//  ----------------------------------------------------------------------------------
//  Every numeric value that represents a real pharmacologic quantity in this project
//  (doses, onset times, durations of action, context-sensitive half-times, clearance,
//  protein binding, receptor mechanisms, and the direction of every listed clinical
//  effect) is sourced from the "Intravenous Anesthetics" chapter of a standard
//  anesthesiology pharmacology text (the nine agents in Box 8.1: propofol,
//  fospropofol, thiopental, methohexital, diazepam, midazolam, lorazepam,
//  remimazolam*, ketamine, etomidate, dexmedetomidine). Gameplay numbers derived from
//  those quantities (fire cooldowns, resource costs, regen rates, AoE radii, enemy
//  HP/speed) are original game-design choices layered on top of the real data -- the
//  underlying pharmacology facts themselves are not invented.
//  (*Remimazolam is referenced in the Codex but not implemented as a playable agent,
//  matching its "investigational / emerging" status in the source material.)
// =====================================================================================
#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <functional>

// ---------------------------------------------------------------------------- Screen
constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 800;
constexpr float ARENA_RADIUS = 340.0f;                 // "sterile field" play radius
inline Vector2 ARENA_CENTER() { return { SCREEN_W * 0.62f, SCREEN_H * 0.52f }; }

// ---------------------------------------------------------------------------- Timing
constexpr float FIXED_DT = 1.0f / 60.0f;

// ---------------------------------------------------------------------------- Utility
inline float Clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float Lerp1(float a, float b, float t) { return a + (b - a) * t; }
inline float RandRangef(float lo, float hi) { return lo + (hi - lo) * (float)GetRandomValue(0, 10000) / 10000.0f; }
inline float Deg2Rad(float d) { return d * (PI / 180.0f); }

// A soft clinical color palette (OR monitor greens/ambers rather than garish arcade neon)
namespace Pal {
    constexpr Color BG          = { 12, 16, 20, 255 };
    constexpr Color FIELD       = { 18, 26, 30, 255 };
    constexpr Color FIELD_EDGE  = { 60, 120, 110, 255 };
    constexpr Color MONITOR_GRN = { 80, 220, 140, 255 };
    constexpr Color MONITOR_AMB = { 235, 180, 70, 255 };
    constexpr Color MONITOR_RED = { 235, 90, 90, 255 };
    constexpr Color MONITOR_BLU = { 110, 180, 235, 255 };
    constexpr Color TEXT_DIM    = { 150, 170, 175, 255 };
    constexpr Color TEXT_BRIGHT = { 225, 235, 235, 255 };
    constexpr Color PANEL       = { 20, 28, 32, 230 };
    constexpr Color PANEL_EDGE  = { 55, 75, 80, 255 };
}

// ---------------------------------------------------------------------------- Enums
enum class DrugID : int {
    Propofol = 0,
    Thiopental,
    Methohexital,
    Midazolam,
    Diazepam,
    Lorazepam,
    Ketamine,
    Etomidate,
    Dexmedetomidine,
    COUNT
};

enum class ThreatKind : int {
    Nociception,     // pain / surgical stimulus spike
    Awareness,       // risk of explicit intraoperative recall
    AirwayReflex,    // laryngospasm / bronchospasm / reflex movement on instrumentation
    SeizureFocus,    // epileptiform activity
    SympatheticSurge,// tachycardia / hypertension from surgical stress
    VagalBradycardia,// bradycardia / heart-block risk
    PONV,            // postoperative nausea & vomiting
    ICPSurge,        // raised intracranial pressure (boss-tier)
    COUNT
};

enum class GameScreen { MENU, CODEX, BRIEFING, PLAYING, PAUSED, SETTINGS, LEVEL_CLEAR, LEVEL_FAIL, GAME_WIN };
