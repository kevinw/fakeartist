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

#include <cstdlib>

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
#ifdef addPixel
#error "addPixel already defined"
#endif

#define addPixel(x, y) if ((x) >= 0 && (y) >= 0) results.push_back(Vector2i((x), (y)));
        addPixel(x + x0, y + y0);
        addPixel(y + x0, x + y0);
        addPixel(-x + x0, y + y0);
        addPixel(-y + x0, x + y0);
        addPixel(-x + x0, -y + y0);
        addPixel(-y + x0, -x + y0);
        addPixel(x + x0, -y + y0);
        addPixel(y + x0, -x + y0);
#undef addPixel
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


Vector2i newDirection(const Vector2i& old)
{
    if (old.x == 1 && old.y == 0) {
        return Vector2i(0, 1);
    } else if (old.x == 0 && old.y == 1) {
        return Vector2i(-1, 0);
    } else if (old.x == -1 && old.y == 0) {
        return Vector2i(0, -1);
    } else if (old.x == 0 && old.y == -1) {
        return Vector2i(1, 0);
    } else {
        cerr << "unexpected case" << endl;
    }
}

//
// use texture.
//
// 00000
// 01200
// 00030
// 00004


float randomFloat() {
    return ((float)rand()) / (float)RAND_MAX;
}



Vector2i randomPoint(const FloatRect& rect)
{
    return Vector2i(randomFloat() * rect.width + rect.left,
                    randomFloat() * rect.height + rect.top);
}

Vector2i randomVelocity()
{
    return Vector2i(randomFloat()*2-1, randomFloat()*2-1);
}

VectorPixels getRandomWalk(const FloatRect& rect)
{
    VectorPixels results;
    Vector2i p(randomPoint(rect));
    Vector2i velocity(randomVelocity());
    
    int N = randomFloat() * 500;
    
    for (int i = 0; i < N; ++i) {
        int length = randomFloat() * 50;
        while(length-- > 0) {
            if (rect.contains(p.x, p.y)) {
                results.push_back(p);
                
            }
            p += velocity;
        }
    }
    
    return results;
    
}

vector<VectorPixels> getRandomWalks(const FloatRect& rect)
{
    vector<VectorPixels> results;
    int count = 200;
    for (int i = 0; i < count; ++i) {
        results.push_back(getRandomWalk(rect));
    }
    return results;
}

VectorPixels getSpiral(const FloatRect& rect_)
{
    VectorPixels results;
    
    IntRect rect(rect_);
    
    Vector2i position(rect.left, rect.top);
    Vector2i velocity(1, 0);
    
    int n = rect.width*rect.height;
    while (n--) {
        if (rect_.contains(position.x, position.y)) {
            results.push_back(position);
        }
        position += velocity;
        if (!rect.contains(position.x, position.y)) {
            position -= velocity;
            
            velocity = newDirection(velocity);
            if (velocity.x == 0 && velocity.y == -1) {
                rect.width -= 1;
                rect.height -= 1;
            } else if (velocity.x == 1 && velocity.y == 0) {
                rect.left += 1;
                rect.width -= 1;
                rect.top += 1;
                rect.height -=1;
            }
            
            position += velocity;
        }
    }
    
    return results;
    
}

vector<VectorPixels> getConcentricCircles(const FloatRect& rect)
{
    Vector2f center = {rect.left + rect.width/2, rect.top + rect.height/2};
    
    vector<VectorPixels> results;
    for (Uint32 radius = 1; radius < min(rect.width, rect.height); ++radius) {
        VectorPixels circle = getCirclePixels(center.x, center.y, radius);
        
        bool valid = true;
        for (auto pixel : circle) {
            if (!rect.contains(pixel.x, pixel.y)) {
                valid = false;
            }
        }
        
        if (valid)
            results.push_back(circle);
    }
    
    return results;
}

vector<VectorPixels> getManySpirals(const FloatRect& rect, Vector2u size)
{
    Vector2u subdivs(rect.width / size.x, rect.height / size.y);
    vector<VectorPixels> results;
    
    for (int row = 0; row < subdivs.y; ++row) {
        for (int col = 0; col < subdivs.x; ++col) {
            FloatRect subdiv = FloatRect(col * size.x, row * size.y, size.x, size.y);
            results.push_back(getSpiral(subdiv));
        }
    }
    
    return results;
}


