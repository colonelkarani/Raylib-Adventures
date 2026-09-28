#include <raylib.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <format>

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
                DrawTextureProportional(texture, position.x, position.y, 125,250);
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
    bool isWinning;

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
            return 1; // Four of a kind!
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

int HasFullHouse(vector<Card> table_cards, vector<Card> player_cards) {
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
        if (pair.second == 3)
        {
            no_of_pairs+=50;
        }
        if (pair.second == 2) {

            no_of_pairs++;
        }

        

    }
if (no_of_pairs >50)
{
    return 1;
}

    return 0; 
}

int HasFlush(vector<Card> table_cards, vector<Card> player_cards)
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

    int spades_count=0;
    int clubs_count=0;
    int hearts_count=0;
    int diamonds_count=0;

    for (auto &&card : player_table_cards)
    {
        if (card.suit == "Hearts")
        {
            hearts_count++;
        }
        else if (card.suit == "Diamonds")
        {
            diamonds_count++;
        }
        else if(card.suit == "Clubs")
        {
            clubs_count++;
        }else
        {
            spades_count++;
        }
   
    }
        if (clubs_count==5||diamonds_count==5||spades_count==5||hearts_count==5)
        {
            return 1;
        }
        else
        {
            return 0;
        }     
}

// Converts card strings ("2" through "13", "1") into numeric values where Ace is 14
int getCardRank(const std::string& card) {
    int rank = std::stoi(card);
    if (rank == 1) return 14; // Treat Ace as high by default
    return rank;
}

bool HasStraight(vector<Card> table_cards, vector<Card> player_cards) {
    // 1. Convert ranks to numbers and insert into a set to sort and remove duplicates
   
   vector<string> hand;


    for (auto &&card : table_cards)
    {
        hand.push_back(card.rank);
    }
    for (auto &&card : player_cards)
    {
        hand.push_back(card.rank);
    }

    std::set<int> ranks;
    for (const auto& card : hand) {
        ranks.insert(getCardRank(card));
    }

    // A straight requires at least 5 unique card ranks
    if (ranks.size() < 5) return false;

    // 2. Check for the special Ace-low "Wheel" straight (A, 2, 3, 4, 5)
    // In our set, this looks like (2, 3, 4, 5, 14)
    if (ranks.count(14) && ranks.count(2) && ranks.count(3) && ranks.count(4) && ranks.count(5)) {
        return true;
    }

    // 3. Check for any standard 5-card consecutive sequence
    // Move a window of 5 elements across the sorted set
    auto it = ranks.begin();
    auto end_window = std::prev(ranks.end(), 4); // Stop where a 5-card window can still fit
    
    for (; it != end_window; ++it) {
        auto check_it = it;
        bool is_straight = true;
        
        // Check if the next 4 elements increase by exactly 1 each step
        for (int i = 0; i < 4; ++i) {
            auto current = check_it;
            auto next = ++check_it;
            if (*next != *current + 1) {
                is_straight = false;
                break;
            }
        }
        if (is_straight) return true;
    }

    return false;
}

void EvaluateCards()
{

}

void RenderPlayerCardStats(vector<Card> tableCards, vector<Card> playersCards, Vector2 position)
{
    DrawText(TextFormat("Has Pair: %d", HasPair(tableCards, playersCards)), position.x,position.y, 20, BLACK);
    DrawText(TextFormat("Has Two Pairs: %d", HasTwoPair(tableCards, playersCards)), position.x,position.y+25 ,20, BLACK);
    DrawText(TextFormat("Has Three of a Kind: %d", HasThreeOfAKind(tableCards, playersCards)), position.x,position.y+50, 20, BLACK);
    DrawText(TextFormat("Has Four of a Kind: %d", HasFourOfAKind(tableCards, playersCards)), position.x,position.y+75, 20, BLACK);
    DrawText(TextFormat("Has Full house: %d", HasFullHouse(tableCards, playersCards)), position.x,position.y+100, 20, BLACK);
    DrawText(TextFormat("Has Flush: %d", HasFlush(tableCards, playersCards)), position.x,position.y+125, 20, BLACK);
    DrawText(TextFormat("Has Straight: %d", HasStraight(tableCards, playersCards)), position.x,position.y+150, 20, BLACK);
}

void RenderCards(string name_of_deck, vector<Card> cards,Vector2 position)
{
    DrawText(name_of_deck.c_str(), position.x, position.y, 20, BLACK);
    position.y+=25;
    for (auto &&i : cards)
    {
        i.RenderCard({position.x,position.y});
        position.x +=i.width;
    } 
}

void RenderHighCard(string name_of_player, vector<Card> tableCards, vector<Card> playersCards, Vector2 position)
{
    string display_text =  format("{}\'s High Card", name_of_player);
    DrawText(display_text.c_str(),position.x, position.y, 20, BLACK);
    position.y +=25;
    Card high_card = FindHighCard(tableCards, playersCards);
    high_card.RenderCard(position);
}

int main()
{
    InitWindow(900,750,"Poker: Game of life");
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
    vector<Card> bot1Cards;
    vector<Card> tableCards;

    for (int i = 0; i < 2; i++)
    {
        playersCards.push_back(MainDeck.dealCard());
    }

        for (int i = 0; i < 2; i++)
    {
        bot1Cards.push_back(MainDeck.dealCard());
    }

        
while (!WindowShouldClose())
{
    BeginDrawing();

    ClearBackground(BLUE);

    if (IsMouseButtonPressed(0))
    {
        tableCards.push_back(MainDeck.dealCard());
    }
    

    RenderCards("Your cards", playersCards, (Vector2){0,0});
    RenderCards("Table", tableCards, (Vector2){0,200});
    RenderCards("Bots Cards", bot1Cards, (Vector2){0,400});

    RenderHighCard("Player", tableCards, playersCards, {300,0});
    RenderHighCard("Bot", tableCards, bot1Cards, {600,0});

    RenderPlayerCardStats(tableCards, playersCards, {500,300});
    RenderPlayerCardStats(tableCards, bot1Cards, {500,500});
    

    EndDrawing();
}

    MainDeck.UnloadAll();
    for (auto& card : playersCards) card.Unload();
    for (auto& card : tableCards) card.Unload();
    for (auto& card : bot1Cards) card.Unload();

    CloseWindow();
return 0;
}