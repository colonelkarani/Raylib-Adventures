#include <iostream>
#include <vector>
#include "raylib.h"
#include "VKUtils.h"

using namespace std;

class Circle
{
public:
Vector2 position;
int radius;
Color color;
Circle(Vector2 position, int radius, Color color): position(position), radius(radius), color(color){}
};

int main ()
{
    InitWindow(1000,1000,"My fucking circles");
    cout<<"A random number is: "<<GetRandomInt(10,90)<<endl;
    Circle my_circle({100,100}, 30, YELLOW); 

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(GREEN);
        if (IsMouseButtonPressed(0))
        {
            DrawCircle(my_circle.position.x, my_circle.position.y, my_circle.radius, my_circle.color);
        }
        
        DrawText("Must code today", 0,0,20,BLACK);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}