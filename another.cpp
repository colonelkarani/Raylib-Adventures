#include <raylib.h>
#include <vector>

using namespace std;

const int window_height = 600;
const int window_width = 600;



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
void ShootSentry(float &start_time)
{
    start_time += GetFrameTime();

        if (start_time>=2)
        {
       if (next_bullet<ammo)
       {
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


int main()
{


InitWindow( window_height, window_width, "Sentry");

static float start_time = 0;

SentryGun sentry1({0,0}, 30);
sentry1.Reload();


while(!WindowShouldClose())
{
    BeginDrawing();
    ClearBackground(RAYWHITE);
    RenderSentry(sentry1);
    sentry1.ShootSentry(start_time);
    RenderBullets(sentry1.bullets);

    EndDrawing();
}

CloseWindow();


}