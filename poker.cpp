#include <raylib.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <iostream>

using namespace std;

    // Constant lists for card properties
    const vector<string> suits = {"Hearts", "Diamonds", "Clubs", "Spades"};
    const vector<string> ranks = {
        "Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"
    };

class Card
{
public:
    int width = 100;
    int height = 150;
    string suit;
    string rank;

    Card(string suit, string rank):
    suit(suit), rank(rank)
    {};

    void RenderCard(Vector2 position)
    {
        DrawRectangle(position.x, position.y, width, height, WHITE);
        DrawRectangleLines(position.x, position.y, width, height, BLACK);
        DrawText(suit.c_str(), position.x, position.y ,20, BLACK);
        DrawText(rank.c_str(), position.x, position.y+20 ,20, BLACK);
    }
    
};

struct Deck
{
    vector<Card> cards;
    void shuffle() {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(cards.begin(), cards.end(), g);
    }
    Card dealCard() {
        if (cards.empty()) {
            throw std::out_of_range("No cards left in the deck!");
        }
        Card topCard = cards.back();
        cards.pop_back(); // Remove the card from the deck
        return topCard;
    }

};


int main()
{
    InitWindow(500,500,"Jr is cool");
    Deck MainDeck;
    // Loop through all 4 suits and 13 ranks
    for (int s = 0; s < 4; ++s) {
        for (int r = 0 ; r < 13; ++r) {
            Card card(suits[s], ranks[r]);
            MainDeck.cards.push_back(card);
        }
    }
    MainDeck.shuffle();
    vector<Card> playersCards;
    for (int i = 0; i < 2; i++)
    {
        playersCards.push_back(MainDeck.dealCard());
    }
    
    Card RandomCard = MainDeck.dealCard();
    Card RandomCard2 = MainDeck.dealCard();
    
while (!WindowShouldClose())
{
    BeginDrawing();

    ClearBackground(BLUE);

    float position_y = 20;
    float position_x = 0;

    for (auto &&i : playersCards)
    {
        i.RenderCard({position_x,position_y});
        position_x +=i.width;
    }

    

    EndDrawing();
}


    CloseWindow();
return 0;
}