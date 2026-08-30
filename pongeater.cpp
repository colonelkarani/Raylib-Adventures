#include <raylib.h>
#include "my_random_engine.cpp"
struct Pong
{
    Vector2 position;
    int radius;
    Color color;
    Vector2 velocity;
    bool isLeft;
};


using namespace std;
int main()
{
    InitWindow(500,500,"Fuck this place");

    Color BackgroundColor = GetRandomColor();
    while(!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BackgroundColor);


        DrawText("Lets now code the pong eater", 0,0,30, WHITE);

        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}