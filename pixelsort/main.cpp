#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include "video/Movie.hpp"

#include <cstdlib>
#include <cmath>

#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <thread>

#include "ResourcePath.hpp"
#include "platform.h"

#include <Magick++.h>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"


using namespace sf;
using namespace std;

typedef vector<Vector2i> VectorPixels;

static const bool showFps = false;

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

static void updateStateFromKeyboard(State& state, Keyboard::Key keyCode)
{
    switch (keyCode) {
        case Keyboard::Num1:
            state.diagonals = !state.diagonals;
            break;
        case Keyboard::Num2:
            state.cols = !state.cols;
            break;
        case Keyboard::Num3:
            state.rows = !state.rows;
            break;
        case Keyboard::Num4:
            state.circles = !state.circles;
            break;
        case Keyboard::Num5:
            state.spirals = !state.spirals;
            break;
        case Keyboard::Num6:
            state.random = !state.random;
            break;
        default:
            break;
    }
}

Vector2i randomPoint(const FloatRect& rect)
{
    return Vector2i(randomFloat() * (float)rect.width + rect.left,
                    randomFloat() * (float)rect.height + rect.top);
}

Vector2f randomVelocity()
{
    return Vector2f(randomFloat() * 2.0f - 1.0f, randomFloat() * 2.0f - 1.0f);
}

VectorPixels getRandomWalk(const FloatRect& rect)
{
    VectorPixels results;
    Vector2f p(randomPoint(rect));
    Vector2f velocity(randomVelocity());
    int N = randomFloat() * 500;
    
    for (int i = 0; i < N; ++i) {
        int length = randomFloat() * 30;
        while(length-- > 0) {
            Vector2i nearestInt(round(p.x), round(p.y));
            if (rect.contains(nearestInt.x, nearestInt.y)) {
                if (results.size() == 0 || results[results.size()-1] != nearestInt) {
                    results.push_back(nearestInt);
                }
            }
            p += velocity;
        }
    }
    
    return results;
}

