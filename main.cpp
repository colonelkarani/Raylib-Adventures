#include "raylib.h"
#include "my_random_engine.cpp" // Assumed to contain find_random_int(min, max)

// Cell structure to track logic and display state separately
struct Cell {
    bool isMine = false;
    bool isRevealed = false;
    bool isFlagged = false;
    int neighborMines = 0; // Number of mines in the surrounding 8 cells
};

const int ROWS = 15;
const int COLS = 15;
const int CELL_SIZE = 40;
const int TOTAL_MINES = 30;

Cell grid[ROWS][COLS];
bool gameOver = false;
bool gameWon = false;

// Function to safely check if coordinates are within bounds
bool IsValid(int r, int c) {
    return (r >= 0 && r < ROWS && c >= 0 && c < COLS);
}

// Function to automatically reveal empty surrounding tiles (Flood Fill)
void RevealCell(int r, int c) {
    if (!IsValid(r, c) || grid[r][c].isRevealed || grid[r][c].isFlagged) return;

    grid[r][c].isRevealed = true;

    // If the cell has 0 neighboring mines, automatically reveal its neighbors
    if (grid[r][c].neighborMines == 0 && !grid[r][c].isMine) {
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                RevealCell(r + dr, c + dc);
            }
        }
    }
}

// Function to check if player won (all non-mine tiles revealed)
void CheckWinCondition() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (!grid[r][c].isMine && !grid[r][c].isRevealed) {
                return; // Found a safe cell not yet revealed, game continues
            }
        }
    }
    gameWon = true;
}

// Initialize or reset the entire match board
void ResetGame() {
    gameOver = false;
    gameWon = false;

    // Reset grid states
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            grid[r][c] = Cell();
        }
    }

    // Place unique random mines using your while-loop engine
    int minesPlaced = 0;
    while (minesPlaced < TOTAL_MINES) {
        int r = find_random_int(0, ROWS - 1);
        int c = find_random_int(0, COLS - 1);

        if (!grid[r][c].isMine) {
            grid[r][c].isMine = true;
            minesPlaced++;
        }
    }

    // Calculate neighboring mine numbers for every cell
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (grid[r][c].isMine) continue;

            int count = 0;
            for (int dr = -1; dr <= 1; dr++) {
                for (int dc = -1; dc <= 1; dc++) {
                    if (IsValid(r + dr, c + dc) && grid[r + dr][c + dc].isMine) {
                        count++;
                    }
                }
            }
            grid[r][c].neighborMines = count;
        }
    }
}

int main() {
    InitWindow(COLS * CELL_SIZE, ROWS * CELL_SIZE, "Raylib Minesweeper");
    SetTargetFPS(60);

    ResetGame();

    while (!WindowShouldClose()) {
        // --- INPUT & LOGIC UPDATE ---
        if (IsKeyPressed(KEY_R)) {
            ResetGame(); // R Key to quickly restart
        }

        if (!gameOver && !gameWon) {
            // Get hovered tile
            int col = GetMouseX() / CELL_SIZE;
            int row = GetMouseY() / CELL_SIZE;

            if (IsValid(row, col)) {
                // Left Click: Reveal Cell
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (grid[row][col].isMine && !grid[row][col].isFlagged) {
                        gameOver = true; // Stepped on a mine!
                        // Reveal all mines to show the player where they failed
                        for (int r = 0; r < ROWS; r++) {
                            for (int c = 0; c < COLS; c++) {
                                if (grid[r][c].isMine) grid[r][c].isRevealed = true;
                            }
                        }
                    } else {
                        RevealCell(row, col);
                        CheckWinCondition();
                    }
                }
                // Right Click: Toggle Flag
                else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    if (!grid[row][col].isRevealed) {
                        grid[row][col].isFlagged = !grid[row][col].isFlagged;
                    }
                }
            }
        }

        // --- DRAWING GRAPHICS LAYER ---
        BeginDrawing();
        ClearBackground(DARKGRAY);

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                int x = c * CELL_SIZE;
                int y = r * CELL_SIZE;

                // Color configuration states
                if (grid[r][c].isRevealed) {
                    if (grid[r][c].isMine) {
                        DrawRectangle(x + 1, y + 1, CELL_SIZE - 1, CELL_SIZE - 1, RED); // Exploded Mine
                        DrawText("X", x + 14, y + 10, 20, WHITE);
                    } else {
                        DrawRectangle(x + 1, y + 1, CELL_SIZE - 1, CELL_SIZE - 1, RAYWHITE); // Revealed Safe Space
                        
                        // Display neighbor number text if above zero
                        if (grid[r][c].neighborMines > 0) {
                            Color numColor = (grid[r][c].neighborMines == 1) ? BLUE : 
                                             (grid[r][c].neighborMines == 2) ? GREEN : RED;
                            DrawText(TextFormat("%d", grid[r][c].neighborMines), x + 15, y + 10, 20, numColor);
                        }
                    }
                } else {
                    // Hidden cells
                    DrawRectangle(x + 1, y + 1, CELL_SIZE - 1, CELL_SIZE - 1, GRAY);
                    
                    if (grid[r][c].isFlagged) {
                        DrawText("F", x + 15, y + 10, 20, ORANGE); // Flagged Cell marker
                    }
                }
            }
        }

        // Overlay text if game status concludes
        if (gameOver) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));
            DrawText("GAME OVER!", GetScreenWidth()/2 - 100, GetScreenHeight()/2 - 20, 32, RED);
            DrawText("Press 'R' to Restart", GetScreenWidth()/2 - 95, GetScreenHeight()/2 + 20, 18, WHITE);
        } else if (gameWon) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));
            DrawText("YOU WIN!", GetScreenWidth()/2 - 75, GetScreenHeight()/2 - 20, 32, GREEN);
            DrawText("Press 'R' to Restart", GetScreenWidth()/2 - 95, GetScreenHeight()/2 + 20, 18, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
