#include "raylib.h"

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
#include <vector>

// Lorenz Attractor parameters (Standard chaotic values)
const double SIGMA = 10.0;
const double RHO = 28.0;
const double BETA = 8.0 / 3.0;

struct Point3D {
    double x, y, z;
};

int main() {
    // Hide window from rendering initially on execution
    SetConfigFlags(FLAG_WINDOW_HIDDEN); 
    InitWindow(800, 600, "Chaos Theory Butterfly Screensaver");

    int screenWidth = GetMonitorWidth(0);
    int screenHeight = GetMonitorHeight(0);
    SetWindowSize(screenWidth, screenHeight);

    // Chaos state variables
    Point3D lorenz = { 0.1, 0.0, 0.0 }; // Initial starting seed
    double dt = 0.01;                  // Time step for integration
    
    // Trajectory tracking
    std::vector<Vector2> trail;
    const size_t MAX_TRAIL_POINTS = 4000; 
    
    // Scale and offsets to center the butterfly on any display resolution
    float scale = (screenHeight / 60.0f); 
    float offsetX = screenWidth / 2.0f;
    float offsetY = screenHeight / 2.0f + (10 * scale); // Shift slightly down to account for Z-axis offset

    // Color states
    float currentHue = (float)find_random_int(0, 360);
    Color systemColor = ColorFromHSV(currentHue, 0.9f, 0.9f);

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
                trail.clear(); // Clear trail on exit so it starts fresh next time
            }
        } else {
            timeSinceLastInput += deltaTime;

            if (timeSinceLastInput >= IDLE_TIME_LIMIT && !isScreensaverActive) {
                isScreensaverActive = true;

                HWND hwnd = (HWND)GetWindowHandle();
                ShowWindow(hwnd, SW_SHOW); 
                
                SetWindowState(FLAG_FULLSCREEN_MODE);
                HideCursor(); 

                // Reset position to generate a unique chaotic path sequence
                lorenz = { 0.1 + (find_random_int(1, 100) / 1000.0), 0.0, 0.0 };
                currentHue = (float)find_random_int(0, 360);
                systemColor = ColorFromHSV(currentHue, 0.9f, 0.9f);
            }
        }

        if (isScreensaverActive) {
            // Calculate multiple physics steps per frame to draw the line fast enough
            for (int step = 0; step < 4; step++) {
                // Lorenz System Equations
                double dx = SIGMA * (lorenz.y - lorenz.x) * dt;
                double dy = (lorenz.x * (RHO - lorenz.z) - lorenz.y) * dt;
                double dz = (lorenz.x * lorenz.y - BETA * lorenz.z) * dt;

                lorenz.x += dx;
                lorenz.y += dy;
                lorenz.z += dz;

                // Project 3D coordinate space onto your 2D display coordinates
                // We use X and Z dimensions to show the classic front-facing butterfly wings
                float screenX = (float)(lorenz.x * scale) + offsetX;
                float screenY = (float)(-lorenz.z * scale) + offsetY; // Inverted because screen Y goes down

                trail.push_back({ screenX, screenY });
            }

            // Recycle buffer if it hits maximum complexity
            if (trail.size() > MAX_TRAIL_POINTS) {
                trail.clear();
                currentHue = (float)find_random_int(0, 360); // Pick a new color motif
                systemColor = ColorFromHSV(currentHue, 0.9f, 0.9f);
            }

            BeginDrawing();
            ClearBackground(BLACK); 

            // Draw the continuous trail of chaos physics
            if (trail.size() > 1) {
                for (size_t i = 0; i < trail.size() - 1; i++) {
                    DrawLineV(trail[i], trail[i + 1], systemColor);
                }
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

