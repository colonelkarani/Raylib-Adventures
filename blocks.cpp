#include <raylib.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "VKUtils.h"
#include "ansi_colors.hpp"



using namespace std;


struct GridSquare
{
    Vector2 position;
    float size;
    Color square_color;
    bool is_filled  = false;
    int GetCentreX()
    {
        return position.x + size/2;
    };
    int GetCentreY()
    {
        return position.y + size/2;
    };
    Rectangle GetRecFromGridSquare()
    {
        return (Rectangle){float(position.x), float(position.y), float(size), float(size)};
    };
};

struct SquareBlock
{
    vector<GridSquare> containing_squares;
    Color block_color;
};


void RenderSquares(GridSquare square)
{
    // DrawRectangle(square.position.x, square.position.y, square.size, square.size, square.square_color);
    DrawRectangleLines(square.position.x, square.position.y, square.size, square.size, WHITE);
}
void RenderSquaresBlue(GridSquare square)
{
    DrawRectangle(square.position.x, square.position.y, square.size, square.size, BLUE);
}

void CreateGridInRect(Rectangle rec, int no_of_squares_x, int no_of_squares_y, vector<GridSquare>& squareStorage)
{
    float cell_width, cell_height;
    cell_width = rec.width/no_of_squares_x;
    cell_height = rec.height/no_of_squares_y;

    float cell_size = (cell_width<cell_height)?cell_width:cell_height;

    for (int i = 0; i < no_of_squares_x; i++)
    {
        float square_x_position =rec.x +(i*cell_size);
        for (int j = 0; j < no_of_squares_y; j++)
        {
             float square_y_position =rec.y +(j*cell_size);             
             
        squareStorage.push_back({(Vector2){float(square_x_position), float(square_y_position)}, cell_width, GetRandomColor()});
        }
    }
}



void DebugMousePosition()
{
    Vector2 mouse_position= GetMousePosition();
    if (IsMouseButtonPressed(0))
    {
        cout<<ANSI_YELLOW <<"Position X: "<<mouse_position.x<<" Position Y: "<<mouse_position.y<<ANSI_RESET<<endl;
    }    
}

void DebugSquareIndices(GridSquare square, int index)
{
    DrawText(TextFormat("%d", index), square.GetCentreX(), square.GetCentreY(), 15, WHITE);
}


//google ai wrote this function
vector<int> GetAdjacentSquareIndices(int index, int no_of_squares_x, int no_of_squares_y)
{
    vector<int> adjacentIndices;

    // 1. Convert the flat index back into 2D grid coordinates
    int current_row = index / no_of_squares_x;
    int current_col = index % no_of_squares_x;

    // 2. Define the 4 cardinal direction steps: {row_offset, col_offset}
    // Up, Down, Left, Right
    int rowOffsets[] = { -1, 1,  0, 0 };
    int colOffsets[] = {  0, 0, -1, 1 };

    // 3. Check each of the 4 neighbors
    for (int i = 0; i < 4; i++)
    {
        int neighbor_row = current_row + rowOffsets[i];
        int neighbor_col = current_col + colOffsets[i];

        // 4. Ensure the neighbor is within the valid bounds of the grid
        if (neighbor_row >= 0 && neighbor_row < no_of_squares_y &&
            neighbor_col >= 0 && neighbor_col < no_of_squares_x)
        {
            // 5. Convert 2D coordinates back into a valid flat vector index
            int neighbor_flat_index = (neighbor_row * no_of_squares_x) + neighbor_col;
            adjacentIndices.push_back(neighbor_flat_index);
        }
    }

    return adjacentIndices;
}


// vector<int> GetRandomBlockIndices()
//  {
//     int no_of_times_to_loop = GetRandomInt(0,5);
//     cout<<"Looping "<<no_of_times_to_loop<<" times"<<endl;
//     vector<int> block_indices;
//     for (int i = 0; i < no_of_times_to_loop; i++)
//     {
//         int index = GetRandomInt(0,5);
//         vector<int> adjucent_squares = GetAdjacentSquareIndices(index, 3, 3);
//         if(find(block_indices.begin(), block_indices.end(), index)!=block_indices.end())
//         {
//             i-=1;
//         }else
//         {
//             block_indices.push_back(index);
//         }
//     }
//     return block_indices;
// }

vector<int> GetRandomBlockIndices()
{
    // 1. Determine size (e.g., 1 to 5 squares)
    int target_size = GetRandomInt(1, 9); 
    cout << "Generating a block of size: " << target_size << endl;
    
    vector<int> block_indices;
    
    // 2. Pick a random starting square (0 to 8 for a 3x3 grid)
    int start_index = GetRandomInt(0, 8); 
    block_indices.push_back(start_index);

    // 3. Keep adding adjacent squares until we hit our target size
    while (block_indices.size() < target_size)
    {
        vector<int> valid_neighbors_pool;

        // Collect all neighbors adjacent to the squares we ALREADY have
        for (int active_idx : block_indices)
        {
            vector<int> neighbors = GetAdjacentSquareIndices(active_idx, 3, 3);
            
            for (int n : neighbors)
            {
                // Only include the neighbor if we haven't added it to our block yet
                if (find(block_indices.begin(), block_indices.end(), n) == block_indices.end() &&
                    find(valid_neighbors_pool.begin(), valid_neighbors_pool.end(), n) == valid_neighbors_pool.end())
                {
                    valid_neighbors_pool.push_back(n);
                }
            }
        }

        // Safety break: if the shape has swallowed the entire grid or has nowhere to grow
        if (valid_neighbors_pool.empty()) {
            break;
        }

        // 4. Randomly pick ONE neighbor from our available growth options and add it
        int random_pool_idx = GetRandomInt(0, valid_neighbors_pool.size() - 1);
        block_indices.push_back(valid_neighbors_pool[random_pool_idx]);
    }

    return block_indices;
}


void ResetMiniGrid(vector<int> &grid_indices)
{
    if (IsMouseButtonPressed(1))
    {
        grid_indices = GetRandomBlockIndices();
    }
    
}


int main()
{
  
    vector<GridSquare> storage_squares_game;
    vector<GridSquare> storage_squares_outer;
    Rectangle storageGridGame = (Rectangle){60.0f, 50.0f, 500.0f,500.0f};
    Rectangle storageGridOuter1 = (Rectangle){630.0f, 220.0f, 150.0f,150.0f};
    
    CreateGridInRect(storageGridGame, 10, 10, storage_squares_game);
    CreateGridInRect(storageGridOuter1, 3, 3, storage_squares_outer);

    vector<int> square_indices = GetRandomBlockIndices();
    


    const int window_width = 1200;
    const int window_height  = 600;
    InitWindow(window_width, window_height, "BLOCKS BLOCKS BLOCKS");

    while (!WindowShouldClose())
    {
        //   Rendering
        BeginDrawing();

        DebugMousePosition();
        ResetMiniGrid(square_indices);

        ClearBackground(BLACK);
        //DrawText("Let me start the drawing now", 100,200, 30, WHITE);
        for (auto &&square : storage_squares_game)
        {
            RenderSquares(square);
        }
        int starting_index=0;
        for (auto &&square : storage_squares_outer)
        {
            RenderSquares(square);
            for (int i = 0; i < int(square_indices.size()); i++)
            {
                if (starting_index == square_indices[i])
                {
                    RenderSquaresBlue(square);
                }
                
            }
            
            DebugSquareIndices(square,starting_index);
            starting_index+=1;
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}