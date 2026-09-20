#include <raylib.h>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <optional>

using namespace std;

// ---------------------------------------------------------------------------
// Enums & Constants
// ---------------------------------------------------------------------------
enum PieceType { EMPTY, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };
enum PieceColor { NONE, WHITE_PIECE, BLACK_PIECE };

struct Move {
    int fromRow, fromCol;
    int toRow, toCol;
    int capturedPiece;      // PieceType
    int promotedTo;         // PieceType (0 if none)
    bool isEnPassant;
    bool isCastleKingSide;
    bool isCastleQueenSide;
};

struct ChessPiece {
    PieceType type = EMPTY;
    PieceColor color = NONE;
    bool hasMoved = false;
};

// ---------------------------------------------------------------------------
// Board Class
// ---------------------------------------------------------------------------
class ChessBoard {
public:
    ChessPiece board[8][8];
    PieceColor turn = WHITE_PIECE;
    int enPassantRow = -1, enPassantCol = -1;
    int halfMoveClock = 0;
    int fullMoveNumber = 1;
    vector<Move> moveHistory;

    ChessBoard() { reset(); }

    void reset() {
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                board[r][c] = { EMPTY, NONE, false };

        // Pawns
        for (int c = 0; c < 8; c++) {
            board[1][c] = { PAWN, BLACK_PIECE, false };
            board[6][c] = { PAWN, WHITE_PIECE, false };
        }
        // Back ranks
        PieceType backRank[8] = { ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK };
        for (int c = 0; c < 8; c++) {
            board[0][c] = { backRank[c], BLACK_PIECE, false };
            board[7][c] = { backRank[c], WHITE_PIECE, false };
        }

        turn = WHITE_PIECE;
        enPassantRow = enPassantCol = -1;
        halfMoveClock = 0;
        fullMoveNumber = 1;
        moveHistory.clear();
    }

    bool isInside(int r, int c) const { return r >= 0 && r < 8 && c >= 0 && c < 8; }

