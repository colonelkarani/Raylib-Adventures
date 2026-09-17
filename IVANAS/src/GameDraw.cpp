// =====================================================================================
//  GameDraw.cpp -- all rendering for the Game class, split out from Game.cpp for
//  readability. Pure presentation; no gameplay state is mutated here.
// =====================================================================================
#include "Game.h"
#include <cstdio>

static void DrawPanel(Rectangle r, const char* title) {
    DrawRectangleRec(r, Pal::PANEL);
    DrawRectangleLinesEx(r, 2.0f, Pal::PANEL_EDGE);
    if (title) DrawText(title, (int)r.x + 10, (int)r.y + 8, 16, Pal::MONITOR_GRN);
}

static void DrawWrappedText(const char* text, int x, int y, int w, int fontSize, Color color) {
    // Minimal word-wrap for briefing / codex prose blocks.
    std::string s(text);
    std::string line, word;
    int cy = y;
    size_t i = 0;
    auto flushLine = [&](const std::string& l) {
        DrawText(l.c_str(), x, cy, fontSize, color);
        cy += fontSize + 6;
    };
    while (i <= s.size()) {
        char c = (i < s.size()) ? s[i] : ' ';
        if (c == ' ' || c == '\n') {
            std::string trial = line.empty() ? word : line + " " + word;
            if (MeasureText(trial.c_str(), fontSize) > w && !line.empty()) {
                flushLine(line);
                line = word;
            } else {
                line = trial;
            }
            word.clear();
            if (c == '\n') { flushLine(line); line.clear(); }
        } else {
            word += c;
        }
        i++;
    }
    if (!line.empty()) flushLine(line);
}

void Game::Draw() {
    ClearBackground(Pal::BG);
    switch (screen_) {
        case GameScreen::MENU:        DrawMenu(); break;
        case GameScreen::CODEX:       DrawCodex(); break;
        case GameScreen::SETTINGS:    DrawSettings(); break;
        case GameScreen::BRIEFING:    DrawBriefing(); break;
        case GameScreen::PLAYING:     DrawPlaying(); break;
        case GameScreen::PAUSED:      DrawPaused(); break;
        case GameScreen::LEVEL_CLEAR: DrawLevelClear(); break;
        case GameScreen::LEVEL_FAIL:  DrawLevelFail(); break;
        case GameScreen::GAME_WIN:    DrawGameWin(); break;
    }
}

void Game::DrawMenu() {
    const char* title = "DO NO HARM: OR DEFENSE";
    int tw = MeasureText(title, 48);
    DrawText(title, SCREEN_W / 2 - tw / 2, 140, 48, Pal::MONITOR_GRN);
    const char* sub = "A medically-grounded top-down area defense about intravenous anesthetics";
    int sw = MeasureText(sub, 18);
    DrawText(sub, SCREEN_W / 2 - sw / 2, 200, 18, Pal::TEXT_DIM);

    const char* opts[3] = { "Start Campaign (8 Scenarios)", "Open Drug Codex", "Settings" };
    for (int i = 0; i < 3; i++) {
        bool sel = (menuSelection_ == i);
        Color c = sel ? Pal::MONITOR_AMB : Pal::TEXT_BRIGHT;
        int w = MeasureText(opts[i], 26);
        int y = 320 + i * 50;
        if (sel) DrawText(">", SCREEN_W / 2 - w / 2 - 30, y, 26, Pal::MONITOR_AMB);
        DrawText(opts[i], SCREEN_W / 2 - w / 2, y, 26, c);
    }
    DrawText("Arrow keys to choose, Enter to select. Press C anytime for the Codex.",
        SCREEN_W / 2 - 260, 460, 16, Pal::TEXT_DIM);

    DrawText("Mouse: aim & fire a bolus.  1-9: select agent.  Wheel: cycle agent.",
        SCREEN_W / 2 - 260, 500, 16, Pal::TEXT_DIM);

    DrawText("Every dose, onset, duration, and clinical effect in this game is drawn from",
        SCREEN_W / 2 - 330, 700, 14, Pal::TEXT_DIM);
    DrawText("Ch. 8, \"Intravenous Anesthetics\" -- gameplay numbers are original, the pharmacology is not.",
        SCREEN_W / 2 - 330, 720, 14, Pal::TEXT_DIM);
}