vector<VectorPixels> getRandomWalks(const FloatRect& rect)
{
    vector<VectorPixels> results;
    int count = 500;
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

VectorPixels getDiagonal(const FloatRect& rect, const Vector2u& pos, float factor)
{
    Vector2f p(pos.x, pos.y);
    VectorPixels results;
    int length = 2 * max(rect.width, rect.height);
    Vector2f vel(sin(factor), cos(factor));
    bool started = false;
    while (length--) {
        Vector2i i(p.x, p.y);
        if (rect.contains(i.x, i.y)) {
            started = true;
            results.push_back(i);
        } else if (started) {
            break;
        }
        p.x += vel.x;
        p.y += vel.y;
    }
    return results;
}

vector<VectorPixels> getDiagonals(const FloatRect& rect, float factor)
{
    vector<VectorPixels> results;
    for (int col = -rect.width/2; col < rect.width*2; ++ col) {
        results.push_back(getDiagonal(rect, Vector2u(col, 0), factor));
    }
    for (int row = -rect.height/2; row < rect.height*2; ++row) {
        results.push_back(getDiagonal(rect, Vector2u(0, row), factor));
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


void sortRun(Uint32* pixels, const Vector2u& size, const VectorPixels& run, Uint8 blackValue)
{
    std::vector<Uint32> unsorted;
    
    int index = 0;
    int indexEnd = 0;
    while (indexEnd < run.size()) {
        index = getFirstNotBlackRun(pixels, size, run, index, blackValue);
        indexEnd = getNextBlackRun(pixels, size, run, index, blackValue);
        if (index < 0)
            break;
        
        int sortLength = indexEnd - index;
        if (sortLength < 0)
            sortLength = 0;

        if (unsorted.size() < sortLength)
          unsorted.resize(sortLength);
        
        for (int i = 0; i < sortLength; ++i) {
            const Vector2i& p = run[index + i];
            unsorted[i] = pixels[p.y * size.x + p.x];
        }
        
        std::sort(begin(unsorted), end(unsorted));
        
        for (int i = 0; i < sortLength; ++i) {
            const Vector2i& p = run[index + i];
            pixels[p.y * size.x + p.x] = unsorted[i];
        }
        
        index = indexEnd + 1;
    }
}


void sortRuns(Image& image, const vector<VectorPixels>& runs, Uint8 blackValue)
{
    Uint32* pixels = getWritablePixels(image);
    for (auto run : runs) {
        sortRun(pixels, image.getSize(), run, blackValue);
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
        if (unsorted.size() < sortLength)
          unsorted.resize(sortLength);
        
        for (int i = 0; i < sortLength; ++i) {
            unsorted[i] = pixels[(y + i) * size.x + x];
        }
        
        std::sort(unsorted.begin(), unsorted.end());
        
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
        if (unsorted.size() < sortLength)
          unsorted.resize(sortLength);
        
        for (int i = 0; i < sortLength; ++i) {
            unsorted[i] = pixels[y * pixelsWidth + (x + i)];
        }
        
        std::sort(unsorted.begin(), unsorted.end());
        
        for (int i = 0; i < sortLength; ++i) {
            pixels[y * pixelsWidth + (x + i)] = unsorted[i];
        }
    
        x = xend + 1;
    }
}


void prettySort(Image& image, const State& state)
{
    FloatRect imageRect(0, 0, image.getSize().x, image.getSize().y);

    if (state.circles) {
        sortRuns(image, getManyCircles(imageRect, Vector2u(200, 200)), state.mouseX * 255);
    }
   
    if (state.cols) {
        for (int col = 0; col < imageRect.width; ++col) {
            sortCol(image, col, 255 * state.mouseY);
        }
    }

    if (state.rows) {
        for (int row = 0; row < imageRect.height; ++row) {
            sortRow(image, row, 255 * state.mouseX);
        }
    }
    
    if (state.spirals) {
        float f = sin(state.time / 1000 / 5) * 400 + 400;
        int spiralSize = static_cast<int>(f);
        auto runs = getManySpirals(imageRect, Vector2u(spiralSize, spiralSize));
        sortRuns(image, runs, state.mouseX * 255);
    }
    
    if (state.random) {
        sortRuns(image, getRandomWalks(imageRect), state.mouseX * 255);
    }
    
    if (state.diagonals) {
        sortRuns(image, getDiagonals(imageRect, state.mouseY), state.mouseX * 255);
    }
}

Magick::Image sfmlToMagick(sf::Image& sfImage)
{
    const sf::Vector2u size(sfImage.getSize());
    Magick::Image image(Magick::Geometry(size.x, size.y));
    image.type(Magick::ImageType::TrueColorType);
    image.modifyImage();
    
}


namespace sfe {
    void dumpAvailableDemuxers();
    void dumpAvailableDecoders();
}

struct Media
{
    enum MediaType
    {
        IMAGE,
        MOVIE,
        WEBCAM
    };
    
    MediaType type;
    string filename;
};

float clamp(float v, float minval=0.0f, float maxval=1.0f)
{
    return min(maxval, max(minval, v));
}

void writeThread(vector<Magick::Image> images, std::string path)
{
    cout << "Writing " << path << endl;
    Magick::writeImages(begin(images), end(images), path);
    cout << "...finished!" << endl;
}

int main(int, char const**)
{
    RenderWindow window(VideoMode(800, 600), "fake artist");
    
    Image icon;
    if (!icon.loadFromFile(resourcePath() + "icon.png"))
        return EXIT_FAILURE;

    window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    window.setFramerateLimit(30);

    sfe::Movie movie;

    Texture texture;
    Sprite displaySprite;

    vector<string> movieFilenames = findMovies();
    
    vector<Media> medias;
    Media webcamMedia = {Media::WEBCAM};
    medias.push_back(webcamMedia);

    for (auto movieFilename : findMovies()) {
        Media media = {Media::MOVIE, movieFilename};
        medias.push_back(media);
    }
    
    Int32 mediaIndex = 0;
    Int32 oldMediaIndex;

    sf::Clock globalClock;
    State state;
    bool updateMedia = true;
    bool firstUpdate = true;

    Image prettyImage;
    
    Magick::InitializeMagick(nullptr);
    vector<Image> animated;
    bool recording = false;

    cv::VideoCapture webcam;
    cv::Mat frameRGB, frameRGBA;

    while (window.isOpen()) {
        Event event;
        
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
            
            if (!recording && event.type == Event::KeyPressed && event.key.code == Keyboard::Space) {
                cout << "Starting to record." << endl;
                recording = true;
            } else if (recording && event.type == Event::KeyReleased && event.key.code == Keyboard::Space) {
                if (animated.size() > 0) {
                    string path = "/Users/kevin/Desktop/test.gif";
                    cout << "Writing " << animated.size() << " images to " << path << endl;

                    vector<Magick::Image> magickAnimated;
                    for (auto sfImage : animated) {
                        const void* pixels = getWritablePixels(sfImage);
                        
                        Magick::Image magickImage(sfImage.getSize().x, sfImage.getSize().y, "RGBA", Magick::StorageType::CharPixel, pixels);
                        magickImage.animationDelay(1);
                        magickAnimated.push_back(magickImage);
                    }
                    thread saveThread(bind(writeThread, magickAnimated, path));
                    saveThread.detach();
                    animated.resize(0);
                }
                recording = false;
            }
            
            if (recording) {
                animated.push_back(prettyImage);
            }
            

            if (event.type == Event::KeyPressed) {
                switch (event.key.code) {
                    case Keyboard::Escape:
                        window.close();
                        break;
                    case Keyboard::Return:
                        if (movie.getStatus() == sfe::Status::Stopped || movie.getStatus() == sfe::Status::Paused) {
                            movie.play();
                        } else {
                            movie.pause();
                        }
                        break;
                    case Keyboard::Down:
                        oldMediaIndex = mediaIndex;
                        mediaIndex = (mediaIndex + 1) % medias.size();
                        updateMedia = true;
                        break;
                    case Keyboard::Up:
                        oldMediaIndex = mediaIndex;
                        mediaIndex = (mediaIndex - 1) % medias.size();
                        cout << oldMediaIndex << " TO " << mediaIndex << endl;
                        updateMedia = true;
                        break;
                    default:
                        break;
                }

                updateStateFromKeyboard(state, event.key.code);
            }
        }
        
        Media& activeMedia = medias[mediaIndex];

        if (updateMedia) {
            updateMedia = false;
            
            Media& oldMedia = medias[oldMediaIndex];
            
            cout  << "new index " << mediaIndex << " " << oldMedia.type << " to " << activeMedia.type << endl;

            if (!firstUpdate) {
                if (oldMedia.type == Media::WEBCAM) {
                    if (webcam.isOpened()) {
                      webcam.release();
                    }
                }
                if (oldMedia.type == Media::MOVIE) {
                    movie.stop();
                }
            } else {
                firstUpdate = false;
            }
            
            if (activeMedia.type == Media::MOVIE) {
              cout << "Loading movie " << activeMedia.filename << endl;
            
              movie.openFromFile(activeMedia.filename);
              texture.create(movie.getSize().x, movie.getSize().y);
              displaySprite.setTexture(texture, true);
              displaySprite.setOrigin(movie.getSize().x/2.0f, movie.getSize().y/2.0f);
              displaySprite.setPosition(window.getSize().x/2.0f, window.getSize().y/2.0f);
              displaySprite.setRotation(movie.getVideoRotation());
            } else if (activeMedia.type == Media::WEBCAM) {
                webcam.open(0);
                
                /* HACK */
                const static int CV_CAP_PROP_FRAME_WIDTH    =3;
                const static int CV_CAP_PROP_FRAME_HEIGHT   =4;

                int width = webcam.get(CV_CAP_PROP_FRAME_WIDTH);
                int height = webcam.get(CV_CAP_PROP_FRAME_HEIGHT);

                texture.create(width, height);
                displaySprite.setTexture(texture, true);
                displaySprite.setOrigin(width/2.0f, height/2.0f);
                displaySprite.setPosition(window.getSize().x/2.0f, window.getSize().y/2.0f);
                displaySprite.setRotation(0);
            }

        }
        
        if (movie.getStatus() == sfe::Status::Stopped) {
            movie.play();
        }
        
        state.mouseX = clamp(static_cast<float>(Mouse::getPosition(window).x) / window.getSize().x);
        state.mouseY = clamp(static_cast<float>(Mouse::getPosition(window).y) / window.getSize().y);

        state.time = globalClock.getElapsedTime().asSeconds();

        if (activeMedia.type == Media::MOVIE) {
          movie.update();
          prettyImage = movie.getCurrentImage().copyToImage();
        } else if (activeMedia.type == Media::WEBCAM) {
            webcam >> frameRGB;
            if (!frameRGB.empty()) {
                cv::cvtColor(frameRGB, frameRGBA, cv::COLOR_BGR2RGBA);
                prettyImage.create(frameRGBA.cols, frameRGBA.rows, frameRGBA.ptr());
            }
        }
        
        prettySort(prettyImage, state);
        texture.update(prettyImage);
        
        window.clear();
        window.draw(displaySprite);
        window.display();
    }
    
//    edgeMain("/Users/kevin/Desktop/penguins.jpg");

    return EXIT_SUCCESS;
}
