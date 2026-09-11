#include "raylib.h"
#include "VKUtils.h"

class Circle
{
    public:
        Vector2 position;
        int radius;
        Color color;
        Circle(Vector2 position, int radius ,Color color):position(position), radius(radius), color(color)
        {

        }
        void addvector (Vector2 vector)
        {
            float dt = GetFrameTime();
            float speed = 5.0;

            position.x+= vector.x+speed*dt;
            position.y+= vector.y+speed*dt;
        }
};

Vector2 CalculateVectorBetweenTwoPoints(Vector2 startingPosition, Vector2 endPosition)
{
    return (Vector2){endPosition.x-startingPosition.x, endPosition.y - startingPosition.y};
}


int main ()
{
    InitWindow(1000,1000,"Hello");
    SetWindowPosition(0,0);
    Circle circle1({200,200}, 30, YELLOW);
    Circle circle2({400,400}, 20, GetRandomColor());

    SetTargetFPS(60);


    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLUE);
        DrawCircle(circle1.position.x,circle1.position.y, circle1.radius, circle1.color);
        DrawCircle(circle2.position.x,circle2.position.y, circle2.radius, circle2.color);
        Vector2 CircleOneToCircleTwo = CalculateVectorBetweenTwoPoints(circle1.position, circle2.position);

        
        if (IsMouseButtonDown(0))
        {
            circle1.addvector(CircleOneToCircleTwo);
            /* code */
        }
        if (IsKeyDown(KEY_DOWN))
        {
            circle2.addvector({0,50});
        }
        if (IsKeyDown(KEY_UP))
        {
            circle2.addvector({0,-50});
        }
        if (IsKeyDown(KEY_RIGHT))
        {
            circle2.addvector({50,0});
        }
        if (IsKeyDown(KEY_LEFT))
        {
            circle2.addvector({-50,0});
        }
        
        EndDrawing();
    }
    CloseWindow();
    
    return 0;
}