void Game::DrawCodex() {
    DrawText("CODEX", 40, 30, 32, Pal::MONITOR_GRN);
    DrawText("[TAB] switch section    [Left/Right or A/D] browse    [ESC] back", 40, 70, 16, Pal::TEXT_DIM);

    const char* tabs[3] = { "Agents", "Concepts", "Data Tables" };
    for (int i = 0; i < 3; i++) {
        Color c = (codexTab_ == i) ? Pal::MONITOR_AMB : Pal::TEXT_DIM;
        DrawText(tabs[i], 40 + i * 140, 100, 20, c);
    }

    Rectangle panel = { 40, 140, (float)SCREEN_W - 80, (float)SCREEN_H - 200 };
    DrawPanel(panel, nullptr);

    if (codexTab_ == 0) {
        const DrugProfile& d = GetDrug((DrugID)codexDrugIndex_);
        DrawText(TextFormat("%d / %d", codexDrugIndex_ + 1, (int)DrugID::COUNT), (int)panel.x + (int)panel.width - 90, (int)panel.y + 14, 16, Pal::TEXT_DIM);
        DrawCircle((int)panel.x + 50, (int)panel.y + 55, 22, d.color);
        DrawText(d.name.c_str(), (int)panel.x + 90, (int)panel.y + 30, 30, Pal::TEXT_BRIGHT);
        std::string doseStr = d.doseHighMgKg > 0.001f
            ? TextFormat("Induction dose: %.2f-%.2f mg/kg IV   |   Duration: %.0f-%.0f min   |   Clearance: %.1f mL/kg/min",
                         d.doseLowMgKg, d.doseHighMgKg, d.durationLowMin, d.durationHighMin, d.clearanceMlKgMin)
            : TextFormat("Not used as a bolus induction agent (N/A)  |  Duration: %.0f-%.0f min  |  Clearance: %.1f mL/kg/min",
                         d.durationLowMin, d.durationHighMin, d.clearanceMlKgMin);
        DrawText(doseStr.c_str(), (int)panel.x + 90, (int)panel.y + 66, 15, Pal::MONITOR_AMB);

        DrawWrappedText(d.codexBlurb.c_str(), (int)panel.x + 30, (int)panel.y + 110, (int)panel.width - 60, 17, Pal::TEXT_BRIGHT);

        // quick flag row
        int fy = (int)panel.y + (int)panel.height - 130;
        int fx = (int)panel.x + 30;
        auto flag = [&](const char* label, bool on) {
            Color c = on ? Pal::MONITOR_GRN : Pal::TEXT_DIM;
            DrawText(TextFormat("%s: %s", label, on ? "YES" : "no"), fx, fy, 15, c);
            fy += 20;
        };
        DrawText("Clinical flags:", fx, fy, 16, Pal::TEXT_DIM); fy += 24;
        flag("True analgesia", d.analgesic);
        flag("Strong amnesia", d.amnestic);
        flag("Anticonvulsant", d.anticonvulsant);
        flag("Lowers ICP", d.loweredICP);
        flag("Raises ICP (avoid in neuro cases)", d.raisesICP);
        flag("Adrenal suppression risk", d.adrenalSuppressant);
        flag("Reversible with an antagonist", d.hasAntagonist);
    } else if (codexTab_ == 1) {
        const auto& entries = GetConceptEntries();
        const CodexEntry& e = entries[codexConceptIndex_];
        DrawText(TextFormat("%d / %d", codexConceptIndex_ + 1, (int)entries.size()), (int)panel.x + (int)panel.width - 90, (int)panel.y + 14, 16, Pal::TEXT_DIM);
        DrawText(e.title.c_str(), (int)panel.x + 30, (int)panel.y + 30, 26, Pal::MONITOR_BLU);
        DrawWrappedText(e.body.c_str(), (int)panel.x + 30, (int)panel.y + 74, (int)panel.width - 60, 18, Pal::TEXT_BRIGHT);
    } else {
        // ---- Data Tables tab: mirrors the source chapter's Table 8.1 (pharmacokinetics)
        // and the qualitative direction-of-effect columns from Table 8.2. ----
        const auto& roster = GetDrugRoster();
        DrawText("Table 8.1-style Pharmacokinetics", (int)panel.x + 20, (int)panel.y + 14, 18, Pal::MONITOR_BLU);

        int colX[6] = { 20, 190, 340, 470, 610, 740 };
        const char* headers[6] = { "Agent", "Dose (mg/kg)", "Duration (min)", "Clearance", "Protein Bind", "CSHT trend" };
        int hy = (int)panel.y + 44;
        for (int c = 0; c < 6; c++) DrawText(headers[c], (int)panel.x + colX[c], hy, 14, Pal::TEXT_DIM);
        DrawLine((int)panel.x + 16, hy + 18, (int)panel.x + (int)panel.width - 16, hy + 18, Pal::PANEL_EDGE);

        int ry = hy + 26;
        for (const auto& d : roster) {
            Color rowColor = Pal::TEXT_BRIGHT;
            DrawText(d.name.c_str(), (int)panel.x + colX[0], ry, 14, rowColor);
            std::string dose = d.doseHighMgKg > 0.001f ? TextFormat("%.2f-%.2f", d.doseLowMgKg, d.doseHighMgKg) : "N/A (infusion)";
            DrawText(dose.c_str(), (int)panel.x + colX[1], ry, 14, rowColor);
            DrawText(TextFormat("%.0f-%.0f", d.durationLowMin, d.durationHighMin), (int)panel.x + colX[2], ry, 14, rowColor);
            DrawText(TextFormat("%.1f mL/kg/min", d.clearanceMlKgMin), (int)panel.x + colX[3], ry, 14, rowColor);
            DrawText(TextFormat("%.0f%%", d.proteinBindingPct), (int)panel.x + colX[4], ry, 14, rowColor);
            const char* trend = d.contextHalfTimeFactor > 0.7f ? "steep" : (d.contextHalfTimeFactor > 0.35f ? "moderate" : "flat");
            Color trendColor = d.contextHalfTimeFactor > 0.7f ? Pal::MONITOR_RED : (d.contextHalfTimeFactor > 0.35f ? Pal::MONITOR_AMB : Pal::MONITOR_GRN);
            DrawText(trend, (int)panel.x + colX[5], ry, 14, trendColor);
            ry += 24;
        }

        ry += 16;
        DrawText("Table 8.2-style Pharmacodynamic Direction", (int)panel.x + 20, ry, 18, Pal::MONITOR_BLU);
        ry += 30;
        int colX2[5] = { 20, 190, 330, 470, 610 };
        const char* headers2[5] = { "Agent", "Blood Pressure", "Heart Rate", "Anticonvulsant", "Analgesia" };
        for (int c = 0; c < 5; c++) DrawText(headers2[c], (int)panel.x + colX2[c], ry, 14, Pal::TEXT_DIM);
        ry += 18;
        DrawLine((int)panel.x + 16, ry, (int)panel.x + (int)panel.width - 16, ry, Pal::PANEL_EDGE);
        ry += 8;
        auto effectStr = [](Effect e) -> const char* {
            switch (e) {
                case Effect::StrongDecrease: return "Decreased";
                case Effect::Decrease: return "Decreased";
                case Effect::Unchanged: return "Unchanged";
                case Effect::Increase: return "Increased";
                case Effect::StrongIncrease: return "Increased";
            }
            return "?";
        };
        for (const auto& d : roster) {
            DrawText(d.name.c_str(), (int)panel.x + colX2[0], ry, 14, Pal::TEXT_BRIGHT);
            DrawText(effectStr(d.bloodPressureEffect), (int)panel.x + colX2[1], ry, 14, Pal::TEXT_BRIGHT);
            DrawText(effectStr(d.heartRateEffect), (int)panel.x + colX2[2], ry, 14, Pal::TEXT_BRIGHT);
            DrawText(d.anticonvulsant ? "Yes" : (d.proconvulsant ? "No (pro-convulsant)" : "No"), (int)panel.x + colX2[3], ry, 14,
                     d.anticonvulsant ? Pal::MONITOR_GRN : Pal::TEXT_DIM);
            DrawText(d.analgesic ? "Yes" : "No", (int)panel.x + colX2[4], ry, 14, d.analgesic ? Pal::MONITOR_GRN : Pal::TEXT_DIM);
            ry += 22;
        }
    }
}

