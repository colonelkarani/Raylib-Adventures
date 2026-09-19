#include <iostream>
#include "raylib.h"
#include "ansi_colors.hpp"

using namespace std;

class Box
{
public:
    Vector2 position;
    int height;
    int width;
    Color color;
    bool is_being_dragged = false;
    void CorrectBoxPosition()
    {
        if(is_being_dragged)
        {
        position.x +=GetMouseDelta().x;
        position.y +=GetMouseDelta().y;
        }
    }
    Rectangle GetRecFromBox(){return (Rectangle){float(position.x), float(position.y), float(width), float(height)};};
    Box(Vector2 position, int height, int width, Color color): position(position), height(height), width(width), color(color){};
};

bool CheckIfMouseIsInRect(Rectangle rect)
{
    if (IsMouseButtonDown(0)&&CheckCollisionPointRec(GetMousePosition() ,rect))
    {
        return true;
    }else
    {
        return false;
    }
    
}

void DragBox(Box &box)
{
    if (CheckIfMouseIsInRect(box.GetRecFromBox()))
    {
        box.is_being_dragged = true;
    }else
    {
        box.is_being_dragged = false;
    }
    
    
}

int main() {
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 450;

    Box my_box((Vector2){screenWidth/2,screenHeight/2}, 40,40,YELLOW);

    InitWindow(screenWidth, screenHeight, "Trying drag and drop");

    // Set target frames per second
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose()) {  // Detect window close button or ESC key
        
        // Update
        // TODO: Update your variables here
        
        // Draw
        BeginDrawing();
            ClearBackground(BLACK);

            DrawRectangle(my_box.position.x, my_box.position.y, my_box.width, my_box.height, my_box.color);
            DragBox(my_box);
            my_box.CorrectBoxPosition();
        EndDrawing();
    }

    // De-Initialization
    CloseWindow();  // Close window and OpenGL context

    return 0;
}
