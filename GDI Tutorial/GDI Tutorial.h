#pragma once

#include "resource.h"
#include <string>

HBRUSH CreatePen(HDC hdc, COLORREF color);
HBRUSH CreateBrush(HDC hdc, COLORREF color, int width);
HFONT CreateFont(HDC hdc, std::string name, int size);