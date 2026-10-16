#include <raylib.h>

int main()
{
    const int window_height = 300;
    const int window_width  = 300;

    InitWindow(window_height, window_width, "Back to writing code");

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        EndDrawing();
    }
    CloseWindow();

    return 0;
}
