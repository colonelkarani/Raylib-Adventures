#include "raylib.h"

// --- THE ULTIMATE WINDOWS COLLISION SHIELD ---
// We temporarily rename CloseWindow and ShowCursor so windows.h defines its own versions 
// without overwriting or breaking Raylib's versions.
#define CloseWindow WindowsCloseWindow
#define ShowCursor WindowsShowCursor
#define Rectangle WindowsRectangle

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Now we safely undo those renames so our code can use Raylib's versions normally!
#undef CloseWindow
#undef ShowCursor
#undef Rectangle
// ---------------------------------------------

// Bring back your random engine!
#include "my_random_engine.cpp" 

struct RainDrop {
    int x;
    int y;
    int speed;
    Color color;
};

int main() {
    // Hide window from rendering initially on execution
    SetConfigFlags(FLAG_WINDOW_HIDDEN); 
    InitWindow(800, 600, "Matrix Screensaver System");

    int screenWidth = GetMonitorWidth(0);
    int screenHeight = GetMonitorHeight(0);
    SetWindowSize(screenWidth, screenHeight);

    const int TOTAL_DROPS = 120; 
    RainDrop rain[TOTAL_DROPS];

    for (int i = 0; i < TOTAL_DROPS; i++) {
        rain[i].x = find_random_int(0, screenWidth - 15);
        rain[i].y = find_random_int(-screenHeight, 0); 
        rain[i].speed = find_random_int(6, 14);
        float randomHue = (float)find_random_int(0, 360);
        rain[i].color = ColorFromHSV(randomHue, 0.9f, 0.9f); 
    }

    bool isScreensaverActive = false;
    float timeSinceLastInput = 0.0f;       
    const float IDLE_TIME_LIMIT = 5.0f; // Activates after 5 seconds of absolute silence

    POINT lastMousePos;
    GetCursorPos(&lastMousePos);

    int startupFrames = 0; 

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        POINT currentMousePos;
        GetCursorPos(&currentMousePos);

        bool inputDetected = false;

        // Skip input logging during the first 10 frames to avoid false startup triggers
        if (startupFrames > 10) {
            if (currentMousePos.x != lastMousePos.x || currentMousePos.y != lastMousePos.y) {
                inputDetected = true;
            }

            // Global background key hardware scan
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
                
                // FIXED BOTH FLAGS BELOW: Changed to Raylib's proper FLAG_FULLSCREEN_MODE
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
                
                // FIXED BOTH FLAGS BELOW: Changed to Raylib's proper FLAG_FULLSCREEN_MODE
                SetWindowState(FLAG_FULLSCREEN_MODE);
                HideCursor(); 
            }
        }

        if (isScreensaverActive) {
            for (int i = 0; i < TOTAL_DROPS; i++) {
                rain[i].y += rain[i].speed;

                if (rain[i].y > screenHeight) {
                    rain[i].y = find_random_int(-100, -20);
                    rain[i].x = find_random_int(0, screenWidth - 15);
                    rain[i].speed = find_random_int(6, 14);
                    float randomHue = (float)find_random_int(0, 360);
                    rain[i].color = ColorFromHSV(randomHue, 0.9f, 0.9f);
                }
            }

            BeginDrawing();
            ClearBackground(BLACK); 
            for (int i = 0; i < TOTAL_DROPS; i++) {
                DrawRectangle(rain[i].x, rain[i].y, 10, 25, rain[i].color);
            }
            EndDrawing();
        } else {
            // Keep background system process alive silently without rendering frames
            BeginDrawing();
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
