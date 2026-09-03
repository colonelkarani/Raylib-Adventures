#include <raylib.h>
#include "VKUtils.h"
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

enum ChessPieceType
{
EMPTY,
PAWN,
ROOK,
BISHOP,
KNIGHT,
QUEEN,
KING
};

enum ChessPieceColor
{
NONE,
WHITE_PIECE,
BLACK_PIECE
};


struct ChessSquare
{
    int position_y;
    int position_x;
    int size;
    char board_square_position_x;
    int board_square_position_y;
    Color color;
    ChessPieceType piece_type = ChessPieceType::EMPTY;
    ChessPieceColor piece_color = ChessPieceColor::NONE;
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
             ChessPieceType piece_type;
             if ((i+j)%2)
             {
                CellColor = WHITE;
             }
             else
             {
                CellColor = BLACK;
             }
             if (j==1||j==number_of_squares_in_y-2)
             {
               piece_type= ChessPieceType::PAWN;
             }
             else
             {
                piece_type = ChessPieceType::EMPTY;
             }
            char position_x= 'n';
            switch (i)
            {
            case 0:
                position_x = 'a';
                break;
            case 1:
                position_x = 'b';
                break;
            case 2:
                position_x = 'c';
                break;
            case 3:
                position_x = 'd';
                break;
            case 4:
                position_x='e';
                break;
            case 5:
                position_x='f';
                break;
            case 6:
                position_x='g';
                break;
            case 7:
                position_x='h';
                break;
            
            default:
            position_x  = 'n';
                break;
            }
            int position_y = 8-j;
             
        ChessSquare square = (ChessSquare){square_y_position, square_x_position, cell_width,position_x,position_y,CellColor, piece_type};
             
        squares.push_back(square);
        }
    }
    


    InitWindow(width, height, "Successfully made the board.");
    SetTargetFPS(60);

    // Texture Loading
    Image iconImage = LoadImage("pawn2.png");
    ImageResize(&iconImage, cell_width -4, cell_height-4);
    Texture2D texture = LoadTextureFromImage(iconImage);
    UnloadImage(iconImage); 

    //Main Update loop
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(background);

        DrawText("I'm trying to draw a grid", 0,0,18, WHITE);      
        

        for (auto &&cell : squares)
        {
            DrawRectangle(cell.position_x, cell.position_y, cell.size, cell.size, cell.color);
            if (IsMousePressedInRec((Rectangle){float(cell.position_x), float(cell.position_y), float(cell.size), float(cell.size)}))
            {
                cell.IsSelected = !cell.IsSelected;                
            }
            if (cell.IsSelected)
            {
            //DrawCircle(cell.GetCentreX(), cell.GetCentreY(), 7, background);
                DrawTexture(texture, cell.GetCentreX()-(texture.width/2), cell.GetCentreY()- (texture.height/2), WHITE);
            }
            if (cell.piece_type == ChessPieceType::PAWN)
            {
                DrawTexture(texture, cell.GetCentreX()-(texture.width/2), cell.GetCentreY()- (texture.height/2), WHITE);
            }
        DrawText(TextFormat("%c%d", cell.board_square_position_x, cell.board_square_position_y), cell.GetCentreX(), cell.GetCentreY(), 20, GREEN);
            
            
            
        }
        
        

        EndDrawing();
    }
    UnloadTexture(texture);
    CloseWindow();
    
}