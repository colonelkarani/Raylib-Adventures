#include <raylib.h>
#include "my_random_engine.cpp"
#include <vector>

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



struct ChessSquare
{
    int position_y;
    int position_x;
    int size;
    Color color;
    int GetCentreX()
    {
        return position_x + size/2;
    };
    int GetCentreY()
    {
        return position_y + size/2;
    };
    bool IsSelected= false;
};

using namespace std;
int main ()
{
    int number_of_squares_in_x=8;
    int number_of_squares_in_y =8;

    int height, width;
    height  = 800;
    width = 800;
    Color background = GetRandomColor();

    int cell_width, cell_height;
    cell_width = width/number_of_squares_in_x;
    cell_height = width/number_of_squares_in_y;



    vector<ChessSquare> squares;

    int starting_position_x=0;
    int starting_position_y=0;
    for (int i = 0; i < number_of_squares_in_x; i++)
    {
        int square_x_position =starting_position_x +(i*cell_width);
        for (int j = 0; j < number_of_squares_in_y; j++)
        {
             int square_y_position =starting_position_y +(j*cell_height);
             Color CellColor;
             if ((i+j)%2)
             {
                CellColor = WHITE;
             }
             else
             {
                CellColor = GREEN;
             }
             
        squares.push_back({square_y_position, square_x_position, cell_width, CellColor});

        }
        
        //starting_position_x+=cell_width;
    }
    


    InitWindow(width, height, "Successfully made the board.");
    SetTargetFPS(60);

        Image iconImage = LoadImage("pawn2.png");
        ImageResize(&iconImage, cell_width -4, cell_height-4);

    Texture2D texture = LoadTextureFromImage(iconImage);


    //  Unload the raw image from RAM (No longer needed once on the GPU)
    UnloadImage(iconImage); 

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(background);
        DrawText("I'm trying to draw a grid", 0,0,18, WHITE);

       
        

        for (auto &&cell : squares)
        {
            DrawRectangle(cell.position_x, cell.position_y, cell.size, cell.size, cell.color);
            if (IsMousePressedInRec((Rectangle){cell.position_x, cell.position_y, cell.size, cell.size}))
            {
                cell.IsSelected = !cell.IsSelected;                
            }
            if (cell.IsSelected)
            {
            //DrawCircle(cell.GetCentreX(), cell.GetCentreY(), 7, background);
                DrawTexture(texture, cell.GetCentreX()-(texture.width/2), cell.GetCentreY()- (texture.height/2), WHITE);
            }
            
            
        }
        
        

        EndDrawing();
    }
    UnloadTexture(texture);
    CloseWindow();
    
}