void Game::DrawBriefing() {
    DrawText(level_.title.c_str(), 60, 60, 34, Pal::MONITOR_GRN);
    DrawText(level_.subtitle.c_str(), 60, 104, 18, Pal::MONITOR_AMB);

    Rectangle panel = { 60, 150, (float)SCREEN_W - 120, 330 };
    DrawPanel(panel, "CASE BRIEFING");
    DrawWrappedText(level_.briefing.c_str(), (int)panel.x + 20, (int)panel.y + 40, (int)panel.width - 40, 18, Pal::TEXT_BRIGHT);

    int y = (int)panel.y + 220;
    if (!level_.recommended.empty()) {
        std::string s = "Indicated: ";
        for (size_t i = 0; i < level_.recommended.size(); i++) { s += GetDrug(level_.recommended[i]).name; if (i + 1 < level_.recommended.size()) s += ", "; }
        DrawText(s.c_str(), (int)panel.x + 20, y, 16, Pal::MONITOR_GRN); y += 24;
    }
    if (!level_.caution.empty()) {
        std::string s = "Use caution: ";
        for (size_t i = 0; i < level_.caution.size(); i++) { s += GetDrug(level_.caution[i]).name; if (i + 1 < level_.caution.size()) s += ", "; }
        DrawText(s.c_str(), (int)panel.x + 20, y, 16, Pal::MONITOR_RED);
    }

    DrawText(TextFormat("Scenario %d of %d", levelIndex_ + 1, (int)GetCampaign().size()), 60, 500, 18, Pal::TEXT_DIM);
    DrawText("Press ENTER to begin.", 60, 540, 20, Pal::TEXT_BRIGHT);
}

