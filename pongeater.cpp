#include <raylib.h>
#include "my_random_engine.cpp"
#include <vector>
#include <cmath>

#define SCREEN_HEIGHT 1000

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



enum class Teams : char
{
    NONE,
    TEAM_A,
    TEAM_B
};

struct PongBall
{
    Vector2 position;
    int radius=15;
    Vector2 velocity = {3,2};
    Teams team = Teams::NONE;
    Color GetPongTeamColor()
    {
        switch (team)
        {
        case Teams::NONE :
            return WHITE;
            break;
        case Teams::TEAM_A :
            return BLUE;
            break;
        case Teams::TEAM_B :
            return RED;
            break;
        default: return GREEN;
        }
    };
    int GetContainingSquareIndex(int size_of_a_single_square,int total_length_of_grid )
    {
           int column,row,number_of_rows;
           column = int(trunc(position.x /size_of_a_single_square));
           row = int(trunc(position.y/size_of_a_single_square));
           number_of_rows = total_length_of_grid/size_of_a_single_square;
            return ((column*number_of_rows)+row);
    }
};

struct GridSquare
{
    int position_y;
    int position_x;
    int size;
    int GetCentreX()
    {
        return position_x + size/2;
    };
    int GetCentreY()
    {
        return position_y + size/2;
    };
    Rectangle GetRecFromGridSquare()
    {
        return (Rectangle){float(position_x), float(position_y), float(size), float(size)};
    };
    Teams team = Teams::NONE;
    Color GetGridTeamColor()
    {
        switch (team)
        {
        case Teams::NONE :
            return BLACK;
            break;
        case Teams::TEAM_A :
            return RED;
            break;
        case Teams::TEAM_B :
            return BLUE;
            break;
        default: return GREEN;
        }
    };
    void GetEatenAndSwitchTeam(Teams new_team)
    {
              team = new_team;
    };
    bool IsSelected= false;
};



void RenderPongBall(PongBall ball)
{
    DrawCircleV(ball.position, ball.radius, ball.GetPongTeamColor());
}

void UpdatePongBallsPosition(PongBall& ball)
{
    ball.position.x+=ball.velocity.x;
    ball.position.y+=ball.velocity.y;
}

void UpdatePongBallsVelocity(PongBall& ball, GridSquare& square)
{
    if (ball.team != square.team)
    {
        //ball.velocity.x = -ball.velocity.x;
        //ball.velocity.y = -ball.velocity.y;
        int previous_ball_x_position =ball.position.x-ball.velocity.x;
        int previous_ball_y_position =ball.position.y-ball.velocity.y;
        Vector2 square_limits_x, square_limits_y;
        square_limits_x = (Vector2){square.position_x, square.position_x+square.size};
        square_limits_y = (Vector2){square.position_y, square.position_y+square.size};

        if (square_limits_x.x>previous_ball_x_position||square_limits_x.y<previous_ball_x_position)
        {
            ball.velocity.x = -ball.velocity.x;
        square.GetEatenAndSwitchTeam(ball.team);

        }else
        if (square_limits_y.x>previous_ball_y_position||square_limits_y.y<previous_ball_y_position)
        {
            ball.velocity.y = -ball.velocity.y;
        square.GetEatenAndSwitchTeam(ball.team);

        }else
        if ((square_limits_x.x>previous_ball_x_position||square_limits_x.y<previous_ball_x_position)&& (square_limits_y.x>previous_ball_y_position||square_limits_y.y<previous_ball_y_position))
        {
            ball.velocity.x = -ball.velocity.x;
            ball.velocity.y = -ball.velocity.y;
        square.GetEatenAndSwitchTeam(ball.team);

        }
        
        

    }
    if ((ball.position.x-ball.radius) <=0 || (ball.position.x+ball.radius)>=SCREEN_HEIGHT)
    {
        ball.velocity.x=-ball.velocity.x;
    }
    if ( (ball.position.y-ball.radius) <=0 || (ball.position.y+ball.radius)>=SCREEN_HEIGHT)
    {
        ball.velocity.y=-ball.velocity.y;
    }
    
    
}

using namespace std;

int main ()
{
  


    int number_of_squares_in_x=15;
    int number_of_squares_in_y =15;


   Color background_color = GetRandomSolidColor();

    int cell_width, cell_height;
    cell_width = SCREEN_HEIGHT/number_of_squares_in_x;
    cell_height = SCREEN_HEIGHT/number_of_squares_in_y;



    vector<GridSquare> squares;
    vector<PongBall> balls;

    int starting_position_x=0;
    int starting_position_y=0;

    for (int i = 0; i < number_of_squares_in_x; i++)
    {
        int square_x_position =starting_position_x +(i*cell_width);
        for (int j = 0; j < number_of_squares_in_y; j++)
        {
             int square_y_position =starting_position_y +(j*cell_height);
             Teams team;
             if (i<(number_of_squares_in_x/2))
             {
                team = Teams::TEAM_A;
             }
             else
             {
                team = Teams::TEAM_B;
             }
             
             
             
        squares.push_back({square_y_position, square_x_position, cell_width, team});
        }
    }

    PongBall ball_1, ball_2 ;
    ball_1.position = (Vector2){200, 400};
    ball_2.position = (Vector2){600, 400};
    ball_1.team = Teams::TEAM_A;
    ball_2.team = Teams::TEAM_B;

    balls.push_back(ball_1);
    balls.push_back(ball_2);
    


    InitWindow(SCREEN_HEIGHT, SCREEN_HEIGHT, "Successfully made the board.");
  //  SetTargetFPS(60);


    //Main Update loop
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

       // DrawText("I'm trying to draw a grid", 0,0,18, WHITE);      
        
     


        for (auto &&cell : squares)
        {
           DrawRectangle(cell.position_x, cell.position_y, cell.size, cell.size, cell.GetGridTeamColor());
            //DrawRectangleLinesEx(cell.GetRecFromGridSquare(), 2, GREEN);

            if (IsMousePressedInRec((Rectangle){float(cell.position_x), float(cell.position_y), float(cell.size), float(cell.size)}))
            {
                cell.IsSelected = !cell.IsSelected;                
            }
            if (cell.IsSelected)
            {
            DrawCircle(cell.GetCentreX(), cell.GetCentreY(), 7, background_color);
            } 
        }

        for (auto &&ball : balls)
        {
            RenderPongBall(ball);
            int cell_index = ball.GetContainingSquareIndex(cell_width, cell_width*number_of_squares_in_x);
            UpdatePongBallsVelocity(ball, squares[cell_index]);
            UpdatePongBallsPosition(ball);


        }
        

        
        DrawText(TextFormat("FPS: %d",GetFPS()),0,0,20, WHITE);


        EndDrawing();
    }
    CloseWindow();
    
}