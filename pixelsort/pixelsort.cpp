#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include "video/Movie.hpp"

#include <vector>
#include <iostream>
#include <algorithm>

#include "ResourcePath.hpp"
#include "platform.h"

using namespace sf;
using namespace std;


void initializeRenderWindow(RenderWindow& window) {
  window.create(VideoMode(1024, 768), "fake artist");
}


int main2()
{
  RenderWindow window;
  initializeRenderWindow(window);
  while (window.isOpen()) {
    Event event;

    while (window.pollEvent(event)) {
      if (event.type == Event::Closed) {
        window.close();
      }
    }

  }
  return EXIT_SUCCESS;
}
