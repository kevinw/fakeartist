
// Note that the "Run Script" build phase will copy the required frameworks
// or dylibs to your application bundle so you can execute it on any OS X
// computer.
//
// Your resource files (images, sounds, fonts, ...) are also copied to your
// application bundle. To get the path to these resource, use the helper
// method resourcePath() from ResourcePath.hpp

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <sfeMovie/Movie.hpp>

#include <vector>
#include <iostream>

#include "ResourcePath.hpp"
#include "platform.h"

using namespace sf;
using namespace std;

static inline Uint8 intensityAtPixel(Image& image, int x, int y)
{
    Color c = image.getPixel(x, y);
    return (Uint8)((c.r + c.g + c.b) / 3.0f);
}

Uint32 getFirstNotWhiteX(Image& image, int x, int y, Uint8 whiteValue) {
    const int width = image.getSize().x;
    while (intensityAtPixel(image, x, y) > whiteValue)
        if (++x >= width)
            return -1;
    return x;
}

Uint32 getFirstNotWhiteY(Image& image, int x, int y, Uint8 whiteValue) {
    const int height = image.getSize().y;
    while (intensityAtPixel(image, x, y) > whiteValue)
        if (++y >= height)
            return -1;
    return y;
}

int getFirstNotBlackX(Image& image, int _x, int _y, Uint8 blackValue) {
    int x = _x;
    int y = _y;
    while (intensityAtPixel(image, x, y) < blackValue) {
        ++x;
        if (x >= image.getSize().x)
            return -1;
    }
    return x;
}

int getFirstNotBlackY(Image& image, int _x, int _y, Uint8 blackValue) {
    int x = _x;
    int y = _y;
    while (intensityAtPixel(image, x, y) < blackValue) {
        ++y;
        if (y >= image.getSize().y)
            return -1;
    }
    return y;
}


int getNextBlackX(Image& image, int _x, int _y, Uint8 blackValue) {
    int x = _x+1;
    int y = _y;
    const int width = image.getSize().x;
    while (intensityAtPixel(image, x, y) > blackValue) {
        x++;
        if (x >= width)
            return width - 1;
    }
    return x - 1;
}

Uint32 getNextWhiteX(Image& image, int x, int y, Uint8 whiteValue) {
    x++;
    const int width = image.getSize().x;
    while (intensityAtPixel(image, x, y) < whiteValue)
        if (++x >= width)
            return width - 1;
    return x - 1;
}

Uint32 getNextWhiteY(Image& image, int x, int y, Uint8 whiteValue) {
    y++;
    const int height = image.getSize().y;
    while (intensityAtPixel(image, x, y) < whiteValue)
        if (++y >= height)
            return height - 1;
    return y - 1;
}

int getNextBlackY(Image& image, int _x, int _y, Uint8 blackValue) {
    int x = _x;
    int y = _y+1;
    const int height = image.getSize().y;
    if (y >= height)
        return height - 1;
    
    while (intensityAtPixel(image, x, y) > blackValue) {
        y++;
        if (y >= height)
            return height - 1;
    }
    return y - 1;
}

struct ColorCmp
{
    bool operator ()(const Color& a, const Color& b) {
        return a.r + a.g + a.b < b.r + b.g + b.b;
    }
};

static ColorCmp colorCmp;

void sortCol(Image& image, int height, int column, int mode, Uint8 blackValue) {
    int x = column;
    int y = 0;
    int yend = 0;
    
    std::vector<Color> unsorted;
    
    while(yend < height-1) {
        switch(mode) {
            case 0:
                y = getFirstNotBlackY(image, x, y, blackValue);
                yend = getNextBlackY(image, x, y, blackValue);
                break;
            case 1:
                y = getFirstNotWhiteY(image, x, y, blackValue);
                yend = getNextWhiteY(image, x, y, blackValue);
                break;
            default:
                break;
        }
        
        if(y < 0) break;
        
        int sortLength = yend-y;
        unsorted.resize(sortLength);
        
        for (int i = 0; i < sortLength; ++i) {
            unsorted[i] = image.getPixel(x, y + i);
        }
        
        std::sort(unsorted.begin(), unsorted.end(), colorCmp);
        
        for (int i = 0; i < sortLength; ++i) {
            image.setPixel(x, y + i, unsorted[i]);
        }
        
        
        y = yend+1;
    }
}


