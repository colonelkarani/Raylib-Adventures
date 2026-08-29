#include <raylib.h>
#include <vector>
#include <iostream>

#include "ansi_colors.hpp"
#include "my_random_engine.cpp"

struct MyCircle
{
    Vector2 position;
    int radius;
    Color circle_color;
    int speed;
    int mass = find_random_int(1,40);
    int find_gravity()
    {
        return mass*10;
    };
    bool is_grounded()
    {
        if ((position.y+radius)>=600)
        {
            return true;
        }
        else
        {
            return false;
        }        
    }
};


using namespace std;
int main()
{
    //Initialize
     int height, width;
     height =600;
     width = 800;

    vector<MyCircle> items_on_screen;
    int CircleRadius = 40;

    Color CircleColor = GetRandomColor();

 //   bool DebugMode = true;
    


    InitWindow(width, height, "Fuck Being Average");

    while(!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

  //  int CircleSpeed = find_random_int(1,20);


         if (IsKeyPressed(KEY_R))
        {
            CircleColor = GetRandomColor();
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)|| IsMouseButtonDown(0))
        {
           items_on_screen.push_back({GetMousePosition(),CircleRadius,CircleColor});
        }
        if (IsKeyPressed(KEY_DOWN)&&CircleRadius>=5)
        {
            CircleRadius-=5;
        }
        if (IsKeyPressed(KEY_UP))
        {
            CircleRadius+=5;
        }
        DrawText("I guess I recreated paint",0,0,20,RAYWHITE);
        
        

        for (size_t i = 0; i < items_on_screen.size(); i++)
        {
            DrawCircle(items_on_screen[i].position.x, items_on_screen[i].position.y, items_on_screen[i].radius, items_on_screen[i].circle_color);
            if (!items_on_screen[i].is_grounded())
            {
                items_on_screen[i].position.y+=items_on_screen[i].find_gravity();
            }
            
        }
        


        EndDrawing();

    }
    CloseWindow();

}