    // -----------------------------------------------------------------------
    // Pseudo-legal move generation
    // -----------------------------------------------------------------------
    void generateMoves(vector<Move>& moves, PieceColor side) const {
        moves.clear();
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].color != side || board[r][c].type == EMPTY) continue;
                switch (board[r][c].type) {
                    case PAWN:   generatePawnMoves(moves, r, c);   break;
                    case ROOK:   generateSlidingMoves(moves, r, c, { {1,0},{-1,0},{0,1},{0,-1} }); break;
                    case KNIGHT: generateKnightMoves(moves, r, c); break;
                    case BISHOP: generateSlidingMoves(moves, r, c, { {1,1},{1,-1},{-1,1},{-1,-1} }); break;
                    case QUEEN:  generateSlidingMoves(moves, r, c, { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} }); break;
                    case KING:   generateKingMoves(moves, r, c);   break;
                    default: break;
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Make / Unmake
    // -----------------------------------------------------------------------
    void makeMove(const Move& m) {
        ChessPiece& from = board[m.fromRow][m.fromCol];
        ChessPiece& to   = board[m.toRow][m.toCol];

        // Save state for undo
        Move undo = m;
        undo.capturedPiece = to.type;
        undo.promotedTo = 0;
        undo.isEnPassant = false;
        undo.isCastleKingSide = false;
        undo.isCastleQueenSide = false;

        // En passant capture
        if (m.isEnPassant) {
            int capRow = (from.color == WHITE_PIECE) ? m.toRow + 1 : m.toRow - 1;
            board[capRow][m.toCol] = { EMPTY, NONE, false };
        }

        // Move piece
        to = from;
        to.hasMoved = true;
        from = { EMPTY, NONE, false };

        // Promotion
        if (m.promotedTo != 0) {
            to.type = (PieceType)m.promotedTo;
        }

        // Castling rook move
        if (m.isCastleKingSide) {
            int row = m.fromRow;
            board[row][5] = board[row][7];
            board[row][5].hasMoved = true;
            board[row][7] = { EMPTY, NONE, false };
        }
        if (m.isCastleQueenSide) {
            int row = m.fromRow;
            board[row][3] = board[row][0];
            board[row][3].hasMoved = true;
            board[row][0] = { EMPTY, NONE, false };
        }

        // Update en passant target
        enPassantRow = enPassantCol = -1;
        if (from.type == PAWN && abs(m.toRow - m.fromRow) == 2) {
            enPassantRow = (m.fromRow + m.toRow) / 2;
            enPassantCol = m.fromCol;
        }

        // Half-move clock
        if (from.type == PAWN || undo.capturedPiece != EMPTY)
            halfMoveClock = 0;
        else
            halfMoveClock++;

        if (turn == BLACK_PIECE) fullMoveNumber++;
        turn = (turn == WHITE_PIECE) ? BLACK_PIECE : WHITE_PIECE;

        moveHistory.push_back(undo);
    }

    void unmakeMove() {
        if (moveHistory.empty()) return;
        Move m = moveHistory.back();
        moveHistory.pop_back();

        turn = (turn == WHITE_PIECE) ? BLACK_PIECE : WHITE_PIECE;
        if (turn == BLACK_PIECE) fullMoveNumber--;

        ChessPiece& from = board[m.fromRow][m.fromCol];
        ChessPiece& to   = board[m.toRow][m.toCol];

        // Restore piece
        from = to;
        from.hasMoved = false; // simplified; could restore exact flag
        from.type = (m.promotedTo != 0) ? PAWN : to.type;
        to = { EMPTY, NONE, false };

        // Restore captured piece
        if (m.capturedPiece != EMPTY) {
            to.type = (PieceType)m.capturedPiece;
            to.color = (turn == WHITE_PIECE) ? BLACK_PIECE : WHITE_PIECE;
        }

        // En passant
        if (m.isEnPassant) {
            int capRow = (turn == WHITE_PIECE) ? m.toRow + 1 : m.toRow - 1;
            board[capRow][m.toCol] = { PAWN, (turn == WHITE_PIECE) ? BLACK_PIECE : WHITE_PIECE, false };
        }

        // Castling
        if (m.isCastleKingSide) {
            int row = m.fromRow;
            board[row][7] = board[row][5];
            board[row][7].hasMoved = true;
            board[row][5] = { EMPTY, NONE, false };
        }
        if (m.isCastleQueenSide) {
            int row = m.fromRow;
            board[row][0] = board[row][3];
            board[row][0].hasMoved = true;
            board[row][3] = { EMPTY, NONE, false };
        }
    }

    // -----------------------------------------------------------------------
    // Legal move filtering
    // -----------------------------------------------------------------------
    vector<Move> getLegalMoves(PieceColor side) const {
        vector<Move> pseudo, legal;
        generateMoves(pseudo, side);
        for (const Move& m : pseudo) {
            ChessBoard copy = *this;
            copy.makeMove(m);
            if (!copy.isKingInCheck(side))
                legal.push_back(m);
        }
        return legal;
    }

    bool isKingInCheck(PieceColor side) const {
        int kingRow = -1, kingCol = -1;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                if (board[r][c].type == KING && board[r][c].color == side)
                    kingRow = r, kingCol = c;

        if (kingRow == -1) return false;

        PieceColor enemy = (side == WHITE_PIECE) ? BLACK_PIECE : WHITE_PIECE;
        vector<Move> enemyMoves;
        generateMoves(enemyMoves, enemy);
        for (const Move& m : enemyMoves)
            if (m.toRow == kingRow && m.toCol == kingCol)
                return true;
        return false;
    }

    bool isCheckmate(PieceColor side) const {
        return isKingInCheck(side) && getLegalMoves(side).empty();
    }

    bool isStalemate(PieceColor side) const {
        return !isKingInCheck(side) && getLegalMoves(side).empty();
    }