void Game::DrawArenaBackdrop() {
    Vector2 c = ArenaCenter();
    DrawCircleV(c, ARENA_RADIUS, Pal::FIELD);
    DrawCircleLines((int)c.x, (int)c.y, ARENA_RADIUS, Pal::FIELD_EDGE);
    for (int r = 100; r < (int)ARENA_RADIUS; r += 100) DrawCircleLines((int)c.x, (int)c.y, (float)r, Fade(Pal::FIELD_EDGE, 0.35f));

    // patient icon at center
    Color pc = vitals_.InDanger() ? ColorLerp(Pal::MONITOR_AMB, Pal::MONITOR_RED, sinf(GetTime() * 8.0f) * 0.5f + 0.5f) : Pal::MONITOR_BLU;
    DrawCircleV(c, 22.0f, Fade(pc, 0.25f));
    DrawCircleLines((int)c.x, (int)c.y, 22.0f, pc);
    DrawCircleV(c, 8.0f, pc);

    // IV site marker
    Vector2 iv = { c.x, c.y + ARENA_RADIUS - 14.0f };
    DrawCircleV(iv, 9.0f, Pal::TEXT_DIM);
    DrawCircleLines((int)iv.x, (int)iv.y, 9.0f, GetDrug(selectedDrug_).color);

    // aim reticle clamped to arena
    Vector2 m = GetMousePosition();
    Vector2 rel = Vector2Subtract(m, c);
    float len = Vector2Length(rel);
    if (len > ARENA_RADIUS) rel = Vector2Scale(rel, ARENA_RADIUS / len);
    Vector2 aim = Vector2Add(c, rel);
    DrawCircleLines((int)aim.x, (int)aim.y, 8.0f, Fade(GetDrug(selectedDrug_).color, 0.8f));
    DrawLine((int)aim.x - 12, (int)aim.y, (int)aim.x + 12, (int)aim.y, Fade(GetDrug(selectedDrug_).color, 0.6f));
    DrawLine((int)aim.x, (int)aim.y - 12, (int)aim.x, (int)aim.y + 12, Fade(GetDrug(selectedDrug_).color, 0.6f));

    if (level_.rewardsSeizureNotKillsIt && ectObjectiveActive_) {
        float pulse = 30.0f + sinf(GetTime() * 6.0f) * 6.0f;
        Color oc = ColorLerp(Pal::MONITOR_RED, Pal::MONITOR_GRN, ectMeter_ / 100.0f);
        DrawCircleLines((int)ectPos_.x, (int)ectPos_.y, pulse, oc);
        DrawCircleLines((int)ectPos_.x, (int)ectPos_.y, pulse - 6, Fade(oc, 0.5f));
        DrawText("SEIZURE FOCUS (OBJECTIVE)", (int)ectPos_.x - 100, (int)ectPos_.y - 60, 14, oc);
    }
}

