#include <raylib.h>
#include <vector>


typedef enum Suit
{
SPADES,
CLUBS,
DIAMONDS,
HEARTS
};

typedef enum Rank { Ace = 1, Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King };

class Card
{

public:
    int width;
    int height;
    Vector2 position;
    Suit suit;

    Card(/* args */);
    ~Card();
};


int main()
{
    InitWindow(500,500,"Jr is cool");
while (!WindowShouldClose())
{
    BeginDrawing();
    ClearBackground(BLUE);
    EndDrawing();
}


    CloseWindow();
return 0;
}