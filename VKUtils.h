#ifndef VKUTILS_H
#define VKUTILS_H

#pragma once
#include "raylib.h"

Color GetRandomColor();
Color GetRandomSolidColor();
Color GetOppositeColor(Color normalColor);
Color GetOppositeSolidColor(Color normalColor);

int GetRandomInt(int lower, int upper);


#endif