private:
    // -----------------------------------------------------------------------
    // Move generators
    // -----------------------------------------------------------------------
    void generatePawnMoves(vector<Move>& moves, int r, int c) const {
        ChessPiece p = board[r][c];
        int dir = (p.color == WHITE_PIECE) ? -1 : 1;
        int startRow = (p.color == WHITE_PIECE) ? 6 : 1;

        // One step
        if (isInside(r + dir, c) && board[r + dir][c].type == EMPTY) {
            addPawnMove(moves, r, c, r + dir, c, 0, false);
            // Two steps
            if (r == startRow && board[r + 2 * dir][c].type == EMPTY)
                addPawnMove(moves, r, c, r + 2 * dir, c, 0, false);
        }

        // Captures
        for (int dc : {-1, 1}) {
            int nr = r + dir, nc = c + dc;
            if (!isInside(nr, nc)) continue;
            if (board[nr][nc].type != EMPTY && board[nr][nc].color != p.color)
                addPawnMove(moves, r, c, nr, nc, board[nr][nc].type, false);
            if (nr == enPassantRow && nc == enPassantCol)
                addPawnMove(moves, r, c, nr, nc, PAWN, true);
        }
    }

    void addPawnMove(vector<Move>& moves, int fr, int fc, int tr, int tc, int cap, bool ep) const {
        // Promotion?
        if (tr == 0 || tr == 7) {
            for (int promo : { QUEEN, ROOK, BISHOP, KNIGHT })
                moves.push_back({ fr, fc, tr, tc, cap, promo, ep, false, false });
        } else {
            moves.push_back({ fr, fc, tr, tc, cap, 0, ep, false, false });
        }
    }

    void generateSlidingMoves(vector<Move>& moves, int r, int c, const vector<pair<int,int>>& dirs) const {
        ChessPiece p = board[r][c];
        for (auto [dr, dc] : dirs) {
            int nr = r + dr, nc = c + dc;
            while (isInside(nr, nc)) {
                if (board[nr][nc].type == EMPTY) {
                    moves.push_back({ r, c, nr, nc, 0, 0, false, false, false });
                } else {
                    if (board[nr][nc].color != p.color)
                        moves.push_back({ r, c, nr, nc, board[nr][nc].type, 0, false, false, false });
                    break;
                }
                nr += dr; nc += dc;
            }
        }
    }

    void generateKnightMoves(vector<Move>& moves, int r, int c) const {
        ChessPiece p = board[r][c];
        int offsets[8][2] = { {2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2} };
        for (auto& o : offsets) {
            int nr = r + o[0], nc = c + o[1];
            if (isInside(nr, nc) && (board[nr][nc].type == EMPTY || board[nr][nc].color != p.color))
                moves.push_back({ r, c, nr, nc, board[nr][nc].type, 0, false, false, false });
        }
    }

    void generateKingMoves(vector<Move>& moves, int r, int c) const {
        ChessPiece p = board[r][c];
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;
                int nr = r + dr, nc = c + dc;
                if (isInside(nr, nc) && (board[nr][nc].type == EMPTY || board[nr][nc].color != p.color))
                    moves.push_back({ r, c, nr, nc, board[nr][nc].type, 0, false, false, false });
            }
        }
        // Castling
        if (p.hasMoved) return;
        // King-side
        if (board[r][c + 1].type == EMPTY && board[r][c + 2].type == EMPTY &&
            board[r][c + 3].type == ROOK && !board[r][c + 3].hasMoved) {
            moves.push_back({ r, c, r, c + 2, 0, 0, false, true, false });
        }
        // Queen-side
        if (board[r][c - 1].type == EMPTY && board[r][c - 2].type == EMPTY &&
            board[r][c - 3].type == EMPTY && board[r][c - 4].type == ROOK && !board[r][c - 4].hasMoved) {
            moves.push_back({ r, c, r, c - 2, 0, 0, false, false, true });
        }
    }
};

// ---------------------------------------------------------------------------
// Simple AI (minimax with alpha-beta, depth 3)
// ---------------------------------------------------------------------------
int pieceValue(PieceType t) {
    switch (t) {
        case PAWN:   return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK:   return 500;
        case QUEEN:  return 900;
        case KING:   return 20000;
        default:     return 0;
    }
}

int evaluateBoard(const ChessBoard& b) {
    int score = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            if (b.board[r][c].type == EMPTY) continue;
            int val = pieceValue(b.board[r][c].type);
            if (b.board[r][c].color == WHITE_PIECE) score += val;
            else score -= val;
        }
    return score;
}

int minimax(ChessBoard& board, int depth, int alpha, int beta, bool maximizing) {
    if (depth == 0) return evaluateBoard(board);

    PieceColor side = maximizing ? WHITE_PIECE : BLACK_PIECE;
    vector<Move> moves = board.getLegalMoves(side);

    if (moves.empty()) {
        if (board.isKingInCheck(side))
            return maximizing ? -99999 : 99999;
        return 0; // stalemate
    }

    if (maximizing) {
        int best = -99999;
        for (const Move& m : moves) {
            board.makeMove(m);
            best = max(best, minimax(board, depth - 1, alpha, beta, false));
            board.unmakeMove();
            alpha = max(alpha, best);
            if (beta <= alpha) break;
        }
        return best;
    } else {
        int best = 99999;
        for (const Move& m : moves) {
            board.makeMove(m);
            best = min(best, minimax(board, depth - 1, alpha, beta, true));
            board.unmakeMove();
            beta = min(beta, best);
            if (beta <= alpha) break;
        }
        return best;
    }
}

