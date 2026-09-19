#include "raylib.h"

// ---------- The reusable piece ----------
struct Draggable
{
    bool dragging = false;
    Vector2 grabOffset{0, 0};

    // pos          = your object's position (moved in place)
    // mouseOnMe    = true if the mouse is currently over your object
    void Update(Vector2& pos, bool mouseOnMe)
    {
        Vector2 mouse = GetMousePosition();

        if (!dragging)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouseOnMe)
            {
                dragging   = true;
                grabOffset = { mouse.x - pos.x, mouse.y - pos.y };
            }
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            dragging = false;
        }
        else
        {
            pos = { mouse.x - grabOffset.x, mouse.y - grabOffset.y };
        }
    }
};

// ---------- Three totally different objects ----------
struct Box
{
    Vector2 pos;
    int w, h;
    Color color;
    Draggable drag;

    void Update()
    {
        Vector2 m = GetMousePosition();
        bool onMe = CheckCollisionPointRec(m, { pos.x, pos.y, (float)w, (float)h });
        drag.Update(pos, onMe);
    }
    void Draw()
    {
        DrawRectangle(pos.x, pos.y, w, h, color);
    }
};

struct Ball
{
    Vector2 pos;
    float radius;
    Color color;
    Draggable drag;

    void Update()
    {
        Vector2 m = GetMousePosition();
        bool onMe = CheckCollisionPointCircle(m, pos, radius);
        drag.Update(pos, onMe);
    }
    void Draw()
    {
        DrawCircleV(pos, radius, color);
    }
};

struct Sprite
{
    Vector2 pos;
    Texture2D tex;
    Draggable drag;

    void Update()
    {
        Vector2 m = GetMousePosition();
        Rectangle r = { pos.x, pos.y, (float)tex.width, (float)tex.height };
        bool onMe = CheckCollisionPointRec(m, r);
        drag.Update(pos, onMe);
    }
    void Draw()
    {
        DrawTextureV(tex, pos, WHITE);
    }
};

// ---------- Demo ----------
int main()
{
    const int screenW = 800;
    const int screenH = 450;

    InitWindow(screenW, screenH, "Drag anything");
    SetTargetFPS(60);

    Box  box  { {100, 100}, 80, 60, YELLOW, {} };
    Ball ball { {400, 200}, 40.0f, SKYBLUE, {} };

    // Generate a texture so this runs without an external file
    Image img = GenImageChecked(64, 64, 16, 16, RED, MAROON);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);

    Sprite sprite { {600, 300}, tex, {} };

    while (!WindowShouldClose())
    {
        // ---- Update ----
        box.Update();
        ball.Update();
        sprite.Update();

        // ---- Draw ----
        BeginDrawing();
            ClearBackground(BLACK);

            box.Draw();
            ball.Draw();
            sprite.Draw();

            DrawText("Drag the box, circle, or sprite.", 10, 10, 20, LIGHTGRAY);
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();
    return 0;
}