#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include "video/Movie.hpp"

#include <cstdlib>

#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <thread>

#include <dlfcn.h>
#include <stdlib.h>


#include "ResourcePath.hpp"
#include "platform.h"

#include <Magick++.h>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "prettysort.h"


using namespace sf;
using namespace std;

Magick::Image sfmlToMagick(sf::Image& sfImage)
{
    const sf::Vector2u size(sfImage.getSize());
    Magick::Image image(Magick::Geometry(size.x, size.y));
    image.type(Magick::ImageType::TrueColorType);
    image.modifyImage();
    
}

static void* prettySortLib = nullptr;
typedef void (*prettySortFuncType)(Image& image, const State& state);
prettySortFuncType prettySortFunc = nullptr;

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

static const char* prettySortLibName = "libprettysort.dylib";

void reload()
{
    if (prettySortLib) {
        dlclose(prettySortLib);
    }
    
    prettySortLib = dlopen(prettySortLibName, RTLD_LOCAL|RTLD_LAZY);
    if (!prettySortLib) {
        cerr << "could not load libprettysort.dylib" << endl;
    } else {
        prettySortFunc = reinterpret_cast<prettySortFuncType>(dlsym(prettySortLib, "prettySort"));
        if (!prettySortFunc) {
            cerr << "could not find prettySort function" << endl;
        }
    }
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
    
    OSXWatcher watcher(prettySortLibName, reload);
    watcher.start();
    
    reload();


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
                    case Keyboard::R:
                        reload();
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
            

            if (!firstUpdate) {
                Media& oldMedia = medias[oldMediaIndex];
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
        
        state.mouseX = clamp(static_cast<float>(Mouse::getPosition(window).x) / window.getSize().x);
        state.mouseY = clamp(static_cast<float>(Mouse::getPosition(window).y) / window.getSize().y);

        state.time = globalClock.getElapsedTime().asSeconds();

        if (activeMedia.type == Media::MOVIE) {
            if (movie.getStatus() == sfe::Status::Stopped) {
                movie.play();
            }
            movie.update();
            prettyImage = movie.getCurrentImage().copyToImage();
        } else if (activeMedia.type == Media::WEBCAM) {
            webcam >> frameRGB;
            if (!frameRGB.empty()) {
                cv::cvtColor(frameRGB, frameRGBA, cv::COLOR_BGR2RGBA);
                prettyImage.create(frameRGBA.cols, frameRGBA.rows, frameRGBA.ptr());
            }
        }
        
        if (prettySortFunc)
            prettySortFunc(prettyImage, state);
        
        texture.update(prettyImage);
        
        window.clear();
        window.draw(displaySprite);
        window.display();
    }
    
//    edgeMain("/Users/kevin/Desktop/penguins.jpg");

    return EXIT_SUCCESS;
}