Move findBestMove(ChessBoard& board, int depth) {
    PieceColor side = board.turn;
    vector<Move> moves = board.getLegalMoves(side);
    if (moves.empty()) return { -1, -1, -1, -1, 0, 0, false, false, false };

    Move bestMove = moves[0];
    int bestVal = (side == WHITE_PIECE) ? -99999 : 99999;

    for (const Move& m : moves) {
        board.makeMove(m);
        int val = minimax(board, depth - 1, -99999, 99999, side == BLACK_PIECE);
        board.unmakeMove();

        if (side == WHITE_PIECE && val > bestVal) { bestVal = val; bestMove = m; }
        if (side == BLACK_PIECE && val < bestVal) { bestVal = val; bestMove = m; }
    }
    return bestMove;
}

// ---------------------------------------------------------------------------
// Main (raylib)
// ---------------------------------------------------------------------------
int main() {
    const int screenWidth = 800;
    const int screenHeight = 800;
    const int cellSize = 100;

    InitWindow(screenWidth, screenHeight, "Chess Engine");
    SetTargetFPS(60);

    // Load textures (with fallback if files are missing)
    auto loadPieceTexture = [](const char* path) -> Texture2D {
        if (FileExists(path)) {
            Image img = LoadImage(path);
            ImageResize(&img, cellSize * 0.8, cellSize * 0.8);
            Texture2D tex = LoadTextureFromImage(img);
            UnloadImage(img);
            return tex;
        }
        return Texture2D{ 0 };
    };

    Texture2D texWhitePawn   = loadPieceTexture("pieces/pawn_white.png");
    Texture2D texBlackPawn   = loadPieceTexture("pieces/pawn_black.png");
    Texture2D texWhiteRook   = loadPieceTexture("pieces/rook_white.png");
    Texture2D texBlackRook   = loadPieceTexture("pieces/rook_black.png");
    Texture2D texWhiteKnight = loadPieceTexture("pieces/knight_white.png");
    Texture2D texBlackKnight = loadPieceTexture("pieces/knight_black.png");
    Texture2D texWhiteBishop = loadPieceTexture("pieces/bishop_white.png");
    Texture2D texBlackBishop = loadPieceTexture("pieces/bishop_black.png");
    Texture2D texWhiteQueen  = loadPieceTexture("pieces/queen_white.png");
    Texture2D texBlackQueen  = loadPieceTexture("pieces/queen_black.png");
    Texture2D texWhiteKing   = loadPieceTexture("pieces/king_white.png");
    Texture2D texBlackKing   = loadPieceTexture("pieces/king_black.png");

    ChessBoard board;
    int selectedRow = -1, selectedCol = -1;
    vector<Move> legalMoves;
    bool vsAI = true;
    bool gameOver = false;
    string statusMsg = "White's turn";

    while (!WindowShouldClose()) {
        // ---------------- Input ----------------
        if (!gameOver) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();
                int col = (int)(mouse.x / cellSize);
                int row = (int)(mouse.y / cellSize);

                if (row >= 0 && row < 8 && col >= 0 && col < 8) {
                    if (selectedRow == -1) {
                        // Select piece
                        if (board.board[row][col].color == board.turn) {
                            selectedRow = row;
                            selectedCol = col;
                            legalMoves = board.getLegalMoves(board.turn);
                        }
                    } else {
                        // Try to move
                        bool moved = false;
                        for (const Move& m : legalMoves) {
                            if (m.fromRow == selectedRow && m.fromCol == selectedCol &&
                                m.toRow == row && m.toCol == col) {
                                board.makeMove(m);
                                moved = true;
                                break;
                            }
                        }
                        selectedRow = selectedCol = -1;
                        legalMoves.clear();

                        if (moved) {
                            // Check game state
                            PieceColor nextSide = board.turn;
                            if (board.isCheckmate(nextSide)) {
                                gameOver = true;
                                statusMsg = (nextSide == WHITE_PIECE) ? "Checkmate! Black wins." : "Checkmate! White wins.";
                            } else if (board.isStalemate(nextSide)) {
                                gameOver = true;
                                statusMsg = "Stalemate! Draw.";
                            } else if (board.isKingInCheck(nextSide)) {
                                statusMsg = (nextSide == WHITE_PIECE) ? "White is in check!" : "Black is in check!";
                            } else {
                                statusMsg = (nextSide == WHITE_PIECE) ? "White's turn" : "Black's turn";
                            }
                        }
                    }
                }
            }
        }

        // AI move
        if (vsAI && !gameOver && board.turn == BLACK_PIECE) {
            Move aiMove = findBestMove(board, 3);
            if (aiMove.fromRow != -1) {
                board.makeMove(aiMove);
                PieceColor nextSide = board.turn;
                if (board.isCheckmate(nextSide)) {
                    gameOver = true;
                    statusMsg = (nextSide == WHITE_PIECE) ? "Checkmate! Black wins." : "Checkmate! White wins.";
                } else if (board.isStalemate(nextSide)) {
                    gameOver = true;
                    statusMsg = "Stalemate! Draw.";
                } else if (board.isKingInCheck(nextSide)) {
                    statusMsg = (nextSide == WHITE_PIECE) ? "White is in check!" : "Black is in check!";
                } else {
                    statusMsg = (nextSide == WHITE_PIECE) ? "White's turn" : "Black's turn";
                }
            }
        }

        // Reset key
        if (IsKeyPressed(KEY_R)) {
            board.reset();
            selectedRow = selectedCol = -1;
            legalMoves.clear();
            gameOver = false;
            statusMsg = "White's turn";
        }

        // ---------------- Drawing ----------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Board squares
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                Color col = ((r + c) % 2 == 0) ? Color{ 240, 217, 181, 255 } : Color{ 181, 136, 99, 255 };
                DrawRectangle(c * cellSize, r * cellSize, cellSize, cellSize, col);

                // Highlight selected square
                if (r == selectedRow && c == selectedCol)
                    DrawRectangle(c * cellSize, r * cellSize, cellSize, cellSize, Color{ 255, 255, 0, 100 });

                // Highlight legal targets
                for (const Move& m : legalMoves) {
                    if (m.fromRow == selectedRow && m.fromCol == selectedCol &&
                        m.toRow == r && m.toCol == c) {
                        DrawCircle(c * cellSize + cellSize / 2, r * cellSize + cellSize / 2,
                                   12, Color{ 0, 255, 0, 120 });
                    }
                }
            }
        }

        // Pieces
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                ChessPiece p = board.board[r][c];
                if (p.type == EMPTY) continue;

                Texture2D* tex = nullptr;
                if (p.type == PAWN)   tex = (p.color == WHITE_PIECE) ? &texWhitePawn   : &texBlackPawn;
                if (p.type == ROOK)   tex = (p.color == WHITE_PIECE) ? &texWhiteRook   : &texBlackRook;
                if (p.type == KNIGHT) tex = (p.color == WHITE_PIECE) ? &texWhiteKnight : &texBlackKnight;
                if (p.type == BISHOP) tex = (p.color == WHITE_PIECE) ? &texWhiteBishop : &texBlackBishop;
                if (p.type == QUEEN)  tex = (p.color == WHITE_PIECE) ? &texWhiteQueen  : &texBlackQueen;
                if (p.type == KING)   tex = (p.color == WHITE_PIECE) ? &texWhiteKing   : &texBlackKing;

                if (tex && tex->id != 0) {
                    float scale = 1.0f;
                    float tw = tex->width * scale;
                    float th = tex->height * scale;
                    float x = c * cellSize + (cellSize - tw) / 2;
                    float y = r * cellSize + (cellSize - th) / 2;
                    DrawTexture(*tex, (int)x, (int)y, WHITE);
                } else {
                    // Fallback: draw a circle
                    Color pc = (p.color == WHITE_PIECE) ? WHITE : BLACK;
                    DrawCircle(c * cellSize + cellSize / 2, r * cellSize + cellSize / 2,
                               cellSize / 3, pc);
                }
            }
        }

        // Status bar
        DrawRectangle(0, 0, screenWidth, 30, Color{ 0, 0, 0, 180 });
        DrawText(statusMsg.c_str(), 10, 5, 20, WHITE);
        DrawText("Press R to reset", screenWidth - 200, 5, 20, LIGHTGRAY);

        EndDrawing();
    }

    // Unload textures
    if (texWhitePawn.id)   UnloadTexture(texWhitePawn);
    if (texBlackPawn.id)   UnloadTexture(texBlackPawn);
    if (texWhiteRook.id)   UnloadTexture(texWhiteRook);
    if (texBlackRook.id)   UnloadTexture(texBlackRook);
    if (texWhiteKnight.id) UnloadTexture(texWhiteKnight);
    if (texBlackKnight.id) UnloadTexture(texBlackKnight);
    if (texWhiteBishop.id) UnloadTexture(texWhiteBishop);
    if (texBlackBishop.id) UnloadTexture(texBlackBishop);
    if (texWhiteQueen.id)  UnloadTexture(texWhiteQueen);
    if (texBlackQueen.id)  UnloadTexture(texBlackQueen);
    if (texWhiteKing.id)   UnloadTexture(texWhiteKing);
    if (texBlackKing.id)   UnloadTexture(texBlackKing);

    CloseWindow();
    return 0;
}