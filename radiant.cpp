#include <raylib.h>
#include <vector>
#include <iostream>
#include <format>

#include "ansi_colors.hpp"
#include "my_random_engine.cpp"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

struct MyCircle
{
    Vector2 position;
    int radius;
    Color circle_color;
    int speed;
    int mass = find_random_int(1,4);
    Vector2 velocity = (Vector2){1,1};
    int find_gravity()
    {
        return mass*10;
    };
    bool is_grounded()
    {
        if ((position.y+radius)>=SCREEN_HEIGHT)
        {
            return true;
        }
        else
        {
            return false;
        }        
    };
    bool is_highest()
    {
        if ((position.y+radius)<=0)
        {
            return true;
        }
        else
        {
            return false;
        }
    };
    bool is_leftest()
    {
        if ((position.x-radius)<=0)
        {
            return true;
        }
        else
        {
            return false;
        }
    };
    bool is_rightest()
    {
        if ((position.x+radius)>=SCREEN_WIDTH)
        {
            return true;
        }
        else
        {
            return false;
        }
    };
};


using namespace std;
int main()
{
    //Initialize
    //  int height, width;
    //  height =600;
    //  width = 800;

    vector<MyCircle> items_on_screen;
    int CircleRadius = 40;

    Color CircleColor = GetRandomColor();

 //   bool DebugMode = true;

 //Rectangle BoundingRect  = (Rectangle){0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
    


    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fuck Being Average");

    while(!WindowShouldClose())
    {
    // Rendering
        BeginDrawing();
        ClearBackground(BLACK);
        int fps = GetFPS();
        int no_of_particles = items_on_screen.size();

  //  int CircleSpeed = find_random_int(1,20);

        // Randomize color
         if (IsKeyPressed(KEY_R))
        {
            CircleColor = GetRandomColor();
        }

        //Clear Circles
        if (IsKeyPressed(KEY_C))
        {
            items_on_screen.clear();
        }

        //Creating the circles
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
           items_on_screen.push_back({GetMousePosition(),CircleRadius,CircleColor});
        }

        //Changing the circles size
        if (IsKeyPressed(KEY_DOWN)||IsKeyDown(KEY_DOWN)&&CircleRadius>=5)
        {
            CircleRadius-=5;
        }
        if (IsKeyPressed(KEY_UP))
        {
            CircleRadius+=5;
        }
        

        
        

        for (size_t i = 0; i < items_on_screen.size(); i++)
        {
            DrawCircle(items_on_screen[i].position.x, items_on_screen[i].position.y, items_on_screen[i].radius, items_on_screen[i].circle_color);
            // if (!items_on_screen[i].is_grounded())
            // {
            //     items_on_screen[i].position.y+=items_on_screen[i].find_gravity();
            // }
            items_on_screen[i].position.y+=(items_on_screen[i].velocity.y);
            items_on_screen[i].position.x+=items_on_screen[i].velocity.x;
            if (items_on_screen[i].is_grounded()||items_on_screen[i].is_highest())
            {
                items_on_screen[i].velocity.y = -items_on_screen[i].velocity.y;
            }
            if (items_on_screen[i].is_rightest()||items_on_screen[i].is_leftest())
            {
                items_on_screen[i].velocity.x = -items_on_screen[i].velocity.x;
            }          
        }        


        
        DrawText(TextFormat("Radius  = %d", CircleRadius),0,0,20,RAYWHITE);
        DrawText(TextFormat("Color", CircleRadius),0,25,20,RAYWHITE);
        DrawRectangle(80,25, 20,20, CircleColor);
        DrawText(TextFormat("Particles: %d", no_of_particles),0,50,20,RAYWHITE);
        DrawText(TextFormat("FPS: %d", fps),0,75,20,RAYWHITE);

       // DrawRectangleLinesEx(BoundingRect, 10.0 , CircleColor);

        EndDrawing();

    }
    CloseWindow();

}