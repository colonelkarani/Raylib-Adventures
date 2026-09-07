#include <raylib.h>
#include "VKUtils.h"
#include <vector>

using namespace std;


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
    Texture2D piece_icon;
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

void ImageResizeProportional(Image* image, int max_width, int max_height) 
{
    // 1. Get original dimensions
    float original_width = (float)image->width;
    float original_height = (float)image->height;

    // 2. Calculate scaling ratios for both dimensions
    float ratio_w = (float)max_width / original_width;
    float ratio_h = (float)max_height / original_height;

    // 3. Choose the smaller ratio to ensure it fits entirely inside the bounding box
    float scale = (ratio_w < ratio_h) ? ratio_w : ratio_h;

    // 4. Calculate new unskewed dimensions
    int new_width = (int)(original_width * scale);
    int new_height = (int)(original_height * scale);

    // 5. Resize the actual image using your existing function
    ImageResize(image, new_width, new_height);
}

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
            //  else if (i == 0 || i )
            //  {
            //     /* code */
            //  }
             
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
    Image white_pawn_icon_image = LoadImage("pieces/pawn_white.png");
    Image black_pawn_icon_image = LoadImage("pieces/pawn_black.png");
    Image white_bishop_icon_image = LoadImage("pieces/bishop_white.png");
    Image black_bishop_icon_image = LoadImage("pieces/bishop_black.png");
    Image white_rook_icon_image = LoadImage("pieces/rook_white.png");
    Image black_rook_icon_image = LoadImage("pieces/rook_black.png");
    Image white_queen_icon_image = LoadImage("pieces/queen_white.png");
    Image black_queen_icon_image = LoadImage("pieces/queen_black.png");
    Image white_king_icon_image = LoadImage("pieces/king_white.png");
    Image black_king_icon_image = LoadImage("pieces/king_black.png");
    Image white_knight_icon_image = LoadImage("pieces/knight_white.png");
    Image black_knight_icon_image = LoadImage("pieces/knight_black.png");

    // ImageResize(&white_pawn_icon_image, cell_width , cell_height);
    ImageResizeProportional(&white_pawn_icon_image, cell_width, cell_height);
    ImageResizeProportional(&black_pawn_icon_image, cell_width, cell_height);
    ImageResizeProportional(&white_bishop_icon_image, cell_width, cell_height);
    ImageResizeProportional(&black_bishop_icon_image, cell_width, cell_height);
    ImageResizeProportional(&white_rook_icon_image, cell_width, cell_height);
    ImageResizeProportional(&black_rook_icon_image, cell_width, cell_height);
    ImageResizeProportional(&white_queen_icon_image, cell_width, cell_height);
    ImageResizeProportional(&black_queen_icon_image, cell_width, cell_height);
    ImageResizeProportional(&white_king_icon_image, cell_width, cell_height);
    ImageResizeProportional(&black_king_icon_image, cell_width, cell_height);
    ImageResizeProportional(&black_knight_icon_image, cell_width, cell_height);
    ImageResizeProportional(&white_knight_icon_image, cell_width, cell_height);


    Texture2D white_pawn_texture = LoadTextureFromImage(white_pawn_icon_image);
    Texture2D black_pawn_texture = LoadTextureFromImage(black_pawn_icon_image);
    Texture2D white_rook_texture = LoadTextureFromImage(white_rook_icon_image);
    Texture2D black_rook_texture = LoadTextureFromImage(black_rook_icon_image);
    Texture2D white_bishop_texture = LoadTextureFromImage(white_bishop_icon_image);
    Texture2D black_bishop_texture = LoadTextureFromImage(black_bishop_icon_image);
    Texture2D white_queen_texture = LoadTextureFromImage(white_queen_icon_image);
    Texture2D black_queen_texture = LoadTextureFromImage(black_queen_icon_image);
    Texture2D white_king_texture = LoadTextureFromImage(white_king_icon_image);
    Texture2D black_king_texture = LoadTextureFromImage(black_king_icon_image);
    Texture2D white_knight_texture = LoadTextureFromImage(white_knight_icon_image);
    Texture2D black_knight_texture = LoadTextureFromImage(black_knight_icon_image);

    UnloadImage(white_pawn_icon_image); 
    UnloadImage(black_pawn_icon_image); 
    UnloadImage(white_rook_icon_image); 
    UnloadImage(black_rook_icon_image); 
    UnloadImage(white_bishop_icon_image); 
    UnloadImage(black_bishop_icon_image); 
    UnloadImage(white_queen_icon_image); 
    UnloadImage(black_queen_icon_image); 
    UnloadImage(white_king_icon_image); 
    UnloadImage(black_king_icon_image); 
    UnloadImage(white_knight_icon_image); 
    UnloadImage(black_knight_icon_image); 

    //Main Update loop
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(background);

        DrawText("Successfully named the boxes", 0,0,18, WHITE);      
        

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
                DrawTexture(white_pawn_texture, cell.GetCentreX()-(white_pawn_texture.width/2), cell.GetCentreY()- (white_pawn_texture.height/2), WHITE);
            }
            if (cell.piece_type == ChessPieceType::PAWN&& cell.piece_color == ChessPieceColor::WHITE_PIECE)
            {
                DrawTexture(white_pawn_texture, cell.GetCentreX()-(white_pawn_texture.width/2), cell.GetCentreY()- (white_pawn_texture.height/2), WHITE);
            }
        DrawText(TextFormat("%c%d", cell.board_square_position_x, cell.board_square_position_y), cell.GetCentreX(), cell.GetCentreY(), 20, GREEN);
            
            
            
        }
        
        

        EndDrawing();
    }

    UnloadTexture(white_pawn_texture);
    UnloadTexture(black_pawn_texture);
    UnloadTexture(white_rook_texture);
    UnloadTexture(black_rook_texture);
    UnloadTexture(white_bishop_texture);
    UnloadTexture(black_bishop_texture);
    UnloadTexture(white_king_texture);
    UnloadTexture(black_king_texture);
    UnloadTexture(white_queen_texture);
    UnloadTexture(black_queen_texture);
    UnloadTexture(white_knight_texture);
    UnloadTexture(black_knight_texture);

    CloseWindow();
    
}