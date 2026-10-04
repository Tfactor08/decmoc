#include "slider.h"
#include "../utils/basic_utils.c"

#define SLIDER_LABEL_MAX 10
#define HANDLE_RADIUS_DEFAULT 10

// TODO: allow setting slider position by clicking on the line

struct Slider {
    int width;
    int startX;
    int endX;
    int posY;
    float min;
    float max;
    bool isBeingDragged;
    Vector2 handlePos;
    float handleRadius;
    char label[SLIDER_LABEL_MAX];
};

static bool MouseOnSlider(Slider *slider)
{
    return CheckCollisionPointCircle(
        GetMousePosition(),
        slider->handlePos,
        slider->handleRadius
    );
}

static void SetCurrentMode(Slider *slider)
{
    if (slider->isBeingDragged) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            slider->isBeingDragged = false;
        }
    } else if (MouseOnSlider(slider) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        slider->isBeingDragged = true;
    }
}

Slider *SliderCreate(int startX, int endX, int posY, float min, float max, const char *label)
{
    Slider *slider = malloc(sizeof(Slider));
    MALLOC_CHECK(slider);

    float handleRadius;
#ifdef HANDLE_RADIUS
    handleRadius = HANDLE_RADIUS;
#else
    handleRadius = HANDLE_RADIUS_DEFAULT;
#endif // HANDLE_RADIUS
    int width = endX - startX;
    float magnitude = max - min;
    Vector2 handlePos = {
        .x = startX + width * ((1 - min) / magnitude), // Position the handle at value 1.
        .y = posY
    };

    *slider = (Slider) {
        .width = width,
        .startX = startX,
        .endX = endX,
        .posY = posY,
        .min = min,
        .max = max,
        .isBeingDragged = false,
        .handlePos = handlePos,
        .handleRadius = handleRadius
    };
    strncpy(slider->label, label, SLIDER_LABEL_MAX);
    return slider;
}

float SliderGetValue(Slider *slider)
{
    float magnitude = slider->max - slider->min;
    float handleX = slider->handlePos.x - slider->startX; // handle's x pos relative to the slider (not screen)
    float value = (magnitude * (handleX / slider->width)) + slider->min;
    return value;
}

// TODO: there must be better naming!
// This function must be called on every frame.
void SliderSetCurrentPos(Slider *slider)
{
    SetCurrentMode(slider);
    if (slider->isBeingDragged) {
        int newPos;
        int mouseX = GetMouseX();
        if (mouseX > slider->endX) newPos = slider->endX;
        else if (mouseX < slider->startX) newPos = slider->startX;
        else newPos = mouseX;
        slider->handlePos.x = newPos;
    }
}

void SliderDraw(Slider *slider)
{
    DrawText(slider->label, slider->startX, slider->posY - 20, 15, BLACK);
    DrawCircle(slider->handlePos.x, slider->handlePos.y, slider->handleRadius, BLACK);
    DrawLine(slider->startX, slider->posY, slider->endX, slider->posY, BLACK);
}

void SliderDestroy(Slider *slider)
{
    free(slider);
}
