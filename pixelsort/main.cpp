//
// TODO: 
//
// use a square texture instead of checking bounds
//

// Note that the "Run Script" build phase will copy the required frameworks
// or dylibs to your application bundle so you can execute it on any OS X
// computer.
//
// Your resource files (images, sounds, fonts, ...) are also copied to your
// application bundle. To get the path to these resource, use the helper
// method resourcePath() from ResourcePath.hpp

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

typedef vector<Vector2i> VectorPixels;


// image ->

// runs of pixels across the image with which to sort

// sorts to apply

static vector<Vector2i> getCirclePixels(int x0, int y0, int radius)
{
    int x = radius;
    int y = 0;
    int radiusError = 1 - x;

    vector<Vector2i> results;
    
    while(x >= y)
    {
        results.push_back(Vector2i(x + x0, y + y0));
        results.push_back(Vector2i(y + x0, x + y0));
        results.push_back(Vector2i(-x + x0, y + y0));
        results.push_back(Vector2i(-y + x0, x + y0));
        results.push_back(Vector2i(-x + x0, -y + y0));
        results.push_back(Vector2i(-y + x0, -x + y0));
        results.push_back(Vector2i(x + x0, -y + y0));
        results.push_back(Vector2i(y + x0, -x + y0));
        y++;
        if (radiusError < 0)
        {
            radiusError += 2 * y + 1;
        }
        else
        {
            x--;
            radiusError += 2 * (y - x + 1);
        }
    }

    return results;
}


vector<VectorPixels> getConcentricCircles(const FloatRect& rect)
{
    Vector2f center = {rect.left + rect.width/2, rect.top + rect.height/2};
    
    vector<VectorPixels> results;
    for (Uint32 radius = 1; radius < max(rect.width, rect.height); ++radius) {
        results.push_back(getCirclePixels(center.x, center.y, radius));
    }
    
    return results;
}

static inline Uint8 intensityAtPixel(Uint8* pixels, const Vector2u& size, const Vector2u& pos)
{
  const Uint8* pixel = &pixels[(pos.x + pos.y * size.x) * 4];
  return (Uint8)((pixel[0] + pixel[1] + pixel[2]) / 3.0f);
}

