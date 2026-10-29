#include <raylib.h>
#include <vector>

using namespace std;

const int window_height = 600;
const int window_width = 600;

int movement_velocity = 20;

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
        return {float(position.x), float(position.y), width, width};
    }
    Vector2 GetBoxCentre()
    {
        return (Vector2){position.x+width/2, position.y+width/2};
    }   
};

void  MovePlayerBox (Square &playerSquare)
{
    if ((IsKeyDown(KEY_DOWN)||IsKeyDown(KEY_S))&&playerSquare.position.y< (window_height - playerSquare.width))
    {
       playerSquare.position.y+= movement_velocity* GetFrameTime();
    }
    if ((IsKeyDown(KEY_UP)||IsKeyDown(KEY_W))&&playerSquare.position.y> 0)
    {
       playerSquare.position.y-= movement_velocity* GetFrameTime();
    }
    if ((IsKeyDown(KEY_LEFT)||IsKeyDown(KEY_A))&&playerSquare.position.x> (0))
    {
       playerSquare.position.x-= movement_velocity* GetFrameTime();
    }
    if ((IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D))&&playerSquare.position.x < (window_width - playerSquare.width))
    {
       playerSquare.position.x+= movement_velocity* GetFrameTime();
    }
    
}

void RenderSquare(Square square)
{
    DrawRectangle(square.position.x, square.position.y, square.width, square.width, square.square_color);
}



struct Bullet
{
    Vector2 position;
    Vector2 velocity;
    bool in_sentry = false;
    void UpdateBulletVelocity()
{
    if (!in_sentry)
    {
    position.x += velocity.x * GetFrameTime();
    position.y += velocity.y* GetFrameTime();
    }
}
};

class Crosshair
{
    public:
    Vector2 position;
    int r1;
    Color color = BLACK;
    int r2;
    Crosshair (int r1, int r2) : r1(r1), r2(r2){}
    void Render()
    {
        DrawCircleLines(position.x, position.y, r1, color);
        DrawCircleLines(position.x, position.y, r2, color);
        
    }
    void UpdatePosition()
    {
        position = GetMousePosition();
    }
};

class SentryGun 
{
    public:
    Vector2 position;
    int ammo;
    Color color = PURPLE;
    int width  =40;
    vector<Bullet> bullets;
    int next_bullet = 0;
SentryGun(Vector2 position,int ammo): position(position), ammo(ammo){}
Vector2 GetCentre()
{
    return (Vector2){position.x+width/2, position.y+width/2};
};
void Reload()
{
    for (int i = 0; i < ammo; i++)
    {
       Bullet bullet;
       bullet.position = GetCentre();
       bullet.velocity = {20,20};
       bullet.in_sentry = true;
       bullets.push_back(bullet);
       next_bullet = 0;
    }
}
void ShootSentry(float &start_time, Vector2 velocity)
{
    start_time += GetFrameTime();

        if (start_time>=.5)
        {
       if (next_bullet<ammo)
       {
        bullets[next_bullet].velocity = velocity;
        bullets[next_bullet].in_sentry = false;
        next_bullet++;
       }
       start_time = 0;
       
        } 

    for (auto &&bullet : bullets)
    {
        bullet.UpdateBulletVelocity();
    }
    
}



};


void RenderSentry(SentryGun sentry)
{
    DrawRectangle(sentry.position.x ,sentry.position.y, sentry.width, sentry.width, sentry.color);
}

void RenderBullets(vector<Bullet> bullets)
{
    for (auto &&bullet : bullets)
    {
    DrawRectangle(bullet.position.x ,bullet.position.y, 10,10, BLACK);
    }    
}

Vector2 GetSentryPlayerVelocity(SentryGun sentry, Square player, int division_factor)
{
Vector2 sentry_position = sentry.GetCentre();
Vector2 player_position = player.position;

return (Vector2){(player_position.x - sentry_position.x)/division_factor, (player_position.y - sentry_position.y)/division_factor};
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

int main()
{


InitWindow( window_height, window_width, "Sentry");

static float start_time = 0;

SentryGun sentry1({0,0}, 300);
Square player_square({200,200} ,10, BLUE);
sentry1.Reload();

Crosshair crosshair(5,10);



while(!WindowShouldClose())
{
    BeginDrawing();
    ClearBackground(YELLOW);
    RenderSentry(sentry1);
    RenderSquare(player_square);
    MovePlayerBox(player_square);
    PlayerSprint();
    sentry1.ShootSentry(start_time, GetSentryPlayerVelocity(sentry1, player_square, 1));
    RenderBullets(sentry1.bullets);

    crosshair.Render();
    crosshair.UpdatePosition();

    EndDrawing();
}

CloseWindow();


}