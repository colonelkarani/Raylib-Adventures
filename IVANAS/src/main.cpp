// =====================================================================================
//  main.cpp -- application entry point. Drives the Game state machine once per
//  rendered frame using raylib's own delta time. A variable timestep is used
//  deliberately (rather than a fixed-step accumulator) because several Update() calls
//  read one-shot input events like IsKeyPressed/IsMouseButtonPressed, which raylib
//  only guarantees to be true once per polled frame; an accumulator that calls
//  Update() more than once per real frame would double-fire those events.
// =====================================================================================
#include "raylib.h"
#include "Game.h"

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "Do No Harm: OR Defense -- Intravenous Anesthetics");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // we handle ESC ourselves per-screen

    Game game;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f; // avoid a huge jump after a stall/breakpoint

        game.Update(dt);

        BeginDrawing();
        game.Draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
