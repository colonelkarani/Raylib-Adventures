#include <raylib.h>
#include <cmath>
#include <string>
#include <vector>

#include "VKUtils.h"

#define WINDOW_HEIGHT 1000
#define WINDOW_WIDTH 1000

using namespace std;

class Bullet
{
    public:
        Vector2 Position;
        int size = 4;
        Color color;
        Bullet(Vector2 position, int size, Color color):Position(position), size(size), color(color){}
};

class Circle
{
    public:
        Vector2 position;
        int radius;
        Color color;
        int no_of_bullets;
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
        void CorrectDistance()
        {
            if (position.x > WINDOW_WIDTH )
            {
                position.x = WINDOW_WIDTH;
            }
            if (position.y > WINDOW_HEIGHT )
            {
                position.y = WINDOW_HEIGHT;
            }
            if (position.x<0)
            {
                position.x = 0;
            }
            if (position.y <0)
            {
                position.y = 0;
            }            
        }
};

Vector2 CalculateVectorBetweenTwoPoints(Vector2 startingPosition, Vector2 endPosition)
{
    return (Vector2){endPosition.x-startingPosition.x, endPosition.y - startingPosition.y};
}

double GetDistanceBetweenTwoVectors (Vector2 startingPosition, Vector2 endPosition)
{
    Vector2 vector_difference = CalculateVectorBetweenTwoPoints(startingPosition, endPosition);
return sqrt((vector_difference.x*vector_difference.x)+(vector_difference.y*vector_difference.y));
}

Vector2 operator/(Vector2 startingOperator, int denominator)
{
return(Vector2){startingOperator.x/denominator, startingOperator.y/denominator};
}

void Shoot(vector<Bullet> bullet_container)
{
if()
{

}
}


int main ()
{

    InitWindow(WINDOW_WIDTH,WINDOW_HEIGHT,"Hello");
    SetWindowPosition(0,0);

    vector<Bullet> bullet_container;

    Circle circle1({200,200}, 30, YELLOW);
    Circle circle2({400,400}, 20, GetRandomSolidColor());
    Circle circle3({400,700}, 30, GetRandomSolidColor());

    SetTargetFPS(60);


    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLUE);
        DrawCircle(circle1.position.x,circle1.position.y, circle1.radius, circle1.color);
        DrawCircle(circle2.position.x,circle2.position.y, circle2.radius, circle2.color);
        DrawCircle(circle3.position.x,circle3.position.y, circle3.radius, circle3.color);
        Vector2 CircleOneToCircleTwo = CalculateVectorBetweenTwoPoints(circle1.position, circle2.position);
        Vector2 CircleThreeToCircleTwo = CalculateVectorBetweenTwoPoints(circle3.position, circle2.position);

        int smoothness_factor= 20;

        int distance12 = int(GetDistanceBetweenTwoVectors(circle1.position, circle2.position));
        int distance32 = int(GetDistanceBetweenTwoVectors(circle3.position, circle2.position));
        
        circle1.CorrectDistance();
        circle2.CorrectDistance();
        circle3.CorrectDistance();
  
        if (distance12<distance32)
        {
        circle1.addvector(CircleOneToCircleTwo/smoothness_factor);
        }else{
        
        circle3.addvector(CircleThreeToCircleTwo/smoothness_factor);
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
        
        DrawText(TextFormat("Distance: %d", distance12), 0,0,20, BLACK);

        EndDrawing();
    }
    CloseWindow();
    
    return 0;
}