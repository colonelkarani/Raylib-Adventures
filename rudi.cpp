#include <raylib.h>
#include <string>
#include <vector>
#include <iostream>
#include "VKUtils.h"

using namespace std;

const int window_height = 600;
const int window_width  = 600;
int movement_velocity = 200;
const int player_limits_y = 0;

int score = 0;
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

typedef enum 
{

} GameState;


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
    DrawText(TextFormat("Score: %d", score), 0, 0, 25, RED);
    DrawText(TextFormat("Round: %d", round), 0, 30, 25, RED);
}

void SpawnSquares(vector<Square> &enemySqaures, float interval, Color color, bool debug, float &start_time)
{
    start_time+=GetFrameTime();
    if (start_time>=interval)
    {
        Square enemySquare({GetRandomInt(0, (window_width)-50), GetRandomInt(0, (window_height/2)-50)}, 50, color); 
        enemySqaures.push_back(enemySquare);
        if (debug)
        {
            cout<<"SQUARE SPAWNED !!"<<endl;
        }
        
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

void UpdateHealthSquarePosition(vector<Square> &enemySquares)
{
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
    for (auto i = enemy_squares.begin(); i != enemy_squares.end();)
    {
        if(CheckCollisionRecs(square.GetRecFromSquare(), i->GetRecFromSquare()))
        {
            score+=1;
            i = enemy_squares.erase(i);
        }
        else
        {
            i++;
        }
    }
}

void DetectPlayerHealthCollision(Square &square, vector<Square> &enemy_squares)
{
    for (auto i = enemy_squares.begin(); i != enemy_squares.end();)
    {
        if(CheckCollisionRecs(square.GetRecFromSquare(), i->GetRecFromSquare()))
        {
            score-=1;
            i = enemy_squares.erase(i);
        }
        else
        {
            i++;
        }
    }
}

void PlayerSprint()
{
    if (IsKeyDown(KEY_LEFT_SHIFT))
    {
        movement_velocity = 500;
    }
    else
    {
       movement_velocity = 200;
    } 
}

void Update(Square &square, vector<Square> &enemy_squares, vector<Square> &health_squares, float&enemy_timer, float &health_timer)
{

    RenderSquare(square);
    MovePlayerBox(square);
    RenderScore();
    SpawnSquares(enemy_squares, 1.0f, GREEN, false, enemy_timer);
    SpawnSquares(health_squares, 10.0f, RED, false, health_timer);
    RenderEnemies(enemy_squares);
    RenderEnemies(health_squares);
    UpdateEnemySquarePosition(enemy_squares);
    UpdateHealthSquarePosition(health_squares);
    UpdateEnemySpeed(10);
    DetectPlayerEnemyCollision(square, enemy_squares);
    DetectPlayerHealthCollision(square, health_squares);
    PlayerSprint();
}

int main()
{
    InitWindow(window_height, window_width, "Back to writing code");
    Square MySquare({500,500}, 50, BLUE);
    MySquare.position = {500,500};
    MySquare.width = 50;
    vector<Square> enemySquares;
    vector<Square> healthSquares;

    static float enemy_timer = 0.0f;
    static float health_timer = 0.0f;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Update(MySquare, enemySquares, healthSquares, enemy_timer, health_timer);
        EndDrawing();
    }
    CloseWindow();

    return 0;
}