void sortRow(Image& image, int width, int row, int mode, Uint8 blackValue)
{
    int x = 0;
    int y = row;
    int xend = 0;
  
    while (xend < width - 1) {
        switch(mode) {
            case 0:
                x = getFirstNotBlackX(image, x, y, blackValue);
                xend = getNextBlackX(image, x, y, blackValue);
                break;
            case 1:
                x = getFirstNotWhiteX(image, x, y, blackValue);
                xend = getNextWhiteX(image, x, y, blackValue);
                break;
            default:
                break;
        }
    
        if(x < 0)
            break;
    
        int sortLength = xend - x;
        
        std::vector<Color> unsorted(sortLength);
        for (int i = 0; i < sortLength; ++i) {
            unsorted[i] = image.getPixel(x + i, y);
        }
        
        std::sort(unsorted.begin(), unsorted.end(), colorCmp);
        
        for (int i = 0; i < sortLength; ++i) {
            image.setPixel(x + i, y, unsorted[i]);
        }
    
        x = xend+1;
    }
}

void prettySort(Image& image, float mouseX, float mouseY, int mode)
{
    const int width = image.getSize().x;
    const int height = image.getSize().y;
    
    for (int row = 0; row < height; ++row) {
        sortRow(image, width, row, mode, 255 * mouseX);
    }
    for (int col = 0; col < width; ++col) {
        sortCol(image, height, col, mode, 255 * mouseY);
    }
}

namespace sfe {
    void dumpAvailableDemuxers();
    void dumpAvailableDecoders();
}

int main(int, char const**)
{
    RenderWindow window(VideoMode(800, 600), "fake artist");
    
    Image icon;
    if (!icon.loadFromFile(resourcePath() + "icon.png"))
        return EXIT_FAILURE;

    window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    window.setFramerateLimit(30);

    // Load a sprite to display
    Image image;
    if (!image.loadFromFile(resourcePath() + "legos.jpg"))
        return EXIT_FAILURE;
    
    sfe::Movie movie;

    Font font;
    if (!font.loadFromFile(resourcePath() + "sansation.ttf"))
        return EXIT_FAILURE;

    Texture texture;
    Sprite movieSprite;

    vector<string> movieFilenames = findMovies();
    
    struct Media
    {
        enum MediaType
        {
            IMAGE,
            MOVIE
        };
        
        MediaType type;
        string filename;
    };
    
    vector<Media> medias;
    for (auto movieFilename : findMovies()) {
        Media media = {Media::MOVIE, movieFilename};
        medias.push_back(media);
        printf("loaded movie '%s'\n", movieFilename.c_str());
    }
    
    Uint32 mediaIndex = 0;

    sf::Clock clock;
    int mode = 0;

    bool updateMedia = true;

    while (window.isOpen()) {
        Event event;
        
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }

            if (event.type == Event::KeyPressed) {
                switch (event.key.code) {
                    case Keyboard::Escape:
                        window.close();
                        break;
                    case Keyboard::Num1:
                        mode = 0;
                        break;
                    case Keyboard::Num2:
                        mode = 1;
                        break;
                    case Keyboard::Right:
                        mediaIndex = (mediaIndex + 1) % medias.size();
                        updateMedia = true;
                        break;
                    case Keyboard::Left:
                        mediaIndex = (mediaIndex - 1) % medias.size();
                        updateMedia = true;
                        break;
                    default:
                        break;
                }
            }
        }
        
        if (updateMedia) {
            updateMedia = false;
            
            Media& activeMedia = medias[mediaIndex];
            
            cout << activeMedia.filename << endl;
            
            movie.openFromFile(activeMedia.filename);
            
            texture.create(movie.getSize().x, movie.getSize().y);
            movieSprite.setTexture(texture);
            movieSprite.setOrigin(movie.getSize().x/2.0f, movie.getSize().y/2.0f);
            movieSprite.rotate(90);
            movieSprite.setPosition(window.getSize().x/2.0f, window.getSize().y/2.0f);
        }
        
        if (movie.getStatus() == sfe::Status::Stopped) {
            movie.play();
        }
        
        float mouseY = static_cast<float>(Mouse::getPosition(window).x) / window.getSize().x;
        float mouseX = static_cast<float>(Mouse::getPosition(window).y) / window.getSize().y;

        movie.update();
        Image imageCopy = movie.getCurrentImage().copyToImage();
        prettySort(imageCopy, mouseX, mouseY, mode);
        texture.update(imageCopy);

        window.clear();
        window.draw(movieSprite);
        window.display();
    }

    return EXIT_SUCCESS;
}
