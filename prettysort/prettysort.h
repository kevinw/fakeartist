#ifndef prettysort_
#define prettysort_

#include <vector>

#include <SFML/Graphics.hpp>

using namespace std;
using namespace sf;

typedef vector<Vector2i> VectorPixels;

struct State
{
    float mouseX;
    float mouseY;
    float time;
    
    bool diagonals = true;
    bool cols = false;
    bool rows = false;
    bool circles = false;
    bool spirals = false;
    bool random = false;
};

void prettySort(Image& image, State& state);

inline Uint32* getWritablePixels(Image& image)
{
    return reinterpret_cast<Uint32*>(const_cast<Uint8*>(image.getPixelsPtr()));
}

#endif