void Game::DrawPlaying() {
    DrawArenaBackdrop();

    for (auto& b : boluses_) b.Draw();
    for (auto& e : enemies_) e.Draw();
    particles_.Draw();

    DrawHUD();
    DrawVitalsPanel();
    DrawHotbar();
    DrawToast();

    if (level_.rewardsSeizureNotKillsIt) {
        Rectangle bar = { ArenaCenter().x - 100, ArenaCenter().y - ARENA_RADIUS - 40, 200, 18 };
        DrawRectangleRec(bar, Fade(BLACK, 0.5f));
        DrawRectangleRec({ bar.x, bar.y, bar.width * (ectMeter_ / 100.0f), bar.height }, Pal::MONITOR_GRN);
        DrawRectangleLinesEx(bar, 1.5f, Pal::PANEL_EDGE);
        DrawText("Seizure adequacy", (int)bar.x, (int)bar.y - 18, 14, Pal::TEXT_DIM);
    }
}

void Game::DrawHUD() {
    DrawText(level_.title.c_str(), 20, 16, 22, Pal::TEXT_BRIGHT);
    DrawText(TextFormat("Score: %d", score_), 20, 44, 18, Pal::MONITOR_AMB);
    DrawText(TextFormat("Time: %.0fs", levelTime_), 20, 68, 16, Pal::TEXT_DIM);
    if (!level_.rewardsSeizureNotKillsIt) {
        DrawText(TextFormat("Threats cleared: %d   Breaches: %d", enemiesCleared_, enemiesReachedCenter_), 20, 90, 16, Pal::TEXT_DIM);
    }
}

static void DrawVitalBar(int x, int y, int w, const char* label, float value, float lo, float hi, float dangerLo, float dangerHi, char unit) {
    DrawText(label, x, y, 14, Pal::TEXT_DIM);
    Rectangle r = { (float)x, (float)y + 18, (float)w, 12 };
    DrawRectangleRec(r, Fade(BLACK, 0.4f));
    float pct = Clampf((value - lo) / (hi - lo), 0.0f, 1.0f);
    bool danger = value < dangerLo || value > dangerHi;
    Color c = danger ? Pal::MONITOR_RED : Pal::MONITOR_GRN;
    DrawRectangleRec({ r.x, r.y, r.width * pct, r.height }, c);
    DrawRectangleLinesEx(r, 1.0f, Pal::PANEL_EDGE);
    DrawText(TextFormat("%.0f%c", value, unit), x + w + 8, y + 16, 14, danger ? Pal::MONITOR_RED : Pal::TEXT_BRIGHT);
}

void Game::DrawVitalsPanel() {
    Rectangle panel = { (float)SCREEN_W - 260, 16, 244, 250 };
    DrawPanel(panel, "MONITOR");
    int x = (int)panel.x + 12, y = (int)panel.y + 34;
    DrawVitalBar(x, y, 150, "MAP (mmHg)", vitals_.map, 30, 160, 55, 140, ' '); y += 42;
    DrawVitalBar(x, y, 150, "Heart Rate (bpm)", vitals_.heartRate, 20, 200, 45, 160, ' '); y += 42;
    DrawVitalBar(x, y, 150, "Resp Rate (/min)", vitals_.respRate, 0, 30, 5, 30, ' '); y += 42;
    DrawVitalBar(x, y, 150, "ICP (mmHg)", vitals_.icp, 0, 50, 0, 25, ' '); y += 42;
    DrawVitalBar(x, y, 150, "Nociception load", vitals_.painLoad, 0, 100, 0, 70, '%'); y += 42;
    DrawVitalBar(x, y, 150, "Awareness risk", vitals_.awareness, 0, 100, 0, 70, '%');
}

