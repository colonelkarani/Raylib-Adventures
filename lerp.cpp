#include "raylib.h"
#include "raymath.h"

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib - Vector2Lerp Example");

    // Define start and end targets
    Vector2 startPos = { 100.0f, 225.0f };
    Vector2 endPos = { 700.0f, 225.0f };
    
    // Animation state
    float t = 0.0f;          // Interpolation factor (0.0 to 1.0)
    float speed = 0.5f;      // Complete movement in 2 seconds (1/0.5 = 2.0s)
    bool movingToEnd = true; // Movement direction flag

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // --- UPDATE ---
        float deltaTime = GetFrameTime();

        // Progress or reverse t based on current direction
        if (movingToEnd) {
            t += speed * deltaTime;
            if (t >= 1.0f) {
                t = 1.0f;
                movingToEnd = false; // Reverse direction
            }
        } else {
            t -= speed * deltaTime;
            if (t <= 0.0f) {
                t = 0.0f;
                movingToEnd = true; // Reverse direction
            }
        }

        // Calculate the current position along the line using Vector2Lerp
        Vector2 currentPos = Vector2Lerp(startPos, endPos, t);

        // --- DRAW ---
        BeginDrawing();
            ClearBackground(RAYWHITE);

            // Draw line connecting the start and end points
            DrawLineV(startPos, endPos, LIGHTGRAY);

            // Draw target markers
            DrawCircleV(startPos, 12.0f, DARKBLUE);
            DrawCircleV(endPos, 12.0f, DARKBLUE);

            // Draw the interpolated moving circle
            DrawCircleV(currentPos, 24.0f, MAROON);

            // Display UI overlay
            DrawText("Vector2Lerp Animation", 10, 10, 20, DARKGRAY);
            DrawText(TextFormat("Interpolation Factor (t): %.2f", t), 10, 40, 20, GRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
