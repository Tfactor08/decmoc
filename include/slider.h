#ifndef SLIDER_H
#define SLIDER_H

#include <stdio.h>
#include <raylib.h>

typedef struct Slider Slider;

Slider *SliderCreate(int startX, int endX, int posY, float min, float max);
float SliderGetValue(Slider *slider);
void SliderSetCurrentPos(Slider *slider);
void SliderDraw(Slider *slider);
void SliderDestroy(Slider *slider);

#endif // SLIDER_H