static inline Uint8 intensityAtPixel(Image& image, int x, int y)
{
    if (x < 0) x = 0;
    if (x >= image.getSize().x)
        x = image.getSize().x - 1;
    if (y < 0) y = 0;
    if (y >= image.getSize().y)
        y = image.getSize().y - 1;
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

int getFirstNotBlackRun(Image& image, const VectorPixels& run, int index, Uint8 blackValue)
{
    if (index >= run.size())
        return -1;
    
    while (intensityAtPixel(image, run[index].x, run[index].y) < blackValue) {
        index++;
        
        if (index >= run.size())
            return -1;
    }
    return index;
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

int getNextBlackRun(Image& image, const VectorPixels& run, int index, Uint8 blackValue) {
    index++;
    if (index >= run.size())
        return run.size() - 1;
    
    while (intensityAtPixel(image, run[index].x, run[index].y) > blackValue) {
        index++;
        if (index >= run.size())
            return run.size() - 1;
    }
    return index - 1;
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

struct PixelCmp
{
  bool operator ()(const Uint8* a, const Uint8* b) {
    return a[0] + a[1] + a[2] < b[1] + b[2] + b[3];
  }
};

static PixelCmp pixelCmp;

struct ColorCmp
{
    bool operator ()(const Color& a, const Color& b) {
        return a.r + a.g + a.b < b.r + b.g + b.b;
    }
};

static ColorCmp colorCmp;

Color getPixelSafe(Image& image, Vector2i point) {
    int x = point.x;
    int y = point.y;
    if (x < 0)
        x = 0;
    if (x >= image.getSize().x)
        x = image.getSize().x - 1;
    if (y < 0)
        y = 0;
    if (y >= image.getSize().y)
        y = image.getSize().y - 1;
    return image.getPixel(x, y);
}

void sortRun(Image& image, const VectorPixels& run, Uint8 blackValue)
{
    std::vector<Color> unsorted;
    
    int index = 0;
    int indexEnd = 0;
    while (indexEnd < run.size()) {
        index = getFirstNotBlackRun(image, run, index, blackValue);
        indexEnd = getNextBlackRun(image, run, index, blackValue);
        if (index < 0)
            break;
        
        int sortLength = indexEnd - index;
        if (sortLength < 0)
            sortLength = 0;
        unsorted.resize(sortLength);
        
        for (int i = 0; i < sortLength; ++i) {
            unsorted[i] = getPixelSafe(image, run[index+i]);
        }
        
        std::sort(begin(unsorted), end(unsorted), colorCmp);
        
        for (int i = 0; i < sortLength; ++i) {
            int x = run[index + i].x;
            int y = run[index + i].y;
            if (x < 0 || y < 0 || x >= image.getSize().x || y >= image.getSize().y)
                continue;
            
            image.setPixel(x, y, unsorted[i]);
        }
        
        index = indexEnd + 1;
    }
}

void sortCol(Image& image, int column, int mode, Uint8 blackValue) {
    int x = column;
    int y = 0;
    int yend = 0;
    
    std::vector<Color> unsorted;
    
    while(yend < image.getSize().y - 1) {
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

void sortRow(Image& image, int row, int mode, Uint8 blackValue)
{
    int x = 0;
    int y = row;
    int xend = 0;
  
    while (xend < image.getSize().x - 1) {
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

/*
for (auto run : getRuns(image)) {
  for (auto span : getSpans(run)) {
    std::sort(span.begin(), span.end(), colorCmp);
  }
}
*/

template <typename T>
struct ImageIterator
{
  ImageIterator(Uint8* pixels, T* run)
    : m_pixels(pixels)
    , m_run(run)
  {
  }

  Uint8* m_pixels;
  T* m_run;
};

struct Run
{
  Run(Uint8* pixels, Vector2u size, Uint32 row) 
    : m_pixels(pixels)
    , m_size(size)
    , m_row(row)
  {
  }


  const ImageIterator<Run> begin() {
    return ImageIterator<Run>(&m_pixels[m_row * m_size.y * 4], this);
  }

  const ImageIterator<Run> end() {
    return ImageIterator<Run>(&m_pixels[(m_size.x + m_row * m_size.y) * 4], this);
  }

  Uint8* m_pixels;
  Vector2u m_size;
  Uint32 m_row;
};

vector<Run> getRuns(Image& image) {
  vector<Run> runs;

  return runs;
}

struct Span
{
  Uint8** begin()
  {
    return nullptr;
  }

  Uint8** end()
  {
    return nullptr;
  }
};

vector<Span> getSpans(const Run& run) {
  vector<Span> spans;

  return spans;
}

void prettySort2(Image& image, const Vector2f& mouse, int mode)
{
  for (auto run : getRuns(image)) {
    for (auto span : getSpans(run)) {
      std::sort(span.begin(), span.end(), pixelCmp);
    }
  }
}

void prettySort(Image& image, float mouseX, float mouseY, int mode)
{
    const int width = image.getSize().x;
    const int height = image.getSize().y;
    
    for (auto run : getConcentricCircles(FloatRect(0, 0, image.getSize().x, image.getSize().y))) {
        sortRun(image, run, 255 - 255 * mouseX);
    }

    for (int row = 0; row < height; ++row) {
        sortRow(image, row, mode, 255 * mouseX);
    }
    
    for (int col = 0; col < width; ++col) {
        sortCol(image, col, mode, 255 * mouseY);
    }
}

namespace sfe {
    void dumpAvailableDemuxers();
    void dumpAvailableDecoders();
}

int main2();

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
    
    Uint32 mediaIndex = medias.size() - 1;

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
            movieSprite.setTexture(texture, true);
            movieSprite.setOrigin(movie.getSize().x/2.0f, movie.getSize().y/2.0f);
            movieSprite.setPosition(window.getSize().x/2.0f, window.getSize().y/2.0f);
            movieSprite.setRotation(movie.getVideoRotation());

        }
        
        if (movie.getStatus() == sfe::Status::Stopped) {
            cout << "replaying video" << endl;
            movie.play();
        }
        
        float mouseY = static_cast<float>(Mouse::getPosition(window).x) / window.getSize().x;
        float mouseX = static_cast<float>(Mouse::getPosition(window).y) / window.getSize().y;

        movie.update();
        Image imageCopy = movie.getCurrentImage().copyToImage();
        // prettySort(imageCopy, mouseX, mouseY, mode);
        prettySort2(imageCopy, Vector2f(mouseX, mouseY), mode);
        texture.update(imageCopy);

        window.clear();
        window.draw(movieSprite);
        window.display();
    }

    return EXIT_SUCCESS;
}
