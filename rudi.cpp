#include <raylib.h>
#include <string>
#include <vector>
#include <iostream>
#include "VKUtils.h"

using namespace std;

const int window_height = 600;
const int window_width  = 600;
int movement_velocity = 200;
const int player_limits_y = window_height/2;

int lives = 5;
int round = 1;
int enemy_speed = 70;
 
class Square
{
    public:
        Vector2 position;
        Vector2 velocity;
        int width;
        Color  square_color;
    Square(Vector2 position,  int width, Color color): position(position), width(width), square_color(color){}; 
    Rectangle GetRecFromSquare ()
    {
        return {position.x, position.y, width, width};
    }   
};




void  MovePlayerBox (Square &playerSquare)
{
    if (IsKeyDown(KEY_DOWN)&&playerSquare.position.y< (window_height - playerSquare.width))
    {
       playerSquare.position.y+= movement_velocity* GetFrameTime();
    }
    if (IsKeyDown(KEY_UP)&&playerSquare.position.y> player_limits_y)
    {
       playerSquare.position.y-= movement_velocity* GetFrameTime();
    }
    if (IsKeyDown(KEY_LEFT)&&playerSquare.position.x> (0))
    {
       playerSquare.position.x-= movement_velocity* GetFrameTime();
    }
    if (IsKeyDown(KEY_RIGHT)&&playerSquare.position.x < (window_width - playerSquare.width))
    {
       playerSquare.position.x+= movement_velocity* GetFrameTime();
    }
    
}


void RenderSquare(Square square)
{
    DrawRectangle(square.position.x, square.position.y, square.width, square.width, square.square_color);
}

void RenderScore()
{
    DrawText(TextFormat("Lives: %d", lives), 0, 0, 25, RED);
    DrawText(TextFormat("Round: %d", round), 0, 30, 25, RED);
}

void SpawnEnemies(vector<Square> &enemySqaures, float interval)
{
 
    static float start_time  = 0.0f;
    start_time+=GetFrameTime();
    if (start_time>=interval)
    {
        Square enemySquare({GetRandomInt(0, (window_width)-50), GetRandomInt(0, (window_height/2)-50)}, 50, GREEN); 
        enemySqaures.push_back(enemySquare);
     //  cout<<"ENEMY SPAWNED!!!"<<endl;
        start_time = 0.0f;
    }
}

void RenderEnemies(vector<Square> enemiesSquares)
{
    for (auto &&enemySquare : enemiesSquares)
    {
        RenderSquare(enemySquare);
    }
    
}

void UpdateEnemySquarePosition(vector<Square> &enemySquares)
{
    for (auto &&enemySquare : enemySquares)
    {
        enemySquare.position.y+=50* GetFrameTime();
    };
    for(auto it = enemySquares.begin();it!=enemySquares.end();)
    {
        it->position.y+= enemy_speed*GetFrameTime();
        if (it->position.y>=window_height)
        {
            it = enemySquares.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void UpdateEnemySpeed(int interval)
{
    static float start = 0;
    start += GetFrameTime();
    if (start>=interval )
    {
        round+=1;
        enemy_speed+=30;
        start = 0;
    }
    
}

void DetectPlayerEnemyCollision(Square &square, vector<Square> &enemy_squares)
{
for (auto &&enemySquare : enemy_squares)
{
    if(CheckCollisionRecs(square.GetRecFromSquare(), enemySquare.GetRecFromSquare()))
    {
        lives-=1;
        square.position.y+=square.width;
    }
}

}

void Update(Square &square, vector<Square> &enemy_squares)
{

    RenderSquare(square);
    MovePlayerBox(square);
    RenderScore();
    SpawnEnemies(enemy_squares, 1.0f);
    RenderEnemies(enemy_squares);
    UpdateEnemySquarePosition(enemy_squares);
    UpdateEnemySpeed(10);
    DetectPlayerEnemyCollision(square, enemy_squares);
}

int main()
{


    InitWindow(window_height, window_width, "Back to writing code");
    Square MySquare({500,500}, 50, BLUE);
    MySquare.position = {500,500};
    MySquare.width = 50;
    vector<Square> enemySquares;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Update(MySquare, enemySquares);
        EndDrawing();
    }
    CloseWindow();

    return 0;
}
