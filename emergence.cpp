#include "raylib.h"
#include <cmath>
#include <vector>

// --- THE ULTIMATE WINDOWS COLLISION SHIELD ---
#define CloseWindow WindowsCloseWindow
#define ShowCursor WindowsShowCursor
#define Rectangle WindowsRectangle

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef CloseWindow
#undef ShowCursor
#undef Rectangle
// ---------------------------------------------

#include "my_random_engine.cpp" 

// Structure representing an individual agent (Boid)
struct Boid {
    Vector2 position;
    Vector2 velocity;
    Color color;
};

// Vector math helper functions
Vector2 Vector2Add(Vector2 v1, Vector2 v2) { return { v1.x + v2.x, v1.y + v2.y }; }
Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) { return { v1.x - v2.x, v1.y - v2.y }; }
Vector2 Vector2Scale(Vector2 v, float scale) { return { v.x * scale, v.y * scale }; }
float Vector2Length(Vector2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
Vector2 Vector2Normalize(Vector2 v) {
    float len = Vector2Length(v);
    if (len > 0) return { v.x / len, v.y / len };
    return { 0, 0 };
}
float Vector2Distance(Vector2 v1, Vector2 v2) { return Vector2Length(Vector2Subtract(v1, v2)); }

int main() {
    // Hide window from rendering initially on execution
    SetConfigFlags(FLAG_WINDOW_HIDDEN); 
    InitWindow(800, 600, "Emergent Complexity Screensaver");

    int screenWidth = GetMonitorWidth(0);
    int screenHeight = GetMonitorHeight(0);
    SetWindowSize(screenWidth, screenHeight);

    // Flocking Hyperparameters
    const int BOID_COUNT = 180;
    const float VISUAL_RANGE = 80.0f;    // Distance to notice neighbors
    const float MIN_DISTANCE = 25.0f;    // Keep-away distance (Separation)
    const float MAX_SPEED = 5.0f;        // Speed ceiling
    const float MIN_SPEED = 2.5f;        // Speed floor
    
    // Rule weights (Tweak these to alter emergent behavior)
    const float SEPARATION_WT = 0.15f;
    const float ALIGNMENT_WT  = 0.05f;
    const float COHESION_WT   = 0.01f;

    std::vector<Boid> flock(BOID_COUNT);

    // Initialize flock randomly spread across the canvas
    for (int i = 0; i < BOID_COUNT; i++) {
        flock[i].position = { (float)find_random_int(0, screenWidth), (float)find_random_int(0, screenHeight) };
        
        // Random velocity vector
        float angle = (float)find_random_int(0, 360) * (3.14159f / 180.0f);
        float speed = (float)find_random_int((int)MIN_SPEED, (int)MAX_SPEED);
        flock[i].velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
        
        // Give each boid a slight variance of the emergent theme color
        float randomHue = (float)find_random_int(120, 240); // Cyan/Blue spectrum
        flock[i].color = ColorFromHSV(randomHue, 0.8f, 0.9f);
    }

    bool isScreensaverActive = false;
    float timeSinceLastInput = 0.0f;       
    const float IDLE_TIME_LIMIT = 5.0f; 

    POINT lastMousePos;
    GetCursorPos(&lastMousePos);
    int startupFrames = 0; 

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        POINT currentMousePos;
        GetCursorPos(&currentMousePos);
        bool inputDetected = false;

        if (startupFrames > 10) {
            if (currentMousePos.x != lastMousePos.x || currentMousePos.y != lastMousePos.y) {
                inputDetected = true;
            }

            for (int key = 8; key <= 255; key++) {
                if (GetAsyncKeyState(key) & 0x8000) {
                    inputDetected = true;
                    break;
                }
            }
        } else {
            startupFrames++;
        }

        lastMousePos = currentMousePos;

        if (inputDetected) {
            timeSinceLastInput = 0.0f; 

            if (isScreensaverActive) {
                isScreensaverActive = false;
                ClearWindowState(FLAG_FULLSCREEN_MODE);
                SetWindowState(FLAG_WINDOW_HIDDEN);
                
                HWND hwnd = (HWND)GetWindowHandle();
                ShowWindow(hwnd, SW_HIDE); 
            }
        } else {
            timeSinceLastInput += deltaTime;

            if (timeSinceLastInput >= IDLE_TIME_LIMIT && !isScreensaverActive) {
                isScreensaverActive = true;

                HWND hwnd = (HWND)GetWindowHandle();
                ShowWindow(hwnd, SW_SHOW); 
                SetWindowState(FLAG_FULLSCREEN_MODE);
                HideCursor(); 
                
                // Re-scramble positions so every activation spawns a new organization phase
                for (int i = 0; i < BOID_COUNT; i++) {
                    flock[i].position = { (float)find_random_int(0, screenWidth), (float)find_random_int(0, screenHeight) };
                }
            }
        }

        if (isScreensaverActive) {
            // Update individual physics based on local emergence calculations
            for (int i = 0; i < BOID_COUNT; i++) {
                Vector2 close_d = { 0, 0 };      // For Separation
                Vector2 avg_vel = { 0, 0 };      // For Alignment
                Vector2 avg_pos = { 0, 0 };      // For Cohesion
                int neighboring_boids = 0;

                for (int j = 0; j < BOID_COUNT; j++) {
                    if (i == j) continue;

                    float dist = Vector2Distance(flock[i].position, flock[j].position);

                    if (dist < VISUAL_RANGE) {
                        // Rule 1: Separation (Run away if way too close)
                        if (dist < MIN_DISTANCE) {
                            close_d = Vector2Add(close_d, Vector2Subtract(flock[i].position, flock[j].position));
                        }
                        
                        // Rule 2 & 3: Accumulate data for Alignment and Cohesion
                        avg_vel = Vector2Add(avg_vel, flock[j].velocity);
                        avg_pos = Vector2Add(avg_pos, flock[j].position);
                        neighboring_boids++;
                    }
                }

                // Apply rules if neighbors are around
                if (neighboring_boids > 0) {
                    // Turn sums into true averages
                    avg_vel = Vector2Scale(avg_vel, 1.0f / neighboring_boids);
                    avg_pos = Vector2Scale(avg_pos, 1.0f / neighboring_boids);

                    // Alignment steering
                    Vector2 align_steering = Vector2Subtract(avg_vel, flock[i].velocity);
                    flock[i].velocity = Vector2Add(flock[i].velocity, Vector2Scale(align_steering, ALIGNMENT_WT));

                    // Cohesion steering (move toward center of mass)
                    Vector2 coh_steering = Vector2Subtract(avg_pos, flock[i].position);
                    flock[i].velocity = Vector2Add(flock[i].velocity, Vector2Scale(coh_steering, COHESION_WT));
                }

                // Always apply separation forces to maintain independent structures
                flock[i].velocity = Vector2Add(flock[i].velocity, Vector2Scale(close_d, SEPARATION_WT));

                // Speed clamping to maintain steady simulation kinetics
                float speed = Vector2Length(flock[i].velocity);
                if (speed > MAX_SPEED) {
                    flock[i].velocity = Vector2Scale(Vector2Normalize(flock[i].velocity), MAX_SPEED);
                } else if (speed < MIN_SPEED) {
                    flock[i].velocity = Vector2Scale(Vector2Normalize(flock[i].velocity), MIN_SPEED);
                }

                // Step forward execution
                flock[i].position = Vector2Add(flock[i].position, flock[i].velocity);

                // Screen wrapping (toroidal topology) so the flow never stops
                if (flock[i].position.x < 0) flock[i].position.x += screenWidth;
                if (flock[i].position.x > screenWidth) flock[i].position.x -= screenWidth;
                if (flock[i].position.y < 0) flock[i].position.y += screenHeight;
                if (flock[i].position.y > screenHeight) flock[i].position.y -= screenHeight;
            }

            BeginDrawing();
            ClearBackground(BLACK); 

            // Draw each flocking agent as a smooth geometric pointer
            for (int i = 0; i < BOID_COUNT; i++) {
                Vector2 dir = Vector2Normalize(flock[i].velocity);
                
                // Define 3 vertices forming a triangle pointed towards the velocity vector
                Vector2 apex = Vector2Add(flock[i].position, Vector2Scale(dir, 10.0f));
                Vector2 leftTail = Vector2Add(flock[i].position, Vector2Add(Vector2Scale(dir, -6.0f), Vector2Scale({-dir.y, dir.x}, 4.0f)));
                Vector2 rightTail = Vector2Add(flock[i].position, Vector2Add(Vector2Scale(dir, -6.0f), Vector2Scale({dir.y, -dir.x}, 4.0f)));

                DrawTriangle(apex, leftTail, rightTail, flock[i].color);
            }

            EndDrawing();
        } else {
            BeginDrawing();
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}

