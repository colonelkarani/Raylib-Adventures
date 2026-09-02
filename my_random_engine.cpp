#include <iostream>
#include <random>
#include <raylib.h>
#include "VKUtils.h"

using namespace std;

int GetRandomInt (int lower, int upper)
{
    static random_device rd;
    static mt19937 gen(rd());

    if (lower<upper)
    {
        uniform_int_distribution<int> dist(lower, upper);
    return dist(gen);
    }
    else
    {
        uniform_int_distribution<int> dist(upper, lower);
    return dist(gen);
    }
}
// float find_random_float (float lower, float upper)
// {
//     static random_device rd;
//     static mt19937 gen(rd());

//     if (lower<upper)
//     {
//         uniform_int_distribution<float> dist(lower, upper);
//     return dist(gen);
//     }
//     else
//     {
//         uniform_int_distribution<float> dist(upper, lower);
//     return dist(gen);
//     }
// }


Color GetRandomColor()
{
    return (Color)
    {
        (unsigned char)GetRandomInt(0,256), 
        (unsigned char)GetRandomInt(0,256), 
        (unsigned char)GetRandomInt(0,256), 
        (unsigned char)GetRandomInt(0,256)
    };
}

Color GetRandomSolidColor()
{
    return (Color)
    {
        (unsigned char)GetRandomInt(0,256), 
        (unsigned char)GetRandomInt(0,256), 
        (unsigned char)GetRandomInt(0,256), 
        (unsigned char)255
    };
}

Color GetOppositeColor(Color normalColor)
{
     return (Color)
    {
        (unsigned char)(255 -normalColor.r ), 
        (unsigned char)(255 - normalColor.g), 
        (unsigned char)(255 - normalColor.b), 
        (unsigned char)(255 - normalColor.a)
    };
}

Color GetOppositeSolidColor(Color normalColor)
{
     return (Color)
    {
        (unsigned char)(255 -normalColor.r) , 
        (unsigned char)(255 - normalColor.g), 
        (unsigned char)(255 - normalColor.b), 
        (unsigned char)(255 )
    };
}