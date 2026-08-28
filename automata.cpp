#include "raylib.h"

// --- THE ULTIMATE WINDOWS COLLISION SHIELD ---
// We temporarily rename conflicting functions so windows.h defines its own versions 
// without overwriting or breaking Raylib's versions.
#define CloseWindow WindowsCloseWindow
#define ShowCursor WindowsShowCursor
#define Rectangle WindowsRectangle
#define DrawText WindowsDrawText       // <-- ADDED THIS TO FIX THE CONVERT ERROR

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Now we safely undo those renames so our code can use Raylib's versions normally!
#undef CloseWindow
#undef ShowCursor
#undef Rectangle
#undef DrawText                  // <-- ADDED THIS TO RESTORE RAYLIB'S DRAWTEXT
// ---------------------------------------------

#include "my_random_engine.cpp"
#include <cmath>
#include <vector>

// Grid configuration (Keep sizes proportional for matrix math performance)
const int GRID_WIDTH = 200;
const int GRID_HEIGHT = 150;
const int CELL_SIZE = 6; // Scales up to fit screen nicely

// Lenia Physics Parameters (The "DNA" of the emergent creatures)
const float DT = 0.6f;         // Time execution step
const float MU = 0.15f;        // Growth center peak 
const float SIGMA = 0.017f;    // Growth breadth variance
const int KERNEL_R = 14;       // Radius of the molecular interaction ring

// Growth mapping function (Gaussian bell curve)
float GrowthFunction(float localSum) {
    float diff = localSum - MU;
    return 2.0f * std::exp(- (diff * diff) / (2.0f * SIGMA * SIGMA)) - 1.0f;
}

int main() {
    // FORCE DISPLAY IMMEDIATE: Removed FLAG_WINDOW_HIDDEN so you can see it load instantly!
    InitWindow(GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, "Quantum Lenia: Continuous Emergence Engine");

    // Two-dimensional dynamic fields for double-buffered matrix state transitions
    std::vector<std::vector<float>> currentGrid(GRID_WIDTH, std::vector<float>(GRID_HEIGHT, 0.0f));
    std::vector<std::vector<float>> nextGrid(GRID_WIDTH, std::vector<float>(GRID_HEIGHT, 0.0f));

    // Precalculate the Kernel Shell structure to prevent expensive square roots every frame
    struct KernelNode { int dx, dy; float weight; };
    std::vector<KernelNode> kernel;
    float kernelWeightSum = 0.0f;

    for (int dx = -KERNEL_R; dx <= KERNEL_R; dx++) {
        for (int dy = -KERNEL_R; dy <= KERNEL_R; dy++) {
            float dist = std::sqrt(dx * dx + dy * dy) / KERNEL_R;
            // Generate a concentric ring-like distribution kernel (creates cellular membrane structures)
            if (dist > 0.0f && dist <= 1.0f) {
                float alpha = 0.5f;
                float weight = std::exp(-std::pow(dist - alpha, 2.0f) / 0.06f);
                kernel.push_back({dx, dy, weight});
                kernelWeightSum += weight;
            }
        }
    }
    // Normalize physics kernel fields
    for (auto& node : kernel) {
        node.weight /= kernelWeightSum;
    }

    // Seed the simulation with dense patches of organic "stem cell" matter
    for (int i = 0; i < 8; i++) {
        int centerX = find_random_int(KERNEL_R * 2, GRID_WIDTH - KERNEL_R * 2);
        int centerY = find_random_int(KERNEL_R * 2, GRID_HEIGHT - KERNEL_R * 2);
        for (int x = -15; x <= 15; x++) {
            for (int y = -15; y <= 15; y++) {
                if (find_random_int(0, 100) > 40) {
                    currentGrid[centerX + x][centerY + y] = (float)find_random_int(20, 100) / 100.0f;
                }
            }
        }
    }

    // Forced active state so it runs without hiding
    bool isScreensaverActive = true; 
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        
        // Fast spacebar hotkey step to instantly drop a fresh chaotic spore cluster if fields collapse
        if (IsKeyPressed(KEY_SPACE)) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                for (int y = 0; y < GRID_HEIGHT; y++) {
                    if (find_random_int(0, 1000) > 985) currentGrid[x][y] = 1.0f;
                }
            }
        }

        if (isScreensaverActive) {
            // Compute Continuous Space-Time Physics Matrix updates
            for (int x = 0; x < GRID_WIDTH; x++) {
                for (int y = 0; y < GRID_HEIGHT; y++) {
                    
                    float localSum = 0.0f;
                    
                    // Convolve localized space coordinates across the Normalized Kernel
                    for (const auto& node : kernel) {
                        // Toroidal Topology wrapping (screws top to bottom, left to right perfectly)
                        int nx = (x + node.dx + GRID_WIDTH) % GRID_WIDTH;
                        int ny = (y + node.dy + GRID_HEIGHT) % GRID_HEIGHT;
                        localSum += currentGrid[nx][ny] * node.weight;
                    }

                    // Apply continuous fractional time integrations
                    float growth = GrowthFunction(localSum);
                    float newValue = currentGrid[x][y] + DT * growth;
                    
                    // Physical boundary clamping
                    if (newValue < 0.0f) newValue = 0.0f;
                    if (newValue > 1.0f) newValue = 1.0f;
                    
                    nextGrid[x][y] = newValue;
                }
            }
            
            // Swap double buffers
            currentGrid = nextGrid;

            // Render execution block
            BeginDrawing();
            ClearBackground(BLACK);

            for (int x = 0; x < GRID_WIDTH; x++) {
                for (int y = 0; y < GRID_HEIGHT; y++) {
                    float val = currentGrid[x][y];
                    if (val > 0.02f) {
                        // High complexity procedural coloring maps molecular density to custom light waves
                        unsigned char r = (unsigned char)(val * 40.0f);
                        unsigned char g = (unsigned char)(val * 210.0f);
                        unsigned char b = (unsigned char)(val * 140.0f) + (unsigned char)((1.0f - val) * 30.0f);
                        
                        DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, (Color){r, g, b, 255});
                    }
                }
            }

            RAYLIB_H::DrawText("QUANTUM LENIA ACTIVE - PRESS SPACE TO INJECT SPORES", 10, 10, 12, DARKGRAY);
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
