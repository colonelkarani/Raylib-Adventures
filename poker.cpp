#include <raylib.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <iostream>
#include <map>

using namespace std;

template <typename T>
int find_vector_index(const vector<T>& vec, const T& value) {
    // 1. Search for the value
    auto it = find(vec.begin(), vec.end(), value);
    
    // 2. Return the index if found, or -1 if not found
    if (it != vec.end()) {
        return distance(vec.begin(), it);
    }
    
    return -1; // Standard way to indicate "not found"
}
// Card properties
const vector<string> suits = {"Hearts", "Diamonds", "Clubs", "Spades"};
const vector<string> ranks = {
    "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13","1"
};


void DrawTextureProportional(
    Texture2D texture,
    float x, float y,
    float max_width,
    float max_height
)
{
    float scale_w = max_width / texture.width;
    float scale_h = max_height / texture.height;

    float scale = fminf(scale_w, scale_h);

    float new_width  = texture.width * scale;
    float new_height = texture.height * scale;

    Rectangle source = {
        0, 0,
        (float)texture.width,
        (float)texture.height
    };

    Rectangle dest = {
        x, y,
        new_width,
        new_height
    };

    Vector2 origin = { 0, 0 };

    DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
}


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

                DrawTextureProportional(texture, position.x, position.y, 100,200);
            
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

struct Player
{
    int money;
    int hand_strength;
    vector<Card> cards;

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


// bool HasPair(vector<Card> table_cards, vector<Card> player_cards)
// {
// bool has_pair = false;
// vector<Card> player_table_cards;

// for (auto &&card : table_cards)
// {
//     player_table_cards.push_back(card);
// }
// for (auto &&card : player_cards)
// {
//     player_table_cards.push_back(card);
// }

// map<string, int> rank_counts;

// for (auto &&card : player_table_cards)
// {
//     rank_counts[c]
// }



// return has_pair;

// }

int HasPair(vector<Card> table_cards, vector<Card> player_cards) {
    vector<Card> player_table_cards;

    for (auto &&card : table_cards)
    {
        player_table_cards.push_back(card);
    }
    for (auto &&card : player_cards)
    {
        player_table_cards.push_back(card);
    }

    // A map tracking: "Rank Name" -> How many times it appears
    std::map<std::string, int> rankCounts;

    // 1. Loop through the hand and count the frequencies of each rank
    for (const auto& card : player_table_cards) {
        rankCounts[card.rank]++;
    }


    for (const auto& pair : rankCounts) {
        if (pair.second == 2) {

            return 1; // Found a pair!
        }
    }

    return 0; // Checked everything, no pair found
}

int HasThreeOfAKind(vector<Card> table_cards, vector<Card> player_cards) {
    vector<Card> player_table_cards;

    for (auto &&card : table_cards)
    {
        player_table_cards.push_back(card);
    }
    for (auto &&card : player_cards)
    {
        player_table_cards.push_back(card);
    }

    // A map tracking: "Rank Name" -> How many times it appears
    std::map<std::string, int> rankCounts;

    // 1. Loop through the hand and count the frequencies of each rank
    for (const auto& card : player_table_cards) {
        rankCounts[card.rank]++;
    }


    for (const auto& pair : rankCounts) {

        if (pair.second == 3) {
            return 1; // Three of a kind!
        }
    }

    return 0; // Checked everything, no pair found
}

int HasFourOfAKind(vector<Card> table_cards, vector<Card> player_cards) {
    vector<Card> player_table_cards;

    for (auto &&card : table_cards)
    {
        player_table_cards.push_back(card);
    }
    for (auto &&card : player_cards)
    {
        player_table_cards.push_back(card);
    }

    // A map tracking: "Rank Name" -> How many times it appears
    std::map<std::string, int> rankCounts;

    // 1. Loop through the hand and count the frequencies of each rank
    for (const auto& card : player_table_cards) {
        rankCounts[card.rank]++;
    }


    for (const auto& pair : rankCounts) {

        if (pair.second == 4) {
            return 1; // Three of a kind!
        }
    }

    return 0; // Checked everything, no pair found
}

int HasTwoPair(vector<Card> table_cards, vector<Card> player_cards) {
    vector<Card> player_table_cards;

    for (auto &&card : table_cards)
    {
        player_table_cards.push_back(card);
    }
    for (auto &&card : player_cards)
    {
        player_table_cards.push_back(card);
    }

    // A map tracking: "Rank Name" -> How many times it appears
    map<string, int> rankCounts;

    // 1. Loop through the hand and count the frequencies of each rank
    for (const auto& card : player_table_cards) {
        rankCounts[card.rank]++;
    }

int no_of_pairs=0;
    for (const auto& pair : rankCounts) {
        if (pair.second == 2) {

            no_of_pairs++;
        }

    }
if (no_of_pairs == 2)
{
    return 1;
}

    return 0; 
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

    DrawText(TextFormat("Has Pair: %d", HasPair(tableCards, playersCards)), 500,470, 20, BLACK);
    DrawText(TextFormat("Has Two Pairs: %d", HasTwoPair(tableCards, playersCards)), 500,500, 20, BLACK);
    DrawText(TextFormat("Has Three of a Kind: %d", HasThreeOfAKind(tableCards, playersCards)), 500,530, 20, BLACK);
    DrawText(TextFormat("Has Four of a Kind: %d", HasThreeOfAKind(tableCards, playersCards)), 500,560, 20, BLACK);

    

    EndDrawing();
}

    MainDeck.UnloadAll();
    for (auto& card : playersCards) card.Unload();
    for (auto& card : tableCards) card.Unload();

    CloseWindow();
return 0;
}