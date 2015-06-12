fakeartist
==========

a video pixel sorting experiment. feeds your webcam any videos you have into a pixelsorter, live.

download: [fakeartist.app](https://dl.dropboxusercontent.com/u/108139/fakeartist/fakeartist-osx.zip) (OS X)

![screenshot](https://dl.dropboxusercontent.com/u/108139/fakeartist/fakeartist.jpg)

how to use
----------

start the program.

move the mouse to change the sorting direction and thresholds.

press 1, 2, 3, 4, and 5 to toggle different sorts.

mac only for now. fork and add windows support for me and i'll hug you!

secrets
-------

if you place videos into a folder at `~/fakeartist`, you MAY be able to sort those videos by pressing the up and down arrow keys.

building
--------

#### Mac OS X

1. Install [homebrew](http://brew.sh/)

2. Install dependencies:

```
brew install sfml ffmpeg graphicsmagick
brew tap homebrew/science
brew install opencv
```

3. Open ```fakeartist.xcodeproj``` in Xcode and build.

#### Windows / Linux

???? (need your help here guys and gals)

todo
----

- is it possible to sort on the GPU?