vector<VectorPixels> getManyCircles(const FloatRect& rect, Vector2u size)
{
    Vector2u subdivs(rect.width / size.x, rect.height / size.y);
    vector<VectorPixels> results;
    
    for (int row = 0; row < subdivs.y; ++row) {
        for (int col = 0; col < subdivs.x; ++col) {
            FloatRect subdiv = FloatRect(col * size.x, row * size.y, size.x, size.y);
            for (auto circle : getConcentricCircles(subdiv)) {
                results.push_back(circle);
            }
        }
    }
    
    return results;
}

static inline Uint8 intensityAtPixel(Uint8* pixels, const Vector2u& size, const Vector2u& pos)
{
  const Uint8* pixel = &pixels[(pos.x + pos.y * size.x) * 4];
  return (Uint8)((pixel[0] + pixel[1] + pixel[2]) / 3.0f);
}

static inline Uint8 intensityAtPixel(Uint32* pixels, Uint32 pixelWidth, int x, int y)
{
  const Uint8* pixel = reinterpret_cast<Uint8*>(&pixels[y * pixelWidth + x]);
  return static_cast<Uint8>((pixel[0] + pixel[1] + pixel[2]) / 3.0f);
}


Uint32* getWritablePixels(Image& image)
{
    return reinterpret_cast<Uint32*>(const_cast<Uint8*>(image.getPixelsPtr()));
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

int getFirstNotBlackRun(Uint32* pixels, const Vector2u& size, const VectorPixels& run, int index, Uint8 blackValue)
{
    if (index >= run.size())
        return -1;
    
    while (intensityAtPixel(pixels, size.x, run[index].x, run[index].y) < blackValue) {
        index++;
        
        if (index >= run.size())
            return -1;
    }
    return index;
}

int getFirstNotBlackX(Uint32* pixels, const Vector2u& size, int _x, int _y, Uint8 blackValue) {
    int x = _x;
    int y = _y;
    while (intensityAtPixel(pixels, size.x, x, y) < blackValue) {
        ++x;
        if (x >= size.x)
            return -1;
    }
    return x;
}

int getFirstNotBlackY(Uint32* pixels, const Vector2u& size, int _x, int _y, Uint8 blackValue) {
    int x = _x;
    int y = _y;
    while (intensityAtPixel(pixels, size.x, x, y) < blackValue) {
        if (++y >= size.y)
            return -1;
    }
    return y;
}

int getNextBlackX(Uint32* pixels, const Vector2u& size, int _x, int _y, Uint8 blackValue) {
    int x = _x+1;
    int y = _y;
    while (x < size.x && intensityAtPixel(pixels, size.x, x, y) > blackValue) {
        if (++x >= size.x)
            return size.x - 1;
    }
    return x - 1;
}

int getNextBlackRun(Uint32* pixels, const Vector2u& size, const VectorPixels& run, int index, Uint8 blackValue) {
    index++;
    if (index >= run.size())
        return run.size() - 1;
    
    while (intensityAtPixel(pixels, size.x, run[index].x, run[index].y) > blackValue) {
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

int getNextBlackY(Uint32* pixels, const Vector2u& size, int _x, int _y, Uint8 blackValue) {
    int x = _x;
    int y = _y + 1;
    const int height = size.y;
    if (y >= height)
        return height - 1;
    
    while (intensityAtPixel(pixels, size.x, x, y) > blackValue) {
        y++;
        if (y >= height)
            return height - 1;
    }
    return y - 1;
}


int comp( const void* a, const void* b ) {
    return ( *( Uint32* )a - *( Uint32* )b );
}

void sortRun(Image& image, const VectorPixels& run, Uint8 blackValue)
{
    std::vector<Uint32> unsorted;
    
    int index = 0;
    int indexEnd = 0;
    Uint32* pixels = getWritablePixels(image);
    const Vector2u& size = image.getSize();
    while (indexEnd < run.size()) {
        index = getFirstNotBlackRun(pixels, size, run, index, blackValue);
        indexEnd = getNextBlackRun(pixels, size, run, index, blackValue);
        if (index < 0)
            break;
        
        int sortLength = indexEnd - index;
        if (sortLength < 0)
            sortLength = 0;
        unsorted.resize(sortLength);
        
        for (int i = 0; i < sortLength; ++i) {
            const Vector2i& p = run[index + i];
            unsorted[i] = pixels[p.y * size.x + p.x];
        }
        
        std::sort(begin(unsorted), end(unsorted));
        //std::qsort(&unsorted[0], unsorted.size(), sizeof( Uint32 ), comp );

        
        for (int i = 0; i < sortLength; ++i) {
            const Vector2i& p = run[index + i];
            pixels[p.y * size.x + p.x] = unsorted[i];
        }
        
        index = indexEnd + 1;
    }
}




void sortCol(Image& image, int column, Uint8 blackValue)
{
    int x = column;
    int y = 0;
    int yend = 0;
    
    Uint32* pixels = getWritablePixels(image);
    const Vector2u& size = image.getSize();
    std::vector<Uint32> unsorted;
    
    while (yend < image.getSize().y - 1) {
        y = getFirstNotBlackY(pixels, size, x, y, blackValue);
        yend = getNextBlackY(pixels, size, x, y, blackValue);
        
        if (y < 0) break;
        
        int sortLength = yend-y;
        unsorted.resize(sortLength);
        
        for (int i = 0; i < sortLength; ++i) {
            unsorted[i] = pixels[(y + i) * size.x + x];
        }
        
        std::sort(unsorted.begin(), unsorted.end());
        //std::qsort(&unsorted[0], unsorted.size(), sizeof( Uint32 ), comp );

        
        for (int i = 0; i < sortLength; ++i) {
            pixels[(y + i) * size.x + x] = unsorted[i];
        }
        
        y = yend + 1;
    }
}

void sortRow(Image& image, int row, Uint8 blackValue)
{
    int x = 0;
    int y = row;
    int xend = 0;
    
    Uint32* pixels = reinterpret_cast<Uint32*>(const_cast<Uint8*>(image.getPixelsPtr()));
    Uint32 pixelsWidth = image.getSize().x;
    const Vector2u& size = image.getSize();
    std::vector<Uint32> unsorted;
  
    while (xend < pixelsWidth - 1) {
        x = getFirstNotBlackX(pixels, size, x, y, blackValue);
        xend = getNextBlackX(pixels, size, x, y, blackValue);
    
        if (x < 0) break;
    
        int sortLength = xend - x;
        unsorted.resize(sortLength);
        
        for (int i = 0; i < sortLength; ++i) {
            unsorted[i] = pixels[y * pixelsWidth + (x + i)];
        }
        
        std::sort(unsorted.begin(), unsorted.end());
        //std::qsort(&unsorted[0], unsorted.size(), sizeof( Uint32 ), comp );

        
        for (int i = 0; i < sortLength; ++i) {
            pixels[y * pixelsWidth + (x + i)] = unsorted[i];
        }
    
        x = xend + 1;
    }
}

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

void prettySort(Image& image, float mouseX, float mouseY)
{
    const int width = image.getSize().x;
    const int height = image.getSize().y;
    
    FloatRect imageRect(0, 0, image.getSize().x, image.getSize().y);
    for (int col = 0; col < width; ++col) {
        sortCol(image, col, 255 * mouseY);
    }

    for (int row = 0; row < height; ++row) {
        sortRow(image, row, 255 * mouseX);
    }
    for (auto run : getManySpirals(imageRect, Vector2u(mouseX * 400, mouseY * 400))) {
        sortRun(image, run, mouseX * 255);
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
    }
    
    Uint32 mediaIndex = medias.size() - 1;

    sf::Clock clock;

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
        
        float mouseX = static_cast<float>(Mouse::getPosition(window).x) / window.getSize().x;
        float mouseY = static_cast<float>(Mouse::getPosition(window).y) / window.getSize().y;

        movie.update();
        Image imageCopy = movie.getCurrentImage().copyToImage();
        prettySort(imageCopy, mouseX, mouseY);
        // prettySort2(imageCopy, Vector2f(mouseX, mouseY));
        texture.update(imageCopy);

        window.clear();
        window.draw(movieSprite);
        window.display();
    }

    return EXIT_SUCCESS;
}