void Game::DrawHotbar() {
    const auto& roster = GetDrugRoster();
    int n = (int)DrugID::COUNT;
    float slotW = 118, slotH = 78, gap = 8;
    float totalW = n * slotW + (n - 1) * gap;
    float startX = SCREEN_W / 2.0f - totalW / 2.0f;
    float y = SCREEN_H - slotH - 16;

    for (int i = 0; i < n; i++) {
        const DrugProfile& p = roster[i];
        const DrugRuntime& r = drugs_[i];
        Rectangle slot = { startX + i * (slotW + gap), y, slotW, slotH };
        bool sel = ((int)selectedDrug_ == i);
        DrawRectangleRec(slot, sel ? Fade(p.color, 0.22f) : Pal::PANEL);
        DrawRectangleLinesEx(slot, sel ? 2.5f : 1.5f, sel ? p.color : Pal::PANEL_EDGE);

        DrawText(TextFormat("%d", i + 1), (int)slot.x + 6, (int)slot.y + 4, 14, Pal::TEXT_DIM);
        DrawText(p.shortName.c_str(), (int)slot.x + 6, (int)slot.y + 20, 16, Pal::TEXT_BRIGHT);

        // budget bar
        Rectangle bBar = { slot.x + 6, slot.y + 42, slot.width - 12, 8 };
        DrawRectangleRec(bBar, Fade(BLACK, 0.4f));
        DrawRectangleRec({ bBar.x, bBar.y, bBar.width * Clampf(r.budget / r.budgetMax, 0, 1), bBar.height }, p.color);
        DrawRectangleLinesEx(bBar, 1.0f, Pal::PANEL_EDGE);

        // load / accumulation bar (context-sensitive half-time proxy)
        Rectangle lBar = { slot.x + 6, slot.y + 54, slot.width - 12, 5 };
        DrawRectangleRec(lBar, Fade(BLACK, 0.4f));
        Color loadColor = (r.load > 45.0f) ? Pal::MONITOR_RED : Pal::MONITOR_AMB;
        DrawRectangleRec({ lBar.x, lBar.y, lBar.width * Clampf(r.load / 100.0f, 0, 1), lBar.height }, loadColor);

        if (r.cooldown > 0.0f) {
            DrawRectangleRec(slot, Fade(BLACK, 0.35f));
            DrawText("onset...", (int)slot.x + 20, (int)slot.y + 32, 12, Pal::TEXT_DIM);
        }
    }
    DrawText("1-9 select   Left-click fire   Wheel cycle", (int)startX, (int)y - 20, 14, Pal::TEXT_DIM);
}

void Game::DrawToast() {
    if (toastTimer_ <= 0.0f || lastToast_.empty()) return;
    int w = MeasureText(lastToast_.c_str(), 18);
    int x = SCREEN_W / 2 - w / 2, y = 120;
    DrawRectangle(x - 14, y - 8, w + 28, 32, Fade(BLACK, 0.55f));
    DrawText(lastToast_.c_str(), x, y, 18, Pal::MONITOR_AMB);
}

void Game::DrawSettings() {
    DrawText("SETTINGS", 40, 30, 32, Pal::MONITOR_GRN);
    Rectangle panel = { (float)SCREEN_W / 2 - 300, 140, 600, 260 };
    DrawPanel(panel, nullptr);

    const char* labels[3] = { "Volume", "Difficulty", "Muted" };
    for (int i = 0; i < 3; i++) {
        Color c = (settingsSelection_ == i) ? Pal::MONITOR_AMB : Pal::TEXT_BRIGHT;
        int y = (int)panel.y + 30 + i * 60;
        DrawText(labels[i], (int)panel.x + 30, y, 20, c);
        Rectangle bar = { panel.x + 220, (float)y + 2, 260, 16 };
        DrawRectangleRec(bar, Fade(BLACK, 0.4f));
        float pct = (i == 0) ? volume_ : (i == 1) ? (difficulty_ - 0.7f) / 0.7f : (muted_ ? 1.0f : 0.0f);
        DrawRectangleRec({ bar.x, bar.y, bar.width * Clampf(pct, 0, 1), bar.height }, Pal::MONITOR_GRN);
        DrawRectangleLinesEx(bar, 1.5f, Pal::PANEL_EDGE);
        std::string valStr = (i == 0) ? TextFormat("%.0f%%", volume_ * 100.0f)
                            : (i == 1) ? TextFormat("x%.1f", difficulty_)
                            : (muted_ ? "ON" : "off");
        DrawText(valStr.c_str(), (int)bar.x + (int)bar.width + 16, y, 18, Pal::TEXT_DIM);
    }
    DrawText("Left/Right to adjust, Enter to toggle mute, ESC to go back.", (int)panel.x + 30, (int)panel.y + 220, 15, Pal::TEXT_DIM);
}

