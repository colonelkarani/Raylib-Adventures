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
    Vector2 GetBoxCentre()
    {
        return (Vector2){position.x+width/2, position.y+width/2};
    }   
};

typedef enum 
{
GAME_ON,
PAUSED,
GAME_OVER,
GAME_WON,
MAIN_MENU
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

void DetectPlayerHealthCollision(Square &square, vector<Square> &enemy_squares, GameState &current_state)
{
    for (auto i = enemy_squares.begin(); i != enemy_squares.end();)
    {
        if(CheckCollisionRecs(square.GetRecFromSquare(), i->GetRecFromSquare()))
        {
            current_state = GameState::GAME_OVER;
            score-=1;
            i = enemy_squares.erase(i);
        }
        else
        {
            i++;
        }
    }
}



void ShootLineLeft(Square &square)
{
    if (IsKeyDown(KEY_A))
    {   
    DrawLine(square.GetBoxCentre().x, square.GetBoxCentre().y, square.GetBoxCentre().x-200, square.GetBoxCentre().y, ORANGE);
}
}

void UpdateGameState(GameState &state, vector<Square> &enemy_squares, vector<Square> &health_squares, float &game_timer)
{
 game_timer+=GetFrameTime();   
if (game_timer>=30)
{
    enemy_squares.clear();
    health_squares.clear();
    round = 1;

    state = GameState::GAME_OVER;  
}


if (score>=10)
{
    enemy_squares.clear();
    health_squares.clear();
    round = 1;

    state = GameState::GAME_WON;
}
}

void RenderWinText()
{
    DrawText("YOU WON", 250,250, 70, PURPLE);
}

void RenderLoseText()
{
    DrawText("YOU LOSE", 250,250, 70, RED);
}

bool IsMousePressedInRec(Rectangle rec)
{
    Vector2 mouse_position = GetMousePosition();

    if (IsMouseButtonPressed(0)&&CheckCollisionPointRec(mouse_position, rec))
        {
            return true;
        }
        else
        {
            return false;
        }        
}

void Restart(GameState &current_state)
{
    Rectangle rect = {0,0,50,50};
    DrawRectangleRec(rect, YELLOW);
    if (IsMousePressedInRec(rect))
    {
        score = 0;
        current_state = GameState::GAME_ON;
    }
    
}

void ChangeState(GameState &state, GameState state_to)
{
    state = state_to;
}

void RenderTimer(float timer)
{
DrawText(TextFormat("Time Left: %d", 30-int(timer)), 0, 50,30, BLUE);
}



void UpdateGamePlay(GameState &state, Square &square, vector<Square> &enemy_squares, vector<Square> &health_squares, float&enemy_timer, float &health_timer, float &game_timer)
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
    DetectPlayerHealthCollision(square, health_squares, state);
    PlayerSprint();
    UpdateGameState(state , enemy_squares, health_squares, game_timer);
    ShootLineLeft(square);
    RenderTimer(game_timer);
}



void UpdateWinMenu(GameState &current_state)
{
    RenderWinText();
    Restart(current_state);
}

void UpdateLoseMenu(GameState &current_state)
{
    RenderLoseText();
    Restart(current_state);
}

int main()
{
    InitWindow(window_height, window_width, "FUCK");
    Square MySquare({500,500}, 50, BLUE);
    MySquare.position = {500,500};
    MySquare.width = 50;
    vector<Square> enemySquares;
    vector<Square> healthSquares;
    GameState current_gamestate = GameState::GAME_ON;

    static float enemy_timer = 0.0f;
    static float health_timer = 0.0f;
    static float game_timer = 0.0f;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        switch (current_gamestate)
        {
        case GameState::GAME_ON:
        UpdateGamePlay(current_gamestate, MySquare, enemySquares, healthSquares, enemy_timer, health_timer, game_timer);
            break;
        case GameState::GAME_WON:
        UpdateWinMenu(current_gamestate);
        break;
        case GameState::GAME_OVER:
        UpdateLoseMenu(current_gamestate);
        break;
        
        default:
            break;
        }

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
