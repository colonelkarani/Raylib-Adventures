#include <raylib.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <iostream>

using namespace std;

template <typename T>
int find_vector_index(const std::vector<T>& vec, const T& value) {
    // 1. Search for the value
    auto it = std::find(vec.begin(), vec.end(), value);
    
    // 2. Return the index if found, or -1 if not found
    if (it != vec.end()) {
        return std::distance(vec.begin(), it);
    }
    
    return -1; // Standard way to indicate "not found"
}
// Card properties
const vector<string> suits = {"Hearts", "Diamonds", "Clubs", "Spades"};
const vector<string> ranks = {
    "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13","1"
};

typedef enum 
{
HIGH_CARD,
PAIR,
TWO_PAIRS
} CardValue;

class Card
{
public:
    int width = 100;
    int height = 150;
    string suit;
    string rank;
    Texture2D texture;

    Card(string suit, string rank):
    suit(suit), rank(rank)
    {
        string filePath = "cards/" + suit + " " + rank + ".png";
        texture = LoadTexture(filePath.c_str());
    };

    void RenderCard(Vector2 position)
    {
    if (texture.id > 0)
            {
                // Calculate scale to fit specified width and height
                float scaleX = width / texture.width;
                float scaleY = height / texture.height;

                DrawTextureEx(texture, position, 0.0f, scaleX, WHITE);
            }
            else
            {
                // Fallback rendering 
                DrawRectangle(position.x, position.y, width, height, WHITE);
                DrawRectangleLines(position.x, position.y, width, height, BLACK);
                DrawText(suit.c_str(), position.x + 5, position.y + 5, 15, BLACK);
                DrawText(rank.c_str(), position.x + 5, position.y + 25, 15, BLACK);
            }
    }

    void Unload()
    {
        UnloadTexture(texture);
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

    void UnloadAll()
    {
        for (auto& card : cards)
        {
            card.Unload();
        }
    }
};

Card FindHighCard(vector<Card> table_cards, vector<Card> player_cards)
{

    vector<Card> player_table_cards;

    for (auto &&card : table_cards)
    {
        player_table_cards.push_back(card);
    }
    for (auto &&card : player_cards)
    {
        player_table_cards.push_back(card);
    }


    Card high_card = player_table_cards[0];

    for (auto &&card : player_table_cards)
    {
       int current_card_index = find_vector_index(ranks, card.rank);
        int high_card_index = find_vector_index(ranks, high_card.rank);

        if (current_card_index > high_card_index)
        {
            high_card = card;
        }
    }
    return high_card;

}


void EvaluateCards()
{
    
}


int main()
{
    InitWindow(800,600,"Poker: Game of life");
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
    vector<Card> tableCards;



    for (int i = 0; i < 2; i++)
    {
        playersCards.push_back(MainDeck.dealCard());
    }

        
while (!WindowShouldClose())
{
    BeginDrawing();

    ClearBackground(BLUE);

    if (IsMouseButtonPressed(0))
    {
        tableCards.push_back(MainDeck.dealCard());
    }
    

    float position_y = 20;
    float position_x_player = 0;
    float position_x_table = 0;

    

    for (auto &&i : playersCards)
    {
        i.RenderCard({position_x_player,position_y});
        position_x_player +=i.width;
    }

    for (auto &&i : tableCards)
    {
        i.RenderCard({position_x_table, 250});
        position_x_table+=i.width;
    }
    
    Card high_card = FindHighCard(tableCards, playersCards);
    high_card.RenderCard({500, 0});

    

    EndDrawing();
}

    MainDeck.UnloadAll();
    for (auto& card : playersCards) card.Unload();
    for (auto& card : tableCards) card.Unload();

    CloseWindow();
return 0;
}