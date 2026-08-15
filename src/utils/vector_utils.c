#ifndef VECTOR_UTILS
#define VECTOR_UTILS

#include <raylib.h>

Vector2 Vec2Add(Vector2 a, Vector2 b)
{
    return (Vector2) {
        .x = a.x + b.x,
        .y = a.y + b.y
    };
}

Vector2 Vec2Subtract(Vector2 a, Vector2 b)
{
    return (Vector2) {
        .x = a.x - b.x,
        .y = a.y - b.y
    };
}

Vector2 Vec2Scale(Vector2 a, float k)
{
    return (Vector2) {
        .x = k * a.x,
        .y = k * a.y
    };
}

Vector2 Vec2Normalize(Vector2 a)
{
    float len = sqrt(a.x*a.x + a.y*a.y);
    return Vec2Scale(a, 1/len);
}

#endif // VECTOR_UTILS