void Game::DrawPaused() {
    DrawPlaying();
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.55f));
    const char* t = "PAUSED";
    int w = MeasureText(t, 44);
    DrawText(t, SCREEN_W / 2 - w / 2, SCREEN_H / 2 - 60, 44, Pal::TEXT_BRIGHT);
    DrawText("ESC / P to resume    Q to quit to menu", SCREEN_W / 2 - 170, SCREEN_H / 2 + 4, 18, Pal::TEXT_DIM);
}

void Game::DrawClinicalReport(Rectangle panel) const {
    int x = (int)panel.x + 20, y = (int)panel.y;
    DrawText("Clinical Report", x, y, 18, Pal::MONITOR_BLU); y += 28;
    DrawText(TextFormat("Threats reaching patient: %d", enemiesReachedCenter_), x, y, 15, Pal::TEXT_BRIGHT); y += 20;
    DrawText(TextFormat("MAP range: %.0f - %.0f mmHg", minMapSeen_, maxMapSeen_), x, y, 15, Pal::TEXT_BRIGHT); y += 20;
    DrawText(TextFormat("Heart rate range: %.0f - %.0f bpm", minHrSeen_, maxHrSeen_), x, y, 15, Pal::TEXT_BRIGHT); y += 20;
    DrawText(TextFormat("Peak ICP: %.0f mmHg", maxIcpSeen_), x, y, 15, Pal::TEXT_BRIGHT); y += 20;
    DrawText(TextFormat("Peak delirium risk: %.0f%%", peakDelirium_), x, y, 15, Pal::TEXT_BRIGHT); y += 20;
    std::string grade = ComputeGrade();
    DrawText(TextFormat("Grade: %s", grade.c_str()), x, y, 18, Pal::MONITOR_AMB);
}

void Game::DrawLevelClear() {
    const char* t = "SCENARIO COMPLETE";
    int w = MeasureText(t, 40);
    DrawText(t, SCREEN_W / 2 - w / 2, 100, 40, Pal::MONITOR_GRN);
    DrawText(TextFormat("Score: %d", score_), SCREEN_W / 2 - 60, 150, 22, Pal::TEXT_BRIGHT);

    Rectangle notePanel = { (float)SCREEN_W / 2 - 420, 190, 840, 190 };
    DrawPanel(notePanel, "TEACHING POINT");
    DrawWrappedText(level_.clinicalNote.c_str(), (int)notePanel.x + 20, (int)notePanel.y + 40, (int)notePanel.width - 40, 17, Pal::TEXT_BRIGHT);

    Rectangle reportPanel = { (float)SCREEN_W / 2 - 420, 400, 840, 190 };
    DrawPanel(reportPanel, nullptr);
    DrawClinicalReport({ reportPanel.x, reportPanel.y + 12, reportPanel.width, reportPanel.height });

    DrawText("Press ENTER to continue.", SCREEN_W / 2 - 110, 610, 20, Pal::TEXT_DIM);
}

void Game::DrawLevelFail() {
    const char* t = "PATIENT DECOMPENSATED";
    int w = MeasureText(t, 40);
    DrawText(t, SCREEN_W / 2 - w / 2, 140, 40, Pal::MONITOR_RED);
    int w2 = MeasureText(failReason_.c_str(), 20);
    DrawText(failReason_.c_str(), SCREEN_W / 2 - w2 / 2, 200, 20, Pal::TEXT_BRIGHT);

    Rectangle reportPanel = { (float)SCREEN_W / 2 - 420, 260, 840, 190 };
    DrawPanel(reportPanel, nullptr);
    DrawClinicalReport({ reportPanel.x, reportPanel.y + 12, reportPanel.width, reportPanel.height });

    DrawText("Press ENTER to retry the scenario, or ESC for the main menu.", SCREEN_W / 2 - 260, 480, 18, Pal::TEXT_DIM);
}

void Game::DrawGameWin() {
    const char* t = "CAMPAIGN COMPLETE";
    int w = MeasureText(t, 44);
    DrawText(t, SCREEN_W / 2 - w / 2, 220, 44, Pal::MONITOR_GRN);
    DrawText("You have run all eight scenarios of intravenous anesthetic management.", SCREEN_W / 2 - 320, 290, 18, Pal::TEXT_BRIGHT);
    DrawText("Press ENTER to return to the menu.", SCREEN_W / 2 - 150, 340, 18, Pal::TEXT_DIM);
}
