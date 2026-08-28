#include <iostream>
#include <random>

using namespace std;

int find_random_int (int lower, int upper)
{
    static random_device rd;
    static mt19937 gen(rd());

    if (lower<upper)
    {
        uniform_int_distribution<float> dist(lower, upper);
    return dist(gen);
    }
    else
    {
        uniform_int_distribution<float> dist(upper, lower);
    return dist(gen);
    }
}
float find_random_float (float lower, float upper)
{
    static random_device rd;
    static mt19937 gen(rd());

    if (lower<upper)
    {
        uniform_int_distribution<float> dist(lower, upper);
    return dist(gen);
    }
    else
    {
        uniform_int_distribution<float> dist(upper, lower);
    return dist(gen);
    }
}